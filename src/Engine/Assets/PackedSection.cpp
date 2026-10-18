#include "Engine/Assets/PackedSection.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace engine::assets {

    namespace {

        constexpr std::uint32_t magic = 0x62A14E45u;
        constexpr std::uint32_t offsetMask = 0x0FFFFFFFu;
        constexpr std::size_t maxFileSize = 16u * 1024u * 1024u;
        constexpr std::size_t maxNodes = 100000u;
        constexpr std::size_t maxDepth = 64u;

        class Reader final {
        public:
            explicit Reader(std::vector<std::uint8_t> bytes)
                : bytes_(std::move(bytes)) {}

            [[nodiscard]] PackedNode parse() {
                if (bytes_.size() < 12 || u32(0) != magic) {
                    throw std::runtime_error(
                        "Invalid packed resource signature"
                    );
                }

                std::size_t offset = 5;

                while (offset < bytes_.size()) {
                    const std::size_t start = offset;

                    while (offset < bytes_.size() &&
                           bytes_[offset] != 0) {
                        ++offset;
                    }

                    if (offset == bytes_.size()) {
                        throw std::runtime_error(
                            "Unterminated resource string table"
                        );
                    }

                    if (offset == start) {
                        ++offset;
                        break;
                    }

                    if (offset - start > 1024 ||
                        names_.size() >= 65536) {
                        throw std::runtime_error(
                            "Invalid resource string table"
                        );
                    }

                    names_.emplace_back(
                        reinterpret_cast<const char*>(
                            bytes_.data() + start
                        ),
                        offset - start
                    );

                    ++offset;
                }

                if (offset >= bytes_.size()) {
                    throw std::runtime_error(
                        "Missing packed resource root"
                    );
                }

                return section(
                    offset,
                    bytes_.size() - offset,
                    "root",
                    0
                );
            }

        private:
            [[nodiscard]] std::uint16_t u16(
                std::size_t offset
            ) const {
                require(offset, 2);

                std::uint16_t result = 0;
                std::memcpy(
                    &result,
                    bytes_.data() + offset,
                    sizeof(result)
                );

                return result;
            }

            [[nodiscard]] std::uint32_t u32(
                std::size_t offset
            ) const {
                require(offset, 4);

                std::uint32_t result = 0;
                std::memcpy(
                    &result,
                    bytes_.data() + offset,
                    sizeof(result)
                );

                return result;
            }

            void require(
                std::size_t offset,
                std::size_t count
            ) const {
                if (offset > bytes_.size() ||
                    count > bytes_.size() - offset) {
                    throw std::runtime_error(
                        "Truncated packed resource"
                    );
                }
            }

            [[nodiscard]] PackedNode section(
                std::size_t offset,
                std::size_t length,
                std::string name,
                std::size_t depth
            ) {
                if (++nodes_ > maxNodes || depth > maxDepth ||
                    length < 6) {
                    throw std::runtime_error(
                        "Invalid packed resource hierarchy"
                    );
                }

                require(offset, length);

                const std::size_t childCount = u16(offset);

                const std::size_t headerSize =
                    6u + childCount * 6u;

                if (headerSize > length) {
                    throw std::runtime_error(
                        "Invalid packed section header"
                    );
                }

                const std::size_t dataStart =
                    offset + headerSize;

                const std::size_t dataLength =
                    length - headerSize;

                const std::uint32_t first =
                    u32(offset + 2);

                const std::size_t ownSize =
                    first & offsetMask;

                if (ownSize > dataLength) {
                    throw std::runtime_error(
                        "Invalid packed section data size"
                    );
                }

                PackedNode result;
                result.name = std::move(name);
                result.type = static_cast<std::uint8_t>(
                    first >> 28
                );

                result.data.assign(
                    bytes_.begin() +
                        static_cast<std::ptrdiff_t>(dataStart),
                    bytes_.begin() +
                        static_cast<std::ptrdiff_t>(
                            dataStart + ownSize
                        )
                );

                result.children.reserve(childCount);

                std::size_t previous = ownSize;

                for (std::size_t i = 0; i < childCount; ++i) {
                    const std::size_t record =
                        offset + 6 + i * 6;

                    const std::uint16_t nameIndex =
                        u16(record);

                    const std::uint32_t descriptor =
                        u32(record + 2);

                    const std::size_t next =
                        descriptor & offsetMask;

                    const auto type =
                        static_cast<std::uint8_t>(
                            descriptor >> 28
                        );

                    if (nameIndex >= names_.size() ||
                        next < previous ||
                        next > dataLength) {
                        throw std::runtime_error(
                            "Invalid packed section child"
                        );
                    }

                    const std::size_t childOffset =
                        dataStart + previous;

                    const std::size_t childLength =
                        next - previous;

                    if (type == 0 && childLength >= 6) {
                        result.children.push_back(
                            section(
                                childOffset,
                                childLength,
                                names_[nameIndex],
                                depth + 1
                            )
                        );
                    } else {
                        PackedNode child;
                        child.name = names_[nameIndex];
                        child.type = type;

                        child.data.assign(
                            bytes_.begin() +
                                static_cast<std::ptrdiff_t>(
                                    childOffset
                                ),
                            bytes_.begin() +
                                static_cast<std::ptrdiff_t>(
                                    childOffset + childLength
                                )
                        );

                        result.children.push_back(
                            std::move(child)
                        );

                        if (++nodes_ > maxNodes) {
                            throw std::runtime_error(
                                "Too many packed resource nodes"
                            );
                        }
                    }

                    previous = next;
                }

                if (previous != dataLength) {
                    throw std::runtime_error(
                        "Packed section has trailing data"
                    );
                }

                return result;
            }

            std::vector<std::uint8_t> bytes_;
            std::vector<std::string> names_;
            std::size_t nodes_ = 0;
        };

        [[nodiscard]] std::vector<std::uint8_t> read_file(
            const std::filesystem::path& path
        ) {
            std::ifstream stream(
                path,
                std::ios::binary | std::ios::ate
            );

            if (!stream) {
                throw std::runtime_error(
                    "Cannot open resource: " + path.string()
                );
            }

            const auto end = stream.tellg();

            if (end < std::streampos(12) ||
                static_cast<std::uint64_t>(end) > maxFileSize) {
                throw std::runtime_error(
                    "Invalid resource size: " + path.string()
                );
            }

            std::vector<std::uint8_t> bytes(
                static_cast<std::size_t>(end)
            );

            stream.seekg(0, std::ios::beg);
            stream.read(
                reinterpret_cast<char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size())
            );

            if (!stream) {
                throw std::runtime_error(
                    "Cannot read resource: " + path.string()
                );
            }

            return bytes;
        }

    }

    const PackedNode* PackedNode::find(
        std::string_view childName
    ) const noexcept {
        const auto it = std::find_if(
            children.begin(),
            children.end(),
            [childName](const PackedNode& node) {
                return node.name == childName;
            }
        );

        return it == children.end() ? nullptr : &*it;
    }

    std::string PackedNode::text() const {
        if (type == 5) {
            constexpr char alphabet[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"
                "0123456789+/";

            std::string result;
            result.reserve(((data.size() + 2) / 3) * 4);

            // Type 5 stores decoded Base64 bytes; encode them to recover text.
            for (std::size_t i = 0; i < data.size(); i += 3) {
                const std::size_t remaining = data.size() - i;
                std::uint32_t value =
                    static_cast<std::uint32_t>(data[i]) << 16;

                if (remaining > 1) {
                    value |= static_cast<std::uint32_t>(data[i + 1]) << 8;
                }

                if (remaining > 2) {
                    value |= data[i + 2];
                }

                result.push_back(alphabet[(value >> 18) & 63]);
                result.push_back(alphabet[(value >> 12) & 63]);
                result.push_back(
                    remaining > 1 ? alphabet[(value >> 6) & 63] : '='
                );
                result.push_back(
                    remaining > 2 ? alphabet[value & 63] : '='
                );
            }

            return result;
        }

        if (type != 1) {
            throw std::runtime_error(
                "Packed node is not a string"
            );
        }

        return std::string(
            data.begin(),
            data.end()
        );
    }

    std::uint32_t PackedNode::unsigned_value() const {
        if (type != 2 || data.size() > 4) {
            throw std::runtime_error(
                "Packed node is not a supported integer"
            );
        }

        std::uint32_t value = 0;

        for (std::size_t i = 0; i < data.size(); ++i) {
            value |= static_cast<std::uint32_t>(
                data[i]
            ) << (i * 8);
        }

        return value;
    }

    bool PackedNode::boolean() const {
        if (type != 4 || data.size() > 1) {
            throw std::runtime_error(
                "Packed node is not a boolean"
            );
        }

        return !data.empty() && data.front() != 0;
    }

    PackedNode load_packed_section(
        const std::filesystem::path& path
    ) {
        try {
            return Reader(read_file(path)).parse();
        } catch (const std::exception& error) {
            throw std::runtime_error(
                path.string() + ": " + error.what()
            );
        }
    }

}
