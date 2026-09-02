#include "Core/Resources/PackedSectionReader.h"

#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
    enum class PackedValueType : std::uint8_t
    {
        Section = 0,
        String = 1,
        Integer = 2,
        Float = 3,
        Boolean = 4,
        Binary = 5,
        EncryptedBinary = 6,
        Reserved = 7
    };

    struct Descriptor final
    {
        PackedValueType type = PackedValueType::Reserved;
        std::uint32_t endOffset = 0;
    };

    struct ChildDescriptor final
    {
        std::uint16_t nameIndex = 0;
        Descriptor data;
    };

    class BinaryReader final
    {
    public:
        explicit BinaryReader(
            const std::span<const std::byte> data) noexcept
            : data_(data)
        {
        }

        template<typename T>
        bool Read(T& output) noexcept
        {
            if (Remaining() < sizeof(T))
            {
                return false;
            }

            std::memcpy(
                &output,
                data_.data() + position_,
                sizeof(T));

            position_ += sizeof(T);

            return true;
        }

        bool ReadString(
            const std::size_t size,
            std::string& output)
        {
            if (Remaining() < size)
            {
                return false;
            }

            output.assign(
                reinterpret_cast<const char*>(
                    data_.data() + position_),
                size);

            position_ += size;

            return true;
        }

        bool ReadBinary(
            const std::size_t size,
            std::vector<std::byte>& output)
        {
            if (Remaining() < size)
            {
                return false;
            }

            output.assign(
                data_.begin() +
                    static_cast<std::ptrdiff_t>(position_),
                data_.begin() +
                    static_cast<std::ptrdiff_t>(
                        position_ + size));

            position_ += size;

            return true;
        }

        bool ReadNullTerminatedString(
            std::string& output)
        {
            output.clear();

            while (position_ < data_.size())
            {
                const std::byte value =
                    data_[position_++];

                if (value == std::byte{0})
                {
                    return true;
                }

                output.push_back(
                    static_cast<char>(
                        std::to_integer<unsigned char>(
                            value)));
            }

            return false;
        }

        bool Skip(
            const std::size_t size) noexcept
        {
            if (Remaining() < size)
            {
                return false;
            }

            position_ += size;

            return true;
        }

        [[nodiscard]]
        std::size_t Position() const noexcept
        {
            return position_;
        }

        [[nodiscard]]
        std::size_t Size() const noexcept
        {
            return data_.size();
        }

        [[nodiscard]]
        std::size_t Remaining() const noexcept
        {
            return data_.size() - position_;
        }

    private:
        std::span<const std::byte> data_;
        std::size_t position_ = 0;
    };

    Descriptor DecodeDescriptor(
        const std::uint32_t raw) noexcept
    {
        Descriptor descriptor;

        descriptor.type =
            static_cast<PackedValueType>(
                raw >> 28u);

        descriptor.endOffset =
            raw & 0x0FFFFFFFu;

        return descriptor;
    }

    class Parser final
    {
    public:
        Parser(
            const std::span<const std::byte> data,
            std::string& error)
            : reader_(data),
              error_(error)
        {
        }

        bool Parse(
            core::resources::DataSection& output)
        {
            std::uint32_t signature = 0;

            if (!reader_.Read(signature))
            {
                return Fail(
                    "Packed section header is truncated.");
            }

            if (signature !=
                core::resources::PackedSectionReader::Signature)
            {
                return Fail(
                    "Packed section signature is invalid.");
            }

            std::uint8_t version = 0;

            if (!reader_.Read(version))
            {
                return Fail(
                    "Packed section version is missing.");
            }

            if (version != 0)
            {
                return Fail(
                    "Unsupported packed section version.");
            }

            if (!ReadStringTable())
            {
                return false;
            }

            output = {};
            output.name = "root";

            if (!ReadSection(
                    output,
                    0))
            {
                return false;
            }

            if (reader_.Position() != reader_.Size())
            {
                return Fail(
                    "Packed section contains trailing data.");
            }

            return true;
        }

    private:
        static constexpr std::size_t MaxDepth = 256;

        bool ReadStringTable()
        {
            for (;;)
            {
                std::string value;

                if (!reader_.ReadNullTerminatedString(
                        value))
                {
                    return Fail(
                        "Packed section string table is truncated.");
                }

                if (value.empty())
                {
                    break;
                }

                strings_.push_back(
                    std::move(value));
            }

            return true;
        }

        bool ReadSection(
            core::resources::DataSection& section,
            const std::size_t depth)
        {
            if (depth >= MaxDepth)
            {
                return Fail(
                    "Packed section nesting depth is too large.");
            }

            std::uint16_t childCount = 0;

            if (!reader_.Read(childCount))
            {
                return Fail(
                    "Packed section child count is truncated.");
            }

            std::uint32_t ownDescriptorRaw = 0;

            if (!reader_.Read(ownDescriptorRaw))
            {
                return Fail(
                    "Packed section value descriptor is truncated.");
            }

            const Descriptor ownDescriptor =
                DecodeDescriptor(
                    ownDescriptorRaw);

            std::vector<ChildDescriptor> childDescriptors;
            childDescriptors.resize(childCount);

            for (ChildDescriptor& child : childDescriptors)
            {
                std::uint32_t descriptorRaw = 0;

                if (!reader_.Read(child.nameIndex) ||
                    !reader_.Read(descriptorRaw))
                {
                    return Fail(
                        "Packed section child descriptor is truncated.");
                }

                child.data =
                    DecodeDescriptor(
                        descriptorRaw);
            }

            const std::size_t payloadStart =
                reader_.Position();

            std::uint32_t previousOffset = 0;

            if (!ReadValue(
                    ownDescriptor,
                    payloadStart,
                    previousOffset,
                    section,
                    depth))
            {
                return false;
            }

            previousOffset =
                ownDescriptor.endOffset;

            section.children.clear();
            section.children.reserve(childCount);

            for (const ChildDescriptor& descriptor :
                 childDescriptors)
            {
                if (descriptor.nameIndex >=
                    strings_.size())
                {
                    return Fail(
                        "Packed section references an invalid string table index.");
                }

                core::resources::DataSection child;

                child.name =
                    strings_[descriptor.nameIndex];

                if (!ReadValue(
                        descriptor.data,
                        payloadStart,
                        previousOffset,
                        child,
                        depth))
                {
                    return false;
                }

                previousOffset =
                    descriptor.data.endOffset;

                section.children.push_back(
                    std::move(child));
            }

            return true;
        }

        bool ReadValue(
            const Descriptor descriptor,
            const std::size_t payloadStart,
            const std::uint32_t previousOffset,
            core::resources::DataSection& target,
            const std::size_t depth)
        {
            if (descriptor.endOffset <
                previousOffset)
            {
                return Fail(
                    "Packed section data offsets are invalid.");
            }

            const std::uint32_t rawSize =
                descriptor.endOffset -
                previousOffset;

            const std::size_t size =
                static_cast<std::size_t>(
                    rawSize);

            if (payloadStart >
                reader_.Size())
            {
                return Fail(
                    "Packed section payload offset is invalid.");
            }

            if (descriptor.endOffset >
                reader_.Size() -
                    payloadStart)
            {
                return Fail(
                    "Packed section value exceeds file size.");
            }

            const std::size_t expectedEnd =
                payloadStart +
                descriptor.endOffset;

            switch (descriptor.type)
            {
                case PackedValueType::Section:
                {
                    const std::string name =
                        target.name;

                    core::resources::DataSection nested;
                    nested.name = name;

                    if (!ReadSection(
                            nested,
                            depth + 1))
                    {
                        return false;
                    }

                    target.value =
                        std::move(nested.value);

                    target.children =
                        std::move(nested.children);

                    break;
                }

                case PackedValueType::String:
                {
                    std::string value;

                    if (!reader_.ReadString(
                            size,
                            value))
                    {
                        return Fail(
                            "Packed string is truncated.");
                    }

                    target.value =
                        std::move(value);

                    break;
                }

                case PackedValueType::Integer:
                {
                    std::int64_t value = 0;

                    switch (size)
                    {
                        case 0:
                        {
                            value = 0;
                            break;
                        }

                        case 1:
                        {
                            std::int8_t temp = 0;

                            if (!reader_.Read(temp))
                            {
                                return Fail(
                                    "Packed integer is truncated.");
                            }

                            value = temp;
                            break;
                        }

                        case 2:
                        {
                            std::int16_t temp = 0;

                            if (!reader_.Read(temp))
                            {
                                return Fail(
                                    "Packed integer is truncated.");
                            }

                            value = temp;
                            break;
                        }

                        case 4:
                        {
                            std::int32_t temp = 0;

                            if (!reader_.Read(temp))
                            {
                                return Fail(
                                    "Packed integer is truncated.");
                            }

                            value = temp;
                            break;
                        }

                        case 8:
                        {
                            if (!reader_.Read(value))
                            {
                                return Fail(
                                    "Packed integer is truncated.");
                            }

                            break;
                        }

                        default:
                        {
                            return Fail(
                                "Packed integer has unsupported size.");
                        }
                    }

                    target.value = value;

                    break;
                }

                case PackedValueType::Float:
                {
                    if ((size % sizeof(float)) != 0)
                    {
                        return Fail(
                            "Packed float array has invalid size.");
                    }

                    const std::size_t count =
                        size / sizeof(float);

                    core::resources::DataSection::FloatArray values;
                    values.resize(count);

                    for (float& value : values)
                    {
                        if (!reader_.Read(value))
                        {
                            return Fail(
                                "Packed float array is truncated.");
                        }
                    }

                    target.value =
                        std::move(values);

                    break;
                }

                case PackedValueType::Boolean:
                {
                    const bool value =
                        size != 0;

                    if (!reader_.Skip(size))
                    {
                        return Fail(
                            "Packed boolean is truncated.");
                    }

                    target.value = value;

                    break;
                }

                case PackedValueType::Binary:
                case PackedValueType::EncryptedBinary:
                {
                    core::resources::DataSection::BinaryData value;

                    if (!reader_.ReadBinary(
                            size,
                            value))
                    {
                        return Fail(
                            "Packed binary block is truncated.");
                    }

                    target.value =
                        std::move(value);

                    break;
                }

                case PackedValueType::Reserved:
                default:
                {
                    return Fail(
                        "Packed section contains unsupported value type.");
                }
            }

            if (reader_.Position() != expectedEnd)
            {
                return Fail(
                    "Packed section value size does not match descriptor.");
            }

            return true;
        }

        bool Fail(
            std::string message)
        {
            error_ = std::move(message);
            return false;
        }

        BinaryReader reader_;
        std::vector<std::string> strings_;
        std::string& error_;
    };
}

namespace core::resources
{
    bool PackedSectionReader::Read(
        const std::span<const std::byte> data,
        DataSection& output,
        std::string& error) const
    {
        error.clear();
        output = {};

        Parser parser(
            data,
            error);

        return parser.Parse(output);
    }

    bool PackedSectionReader::HasSignature(
        const std::span<const std::byte> data) noexcept
    {
        if (data.size() <
            sizeof(std::uint32_t))
        {
            return false;
        }

        std::uint32_t signature = 0;

        std::memcpy(
            &signature,
            data.data(),
            sizeof(signature));

        return signature == Signature;
    }
}