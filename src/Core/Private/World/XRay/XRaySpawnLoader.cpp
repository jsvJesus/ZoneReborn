#include "Core/World/XRay/XRaySpawnLoader.h"

#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <new>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace core::world::xray
{
    namespace
    {
        class PacketReader
        {
        public:
            PacketReader(const std::vector<std::byte>& bytes, std::size_t start, std::size_t size)
                : bytes_(bytes), position_(start), end_(start+size) {}
            std::size_t Position() const noexcept { return position_; }
            std::size_t Remaining() const noexcept { return end_-position_; }
            void Skip(std::size_t size)
            {
                if (size > Remaining()) throw std::runtime_error("truncated packet field");
                position_ += size;
            }
            template<class T> T Read()
            {
                if (sizeof(T) > Remaining()) throw std::runtime_error("truncated packet field");
                T result;
                std::memcpy(&result, bytes_.data()+position_, sizeof(T));
                position_ += sizeof(T);
                return result;
            }
            float Float()
            {
                const auto value = Read<float>();
                if (!std::isfinite(value)) throw std::runtime_error("non-finite parameter");
                return value;
            }
            math::Vector3 Vector() { return {Float(),Float(),Float()}; }
            std::string String()
            {
                const auto begin = position_;
                while (position_ < end_ && bytes_[position_] != std::byte{0}) ++position_;
                if (position_ == end_) throw std::runtime_error("unterminated string");
                const auto* text = reinterpret_cast<const char*>(bytes_.data()+begin);
                const std::string result(text,position_-begin);
                ++position_;
                return result;
            }
        private:
            const std::vector<std::byte>& bytes_;
            std::size_t position_, end_;
        };

        template<class T> std::string Text(const T& value)
        {
            std::ostringstream stream;
            stream << std::setprecision(9) << value;
            return stream.str();
        }
        std::string Text(const math::Vector3& p)
        { return Text(p.x)+", "+Text(p.y)+", "+Text(p.z); }
        template<class T> void Property(SpawnObject& object, const std::string& name, const T& value)
        { object.parameters.emplace_back(name,Text(value)); }

        void ReadAlife(PacketReader& state, SpawnObject& object)
        {
            Property(object,"Graph vertex",state.Read<std::uint16_t>());
            Property(object,"Graph distance",state.Float());
            Property(object,"Direct control",state.Read<std::uint32_t>());
            Property(object,"AI node",state.Read<std::uint32_t>());
            Property(object,"ALife flags",state.Read<std::uint32_t>());
            object.customData = state.String();
            Property(object,"Story ID",state.Read<std::uint32_t>());
            Property(object,"Spawn story ID",state.Read<std::uint32_t>());
        }
        void ReadVisual(PacketReader& state, SpawnObject& object)
        {
            object.visual = state.String();
            Property(object,"Visual flags",unsigned(state.Read<std::uint8_t>()));
        }
        bool ReadSkeleton(PacketReader& state, SpawnObject& object)
        {
            Property(object,"Skeleton startup animation",state.String());
            const auto flags = state.Read<std::uint8_t>();
            Property(object,"Skeleton flags",unsigned(flags));
            Property(object,"Skeleton source ID",state.Read<std::uint16_t>());
            // Saved physics bones have their own serialization: retain them opaque.
            return (flags & 4u) == 0;
        }
        void ReadShapes(PacketReader& state, SpawnObject& object)
        {
            const auto count = state.Read<std::uint8_t>();
            Property(object,"Shape count",unsigned(count));
            for (unsigned i = 0; i < count; ++i)
            {
                const auto prefix = "Shape " + Text(i) + " ";
                const auto type = state.Read<std::uint8_t>();
                if (type == 0)
                {
                    Property(object,prefix+"sphere centre",state.Vector());
                    const float radius = state.Float();
                    if (radius < 0.0f) throw std::runtime_error("negative shape radius");
                    Property(object,prefix+"sphere radius",radius);
                }
                else if (type == 1)
                {
                    for (const auto* axis : {"box i","box j","box k","box centre"})
                        Property(object,prefix+axis,state.Vector());
                }
                else throw std::runtime_error("unsupported shape type");
            }
        }

        void DecodeState(SpawnObject& object)
        {
            // Only layouts explicitly identified here are decoded. Custom sections
            // and older/newer STATE layouts retain their complete native bytes.
            if (object.version != 128) return;
            const auto& section = object.section;
            const bool graph = section == "graph_point";
            const bool breakable = section == "breakable_object";
            const bool physical = section == "physic_object" || section == "physic_destroyable_object";
            const bool lamp = section == "lights_hanging_lamp";
            const bool inventory = section == "inventory_box";
            const bool restrictor = section == "space_restrictor";
            const bool changer = section == "level_changer";
            const bool climbable = section == "climable_object";
            if (!(graph || breakable || physical || lamp || inventory || restrictor || changer || climbable)) return;

            PacketReader state(object.packet,object.stateOffset,object.stateSize);
            bool completeLayout = true;
            if (graph)
            {
                Property(object,"Connection point",state.String());
                Property(object,"Connection level",state.String());
                for (unsigned i = 0; i < 4; ++i)
                    Property(object,"Location " + Text(i),unsigned(state.Read<std::uint8_t>()));
            }
            else
            {
                ReadAlife(state,object);
                if (breakable || physical || lamp || inventory) ReadVisual(state,object);
                bool skeletonKnown = true;
                if (physical || lamp) skeletonKnown = ReadSkeleton(state,object);
                if (!skeletonKnown)
                {
                    if (state.Remaining() == 0) throw std::runtime_error("missing saved physics STATE data");
                    completeLayout = false;
                }
                if (skeletonKnown)
                {
                    if (breakable) Property(object,"Health",state.Float());
                    if (physical)
                    {
                        Property(object,"Physics type",state.Read<std::uint32_t>());
                        Property(object,"Mass",state.Float());
                        Property(object,"Fixed bones",state.String());
                    }
                    if (inventory)
                    {
                        Property(object,"Can take",unsigned(state.Read<std::uint8_t>()));
                        Property(object,"Closed",unsigned(state.Read<std::uint8_t>()));
                        Property(object,"Tip text",state.String());
                    }
                    if (lamp)
                    {
                        Property(object,"Light ARGB",state.Read<std::uint32_t>());
                        Property(object,"Brightness",state.Float());
                        Property(object,"Colour animator",state.String());
                        Property(object,"Range",state.Float());
                        Property(object,"Light flags",state.Read<std::uint16_t>());
                        Property(object,"Startup animation",state.String());
                        Property(object,"Fixed bones",state.String());
                        Property(object,"Health",state.Float());
                        Property(object,"Virtual size",state.Float());
                        Property(object,"Ambient radius",state.Float());
                        Property(object,"Ambient power",state.Float());
                        for (const auto* name : {"Ambient texture","Light texture","Main bone"})
                            Property(object,name,state.String());
                        Property(object,"Spot cone radians",state.Float());
                        Property(object,"Glow texture",state.String());
                        Property(object,"Glow radius",state.Float());
                        Property(object,"Ambient bone",state.String());
                        for (const auto* name : {"Volumetric quality","Volumetric intensity","Volumetric distance"})
                            Property(object,name,state.Float());
                    }
                    if (restrictor || changer || climbable)
                    {
                        ReadShapes(state,object);
                        if (climbable) Property(object,"Climb material",state.String());
                        else Property(object,"Restrictor type",unsigned(state.Read<std::uint8_t>()));
                    }
                    if (changer)
                    {
                        Property(object,"Next graph vertex",state.Read<std::uint16_t>());
                        Property(object,"Next AI node",state.Read<std::uint32_t>());
                        Property(object,"Next position",state.Vector());
                        Property(object,"Next angles radians",state.Vector());
                        Property(object,"Target level",state.String());
                        Property(object,"Target point",state.String());
                        Property(object,"Silent mode",unsigned(state.Read<std::uint8_t>()));
                    }
                }
            }
            object.decodedStateBytes = state.Position()-object.stateOffset;
            object.stateDecoding = completeLayout && state.Remaining() == 0
                ? SpawnStateDecoding::Complete : SpawnStateDecoding::Partial;
        }

        void DecodePacket(SpawnObject& object)
        {
            PacketReader packet(object.packet,0,object.packet.size());
            if (packet.Read<std::uint16_t>() != 1) throw std::runtime_error("packet is not M_SPAWN");
            object.section = packet.String();
            if (object.section.empty()) throw std::runtime_error("empty object section");
            object.name = packet.String();
            object.gameId = packet.Read<std::uint8_t>();
            object.respawnPoint = packet.Read<std::uint8_t>();
            object.position = packet.Vector();
            object.rotation = packet.Vector();
            object.respawnTime = packet.Read<std::uint16_t>();
            object.id = packet.Read<std::uint16_t>();
            object.parentId = packet.Read<std::uint16_t>();
            object.phantomId = packet.Read<std::uint16_t>();
            object.flags = packet.Read<std::uint16_t>();
            if ((object.flags & 0x20u) == 0) throw std::runtime_error("missing spawn version flag");
            object.version = packet.Read<std::uint16_t>();
            if (object.version == 0 || object.version > 128) throw std::runtime_error("unsupported common packet version");
            if (object.version > 120) object.gameTypeMask = packet.Read<std::uint16_t>();
            if (object.version > 69) object.scriptVersion = packet.Read<std::uint16_t>();
            if (object.version > 70)
            {
                object.clientDataSize = object.version > 93
                    ? packet.Read<std::uint16_t>() : packet.Read<std::uint8_t>();
                object.clientDataOffset = packet.Position();
                packet.Skip(object.clientDataSize);
            }
            if (object.version > 79) object.spawnId = packet.Read<std::uint16_t>();
            if (object.version < 112)
            {
                if (object.version > 82) packet.Float();
                if (object.version > 83)
                {
                    packet.Read<std::uint32_t>(); packet.String();
                    packet.Read<std::uint32_t>(); packet.Read<std::uint32_t>(); packet.Read<std::uint64_t>();
                }
                if (object.version > 84) { packet.Read<std::uint64_t>(); packet.Read<std::uint64_t>(); }
            }
            const auto stateSize = packet.Read<std::uint16_t>();
            if (stateSize < 2) throw std::runtime_error("invalid STATE size");
            object.stateOffset = packet.Position();
            object.stateSize = stateSize-2; // Native size includes its own u16.
            packet.Skip(object.stateSize);
            DecodeState(object);
        }
    }

    bool SpawnLoader::Load(const std::filesystem::path& path, SpawnData& output, std::string& error) const
    {
        error.clear();
        std::uint32_t chunkId = 0;
        try
        {
            std::ifstream file(path,std::ios::binary | std::ios::ate);
            if (!file) throw std::runtime_error("unable to open file");
            const auto size = file.tellg();
            if (size < 0) throw std::runtime_error("unable to determine file size");
            SpawnData candidate;
            candidate.fileBytes = static_cast<std::uint64_t>(size);
            file.seekg(0,std::ios::beg);
            std::uint64_t cursor = 0;
            std::unordered_set<std::uint32_t> chunks;
            std::unordered_set<std::string> sections;
            while (cursor < candidate.fileBytes)
            {
                if (candidate.fileBytes-cursor < 8) throw std::runtime_error("truncated chunk header");
                std::array<std::uint32_t,2> header{};
                if (!file.read(reinterpret_cast<char*>(header.data()),8)) throw std::runtime_error("unable to read chunk header");
                chunkId = header[0];
                const auto packetBytes = header[1];
                cursor += 8;
                if ((chunkId & 0x80000000u) != 0) throw std::runtime_error("compressed spawn chunk is unsupported");
                if (!chunks.insert(chunkId).second) throw std::runtime_error("duplicate chunk ID");
                if (packetBytes > candidate.fileBytes-cursor) throw std::runtime_error("truncated spawn chunk");
                // Bound each allocation independently of an untrusted file header.
                if (packetBytes < 2 || packetBytes > 128u*1024u) throw std::runtime_error("invalid spawn packet size");
                SpawnObject object;
                object.chunkId = chunkId;
                object.packet.resize(packetBytes);
                if (!file.read(reinterpret_cast<char*>(object.packet.data()),packetBytes)) throw std::runtime_error("unable to read spawn packet");
                DecodePacket(object);
                sections.insert(object.section);
                if (object.stateDecoding == SpawnStateDecoding::Complete) ++candidate.decodedStates;
                if (object.stateDecoding == SpawnStateDecoding::Partial) ++candidate.partialStates;
                candidate.objects.push_back(std::move(object));
                cursor += packetBytes;
            }
            candidate.sectionCount = sections.size();
            output = std::move(candidate);
            return true;
        }
        catch (const std::bad_alloc&) { error = "level.spawn: insufficient memory."; }
        catch (const std::exception& failure)
        { error = "level.spawn chunk " + Text(chunkId) + ": " + failure.what() + "."; }
        return false;
    }
}
