#include "Core/World/XRay/XRayDetails.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace core::world::xray
{
    namespace
    {
        struct Span { std::uint64_t offset = 0, size = 0; bool found = false; };
        class Cursor final
        {
        public:
            Cursor(const std::byte* bytes, std::uint64_t size) : bytes_(bytes), size_(size) {}
            template<class T> bool Read(T& value)
            {
                if (sizeof(T) > size_-position_) return false;
                std::memcpy(&value, bytes_+position_, sizeof(T));
                position_ += sizeof(T);
                return true;
            }
            bool String(std::string& value)
            {
                value.clear();
                for (unsigned i = 0; i < 256; ++i)
                {
                    char ch;
                    if (!Read(ch)) return false;
                    if (ch == 0) return true;
                    value.push_back(ch);
                }
                return false;
            }
            template<class T> bool Array(std::vector<T>& values, std::uint32_t count)
            {
                const auto size = std::uint64_t(count)*sizeof(T);
                if (size > size_-position_) return false;
                values.resize(count);
                if (size) std::memcpy(values.data(), bytes_+position_, static_cast<std::size_t>(size));
                position_ += size;
                return true;
            }
            std::uint64_t Remaining() const noexcept { return size_-position_; }
        private:
            const std::byte* bytes_;
            std::uint64_t size_, position_ = 0;
        };
        bool Finite(const math::Vector3& p)
        { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
    }

    struct DetailsData::State
    {
        HANDLE file = INVALID_HANDLE_VALUE, mapping = nullptr;
        const std::byte* bytes = nullptr;
        std::uint64_t slotOffset = 0;
        DetailStatistics statistics;
        std::vector<DetailModel> models;
        ~State()
        {
            if (bytes) UnmapViewOfFile(bytes);
            if (mapping) CloseHandle(mapping);
            if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        }
    };

    DetailsData::DetailsData() = default;
    DetailsData::~DetailsData() = default;
    DetailStatistics DetailsData::Statistics() const noexcept
    { return state_ ? state_->statistics : DetailStatistics{}; }
    const std::vector<DetailModel>& DetailsData::Models() const noexcept
    {
        static const std::vector<DetailModel> empty;
        return state_ ? state_->models : empty;
    }
    bool DetailsData::ReadSlot(std::int64_t x, std::int64_t z, DetailSlot& slot) const noexcept
    {
        if (!state_ || x < 0 || z < 0 || x >= state_->statistics.sizeX || z >= state_->statistics.sizeZ)
            return false;
        const auto index = std::uint64_t(z)*state_->statistics.sizeX+std::uint64_t(x);
        std::memcpy(&slot, state_->bytes+state_->slotOffset+index*sizeof(DetailSlot), sizeof(slot));
        return true;
    }

    bool DetailsData::Load(const std::filesystem::path& path, std::string& error)
    {
        error.clear();
        const auto fail = [&](const char* message)
        { error = "level.details: "+std::string(message)+" ("+path.string()+")"; return false; };
        try
        {
            auto next = std::make_unique<State>();
            next->file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (next->file == INVALID_HANDLE_VALUE) return fail("unable to open file");
            LARGE_INTEGER fileSize{};
            if (!GetFileSizeEx(next->file, &fileSize) || fileSize.QuadPart < 8 ||
                fileSize.QuadPart > 1024ll*1024*1024) return fail("invalid or excessive file size");
            next->statistics.mappedBytes = static_cast<std::uint64_t>(fileSize.QuadPart);
            next->mapping = CreateFileMappingW(next->file, nullptr, PAGE_READONLY, 0, 0, nullptr);
            if (!next->mapping) return fail("unable to create mapping");
            next->bytes = static_cast<const std::byte*>(MapViewOfFile(next->mapping, FILE_MAP_READ, 0, 0, 0));
            if (!next->bytes) return fail("unable to map file");
            std::array<Span, 3> chunks{};
            std::uint64_t position = 0;
            while (position < next->statistics.mappedBytes)
            {
                if (next->statistics.mappedBytes-position < 8) return fail("truncated chunk header");
                std::uint32_t id, size;
                std::memcpy(&id, next->bytes+position, 4);
                std::memcpy(&size, next->bytes+position+4, 4);
                position += 8;
                if (size > next->statistics.mappedBytes-position) return fail("chunk exceeds file");
                const auto rawId = id & 0x7fffffffu;
                if (rawId < chunks.size())
                {
                    if ((id & 0x80000000u) != 0) return fail("compressed detail chunk is unsupported");
                    if (chunks[rawId].found) return fail("duplicate detail chunk");
                    chunks[rawId] = {position, size, true};
                }
                position += size;
            }
            if (!chunks[0].found || !chunks[1].found || !chunks[2].found || chunks[0].size != 24)
                return fail("missing or invalid header/models/slots chunks");
            Cursor header(next->bytes+chunks[0].offset, chunks[0].size);
            auto& stats = next->statistics;
            std::uint32_t modelCount;
            if (!header.Read(stats.version) || !header.Read(modelCount) || !header.Read(stats.offsetX) ||
                !header.Read(stats.offsetZ) || !header.Read(stats.sizeX) || !header.Read(stats.sizeZ))
                return fail("truncated header");
            if (stats.version != 3) return fail("unsupported detail version (expected 3)");
            stats.slots = std::uint64_t(stats.sizeX)*stats.sizeZ;
            if (modelCount > 63 || stats.slots > 64ull*1024*1024 ||
                stats.slots*sizeof(DetailSlot) != chunks[2].size)
                return fail("invalid model count or slot grid size");
            stats.models = modelCount;
            next->slotOffset = chunks[2].offset;
            next->models.resize(modelCount);
            std::array<bool, 63> foundModels{};
            position = 0;
            const auto& modelChunk = chunks[1];
            std::uint64_t totalVertices = 0, totalIndices = 0;
            while (position < modelChunk.size)
            {
                if (modelChunk.size-position < 8) return fail("truncated model chunk");
                std::uint32_t id, size;
                const auto* bytes = next->bytes+modelChunk.offset+position;
                std::memcpy(&id, bytes, 4); std::memcpy(&size, bytes+4, 4);
                position += 8;
                if (id >= modelCount || foundModels[id] || size > modelChunk.size-position)
                    return fail("invalid/duplicate model ID or size");
                foundModels[id] = true;
                auto& model = next->models[id];
                Cursor data(next->bytes+modelChunk.offset+position, size);
                std::uint32_t vertexCount, indexCount;
                if (!data.String(model.shader) || !data.String(model.texture) || !data.Read(model.flags) ||
                    !data.Read(model.minimumScale) || !data.Read(model.maximumScale) ||
                    !data.Read(vertexCount) || !data.Read(indexCount)) return fail("truncated model data");
                totalVertices += vertexCount; totalIndices += indexCount;
                if (!std::isfinite(model.minimumScale) || !std::isfinite(model.maximumScale) ||
                    model.minimumScale <= 0 || model.maximumScale < model.minimumScale || model.maximumScale > 100 ||
                    vertexCount == 0 || vertexCount > 65535 || indexCount == 0 || indexCount%3 != 0 ||
                    totalVertices > 1024*1024 || totalIndices > 3*1024*1024 ||
                    data.Remaining() != std::uint64_t(vertexCount)*20+std::uint64_t(indexCount)*2)
                    return fail("invalid or excessive model geometry/scales");
                if (!data.Array(model.vertices, vertexCount) || !data.Array(model.indices, indexCount))
                    return fail("truncated geometry");
                model.minimum = model.maximum = model.vertices.front().position;
                for (const auto& vertex : model.vertices)
                {
                    if (!Finite(vertex.position) || !std::isfinite(vertex.u) || !std::isfinite(vertex.v) ||
                        std::abs(vertex.position.x) > 1000 || std::abs(vertex.position.y) > 1000 ||
                        std::abs(vertex.position.z) > 1000) return fail("invalid model vertex");
                    model.minimum.x = std::min(model.minimum.x, vertex.position.x);
                    model.minimum.y = std::min(model.minimum.y, vertex.position.y);
                    model.minimum.z = std::min(model.minimum.z, vertex.position.z);
                    model.maximum.x = std::max(model.maximum.x, vertex.position.x);
                    model.maximum.y = std::max(model.maximum.y, vertex.position.y);
                    model.maximum.z = std::max(model.maximum.z, vertex.position.z);
                }
                for (auto index : model.indices) if (index >= vertexCount) return fail("model index exceeds vertices");
                position += size;
            }
            for (std::uint32_t i = 0; i < modelCount; ++i)
                if (!foundModels[i]) return fail("missing model chunk");
            for (std::uint64_t i = 0; i < stats.slots; ++i)
            {
                DetailSlot slot;
                std::memcpy(&slot, next->bytes+next->slotOffset+i*16, 16);
                bool populated = false;
                for (std::size_t part = 0; part < 4; ++part)
                {
                    const auto id = slot.Model(part);
                    if (id != 63 && id >= modelCount) return fail("slot references an unavailable model");
                    populated |= id != 63 && slot.palettes[part] != 0;
                }
                stats.populatedSlots += populated ? 1 : 0;
            }
            state_ = std::move(next);
            return true;
        }
        catch (const std::bad_alloc&) { return fail("insufficient memory"); }
    }
}
