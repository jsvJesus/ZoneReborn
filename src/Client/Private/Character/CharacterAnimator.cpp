#include "Character/CharacterAnimator.h"

#include "Graphics/Renderer.h"

#include "Core/Log.h"
#include "Core/Math/Transform3x4.h"
#include "Core/Math/Vector3.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    using Transform =
        core::math::Transform3x4;

    using Vector3 =
        core::math::Vector3;

    constexpr char IdleAnimationPath[] =
        "res/characters2/basemodel/animations/unarmed/"
        "idle_unarmed/idle_move_stay_unarmed.animation";

    constexpr float IdleFrameRate =
        22.0f;
    
    struct Quaternion final
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;
    };
    
    template<typename T>
    struct Key final
    {
        float time =
            0.0f;

        T value{};
    };

    struct AnimationChannel final
    {
        std::string
            identifier;

        std::vector<Key<Vector3>>
            scaleKeys;

        std::vector<Key<Vector3>>
            positionKeys;

        std::vector<Key<Quaternion>>
            rotationKeys;

        std::vector<Key<Transform>>
            discreteKeys;
    };

    struct AnimationClip final
    {
        float totalFrames =
            0.0f;

        std::string identifier;
        std::string internalIdentifier;

        std::vector<AnimationChannel>
            channels;

        std::unordered_map<
            std::string,
            std::size_t>
            lookup;
    };
    
    class BinaryReader final
    {
    public:
        explicit BinaryReader(
            const std::vector<std::byte>& data)
            : data_(
                data)
        {
        }

        template<typename T>
        [[nodiscard]]
        bool Read(
            T& output)
        {
            static_assert(
                std::is_trivially_copyable_v<T>);

            if (offset_ +
                    sizeof(T) >
                data_.size())
            {
                return false;
            }

            std::memcpy(
                &output,
                data_.data() +
                    offset_,
                sizeof(T));

            offset_ +=
                sizeof(T);

            return true;
        }
        
        [[nodiscard]]
        bool ReadString(
            std::string& output)
        {
            output.clear();

            std::int32_t length =
                0;

            if (!Read(
                    length))
            {
                return false;
            }

            if (length <
                0)
            {
                return false;
            }

            const std::size_t size =
                static_cast<std::size_t>(
                    length);

            if (offset_ +
                    size >
                data_.size())
            {
                return false;
            }

            output.assign(
                reinterpret_cast<const char*>(
                    data_.data() +
                    offset_),
                size);

            offset_ +=
                size;

            return true;
        }

        [[nodiscard]]
        bool Skip(
            const std::size_t size)
        {
            if (offset_ +
                    size >
                data_.size())
            {
                return false;
            }

            offset_ +=
                size;

            return true;
        }


        [[nodiscard]]
        std::size_t Remaining() const noexcept
        {
            return
                data_.size() -
                offset_;
        }

    private:
        const std::vector<std::byte>&
            data_;

        std::size_t offset_ =
            0;
    };

    [[nodiscard]]
    bool ReadVector3(
        BinaryReader& reader,
        Vector3& output)
    {
        return
            reader.Read(
                output.x) &&
            reader.Read(
                output.y) &&
            reader.Read(
                output.z);
    }
    
    [[nodiscard]]
    bool ReadQuaternion(
        BinaryReader& reader,
        Quaternion& output)
    {
        return
            reader.Read(
                output.x) &&
            reader.Read(
                output.y) &&
            reader.Read(
                output.z) &&
            reader.Read(
                output.w);
    }

    template<typename TValue, typename ReaderFunction>
    [[nodiscard]]
    bool ReadKeyVector(
        BinaryReader& reader,
        std::vector<Key<TValue>>& output,
        ReaderFunction readValue)
    {
        std::uint32_t count =
            0;

        if (!reader.Read(
                count))
        {
            return false;
        }

        if (count >
            1000000u)
        {
            return false;
        }

        output.clear();

        output.reserve(
            count);

        for (std::uint32_t index = 0;
             index < count;
             ++index)
        {
            Key<TValue>
                key;

            if (!reader.Read(
                    key.time))
            {
                return false;
            }

            if (!readValue(
                    reader,
                    key.value))
            {
                return false;
            }

            output.push_back(
                std::move(
                    key));
        }

        return true;
    }

    [[nodiscard]]
    bool SkipIndexVector(
        BinaryReader& reader)
    {
        std::uint32_t count =
            0;

        if (!reader.Read(
                count))
        {
            return false;
        }

        if (count >
            reader.Remaining() /
                sizeof(std::uint32_t))
        {
            return false;
        }

        return
            reader.Skip(
                static_cast<std::size_t>(
                    count) *
                sizeof(std::uint32_t));
    }
    
    [[nodiscard]]
    Transform Matrix4x4ToTransform(
        const float* values) noexcept
    {
        Transform result;

        result.values[0] =
            values[0];

        result.values[1] =
            values[1];

        result.values[2] =
            values[2];
        
        result.values[3] =
            values[4];

        result.values[4] =
            values[5];

        result.values[5] =
            values[6];

        result.values[6] =
            values[8];

        result.values[7] =
            values[9];

        result.values[8] =
            values[10];


        result.values[9] =
            values[12];

        result.values[10] =
            values[13];

        result.values[11] =
            values[14];

        return result;
    }

    [[nodiscard]]
    bool ReadDiscreteChannel(
        BinaryReader& reader,
        AnimationChannel& channel)
    {
        if (!reader.ReadString(
                channel.identifier))
        {
            return false;
        }

        std::uint32_t count =
            0;

        if (!reader.Read(
                count))
        {
            return false;
        }

        if (count >
            100000u)
        {
            return false;
        }

        channel.discreteKeys.reserve(
            count);

        for (std::uint32_t index = 0;
             index < count;
             ++index)
        {
            Key<Transform>
                key;

            float matrix[16]{};

            if (!reader.Read(
                    key.time))
            {
                return false;
            }

            for (float& value :
                 matrix)
            {
                if (!reader.Read(
                        value))
                {
                    return false;
                }
            }

            key.value =
                Matrix4x4ToTransform(
                    matrix);

            channel.discreteKeys.push_back(
                key);
        }

        return true;
    }

    [[nodiscard]]
    bool ReadInterpolatedChannel(
        BinaryReader& reader,
        const std::int32_t type,
        AnimationChannel& channel)
    {
        if (!reader.ReadString(
                channel.identifier))
        {
            return false;
        }

        //
        // Type 4 stores compression factors.
        //
        if (type ==
            4)
        {
            float scaleError =
                0.0f;

            float positionError =
                0.0f;

            float rotationError =
                0.0f;

            if (!reader.Read(
                    scaleError) ||
                !reader.Read(
                    positionError) ||
                !reader.Read(
                    rotationError))
            {
                return false;
            }
        }

        if (!ReadKeyVector<Vector3>(
                reader,
                channel.scaleKeys,
                [](
                    BinaryReader& stream,
                    Vector3& value)
                {
                    return
                        ReadVector3(
                            stream,
                            value);
                }))
        {
            return false;
        }

        if (!ReadKeyVector<Vector3>(
                reader,
                channel.positionKeys,
                [](
                    BinaryReader& stream,
                    Vector3& value)
                {
                    return
                        ReadVector3(
                            stream,
                            value);
                }))
        {
            return false;
        }

        if (!ReadKeyVector<Quaternion>(
                reader,
                channel.rotationKeys,
                [](
                    BinaryReader& stream,
                    Quaternion& value)
                {
                    return
                        ReadQuaternion(
                            stream,
                            value);
                }))
        {
            return false;
        }

        //
        // Original BigWorld files also save 3 index vectors.
        // Runtime can reconstruct interpolation directly from
        // the key timestamps, so we only skip them.
        //
        return
            SkipIndexVector(
                reader) &&
            SkipIndexVector(
                reader) &&
            SkipIndexVector(
                reader);
    }
    
    [[nodiscard]]
    bool SkipMorphChannel(
        BinaryReader& reader)
    {
        std::string identifier;

        if (!reader.ReadString(
                identifier))
        {
            return false;
        }

        std::uint32_t count =
            0;

        if (!reader.Read(
                count))
        {
            return false;
        }

        if (count >
            reader.Remaining() /
                sizeof(float))
        {
            return false;
        }

        return
            reader.Skip(
                static_cast<std::size_t>(
                    count) *
                sizeof(float));
    }

    [[nodiscard]]
    bool SkipCueChannel(
        BinaryReader& reader)
    {
        std::uint32_t count =
            0;

        if (!reader.Read(
                count))
        {
            return false;
        }

        for (std::uint32_t index = 0;
             index < count;
             ++index)
        {
            float time =
                0.0f;

            std::string cue;

            if (!reader.Read(
                    time) ||
                !reader.ReadString(
                    cue))
            {
                return false;
            }

            std::uint32_t argumentCount =
                0;

            if (!reader.Read(
                    argumentCount))
            {
                return false;
            }

            if (argumentCount >
                reader.Remaining() /
                    sizeof(float))
            {
                return false;
            }

            if (!reader.Skip(
                    static_cast<std::size_t>(
                        argumentCount) *
                    sizeof(float)))
            {
                return false;
            }
        }

        return true;
    }
    
    [[nodiscard]]
    bool ReadStreamedFallback(
        BinaryReader& reader,
        AnimationChannel& channel)
    {
        if (!reader.ReadString(
                channel.identifier))
        {
            return false;
        }

        Vector3 scale;
        Vector3 position;

        Quaternion rotation;

        if (!ReadVector3(
                reader,
                scale) ||
            !ReadVector3(
                reader,
                position) ||
            !ReadQuaternion(
                reader,
                rotation))
        {
            return false;
        }

        channel.scaleKeys.push_back(
            {
                0.0f,
                scale
            });

        channel.positionKeys.push_back(
            {
                0.0f,
                position
            });

        channel.rotationKeys.push_back(
            {
                0.0f,
                rotation
            });

        return true;
    }
    
    [[nodiscard]]
    bool SkipTranslationOverride(
        BinaryReader& reader)
    {
        std::string identifier;
        std::string animationResource;

        if (!reader.ReadString(
                identifier) ||
            !reader.ReadString(
                animationResource))
        {
            return false;
        }

        Vector3 translation;

        if (!ReadVector3(
                reader,
                translation))
        {
            return false;
        }

        std::uint8_t overrideTranslation =
            0;

        return
            reader.Read(
                overrideTranslation);
    }

    [[nodiscard]]
    bool LoadAnimation(
        const core::resources::ResourceFileSystem& resources,
        AnimationClip& output,
        std::string& error)
    {
        output =
            {};

        std::vector<std::byte>
            bytes;

        if (!resources.ReadBinary(
                IdleAnimationPath,
                bytes))
        {
            error =
                std::string(
                    "Unable to read idle animation: ") +
                IdleAnimationPath;

            return false;
        }

        BinaryReader
            reader(
                bytes);

        if (!reader.Read(
                output.totalFrames) ||
            !reader.ReadString(
                output.identifier) ||
            !reader.ReadString(
                output.internalIdentifier))
        {
            error =
                "Invalid BigWorld animation header.";

            return false;
        }

        std::int32_t channelCount =
            0;

        if (!reader.Read(
                channelCount) ||
            channelCount <
                0 ||
            channelCount >
                10000)
        {
            error =
                "Invalid BigWorld animation channel count.";

            return false;
        }

        output.channels.reserve(
            static_cast<std::size_t>(
                channelCount));

        std::unordered_map<
            std::int32_t,
            std::uint32_t>
            typeCounts;

        for (std::int32_t channelIndex = 0;
             channelIndex < channelCount;
             ++channelIndex)
        {
            std::int32_t type =
                -1;

            if (!reader.Read(
                    type))
            {
                error =
                    "Unexpected EOF while reading animation channel type.";

                return false;
            }

            ++typeCounts[type];

            AnimationChannel
                channel;

            bool loaded =
                false;

            switch (type)
            {
                case 0:
                    loaded =
                        ReadDiscreteChannel(
                            reader,
                            channel);

                    break;

                case 1:
                case 3:
                case 4:
                    loaded =
                        ReadInterpolatedChannel(
                            reader,
                            type,
                            channel);

                    break;

                case 2:
                    loaded =
                        SkipMorphChannel(
                            reader);

                    break;

                case 5:
                    loaded =
                        ReadStreamedFallback(
                            reader,
                            channel);

                    break;

                case 6:
                    loaded =
                        SkipCueChannel(
                            reader);

                    break;

                case 7:
                    loaded =
                        SkipTranslationOverride(
                            reader);

                    break;

                default:
                    error =
                        "Unsupported BigWorld animation channel type: " +
                        std::to_string(
                            type);

                    return false;
            }

            if (!loaded)
            {
                error =
                    "Unable to parse animation channel #" +
                    std::to_string(
                        channelIndex) +
                    " type=" +
                    std::to_string(
                        type);

                return false;
            }

            if (!channel.identifier.empty())
            {
                output.lookup.emplace(
                    channel.identifier,
                    output.channels.size());

                output.channels.push_back(
                    std::move(
                        channel));
            }
        }

        std::string typeLog =
            "Character idle channel types:";

        for (const auto& [type, count] :
             typeCounts)
        {
            typeLog +=
                " " +
                std::to_string(
                    type) +
                "=" +
                std::to_string(
                    count);
        }

        core::Log::Info(
            typeLog);

        core::Log::Info(
            std::string(
                "Character idle loaded: frames=") +
            std::to_string(
                output.totalFrames) +
            ", channels=" +
            std::to_string(
                output.channels.size()) +
            ", fps=" +
            std::to_string(
                IdleFrameRate));

        return true;
    }
    
    Vector3 Lerp(
        const Vector3 a,
        const Vector3 b,
        const float t) noexcept
    {
        return
        {
            a.x +
                (b.x - a.x) *
                t,

            a.y +
                (b.y - a.y) *
                t,

            a.z +
                (b.z - a.z) *
                t
        };
    }
    
    Quaternion Normalize(
        Quaternion value) noexcept
    {
        const float lengthSquared =
            value.x * value.x +
            value.y * value.y +
            value.z * value.z +
            value.w * value.w;

        if (lengthSquared <=
            0.00000001f)
        {
            return {};
        }

        const float inverse =
            1.0f /
            std::sqrt(
                lengthSquared);

        value.x *= inverse;
        value.y *= inverse;
        value.z *= inverse;
        value.w *= inverse;

        return value;
    }
    
    Quaternion Slerp(
        Quaternion a,
        Quaternion b,
        const float t) noexcept
    {
        a =
            Normalize(
                a);

        b =
            Normalize(
                b);

        float dot =
            a.x * b.x +
            a.y * b.y +
            a.z * b.z +
            a.w * b.w;

        if (dot <
            0.0f)
        {
            dot =
                -dot;

            b.x =
                -b.x;

            b.y =
                -b.y;

            b.z =
                -b.z;

            b.w =
                -b.w;
        }

        if (dot >
            0.9995f)
        {
            Quaternion result
            {
                a.x +
                    (b.x - a.x) *
                    t,

                a.y +
                    (b.y - a.y) *
                    t,

                a.z +
                    (b.z - a.z) *
                    t,

                a.w +
                    (b.w - a.w) *
                    t
            };

            return
                Normalize(
                    result);
        }

        dot =
            std::clamp(
                dot,
                -1.0f,
                1.0f);

        const float theta =
            std::acos(
                dot);

        const float sine =
            std::sin(
                theta);

        if (std::abs(
                sine) <
            0.000001f)
        {
            return a;
        }

        const float weightA =
            std::sin(
                (
                    1.0f -
                    t
                ) *
                theta) /
            sine;

        const float weightB =
            std::sin(
                t *
                theta) /
            sine;

        return
        {
            a.x * weightA +
                b.x * weightB,

            a.y * weightA +
                b.y * weightB,

            a.z * weightA +
                b.z * weightB,

            a.w * weightA +
                b.w * weightB
        };
    }
    
    template<typename TValue,
             typename Interpolator>
    TValue SampleKeys(
        const std::vector<Key<TValue>>& keys,
        const float frame,
        const TValue& fallback,
        Interpolator interpolate)
    {
        if (keys.empty())
        {
            return fallback;
        }

        if (keys.size() ==
                1 ||
            frame <=
                keys.front().time)
        {
            return
                keys.front().value;
        }

        if (frame >=
            keys.back().time)
        {
            return
                keys.back().value;
        }

        const auto upper =
            std::upper_bound(
                keys.begin(),
                keys.end(),
                frame,
                [](
                    const float value,
                    const Key<TValue>& key)
                {
                    return
                        value <
                        key.time;
                });

        if (upper ==
            keys.begin())
        {
            return
                upper->value;
        }

        const auto lower =
            upper -
            1;

        const float duration =
            upper->time -
            lower->time;

        if (duration <=
            0.000001f)
        {
            return
                lower->value;
        }

        const float t =
            (
                frame -
                lower->time
            ) /
            duration;

        return
            interpolate(
                lower->value,
                upper->value,
                t);
    }
    
    Transform BuildTransform(
        const Quaternion rotation,
        const Vector3 scale,
        const Vector3 position) noexcept
    {
        const Quaternion q =
            Normalize(
                rotation);

        const float xx =
            q.x * q.x;

        const float yy =
            q.y * q.y;

        const float zz =
            q.z * q.z;

        const float xy =
            q.x * q.y;

        const float xz =
            q.x * q.z;

        const float yz =
            q.y * q.z;

        const float xw =
            q.x * q.w;

        const float yw =
            q.y * q.w;

        const float zw =
            q.z * q.w;
        
        Transform result;

        result.values[0] =
            (
                1.0f -
                2.0f *
                    (
                        yy +
                        zz
                    )
            ) *
            scale.x;

        result.values[1] =
            (
                2.0f *
                (
                    xy +
                    zw
                )
            ) *
            scale.x;

        result.values[2] =
            (
                2.0f *
                (
                    xz -
                    yw
                )
            ) *
            scale.x;

        result.values[3] =
            (
                2.0f *
                (
                    xy -
                    zw
                )
            ) *
            scale.y;

        result.values[4] =
            (
                1.0f -
                2.0f *
                    (
                        xx +
                        zz
                    )
            ) *
            scale.y;

        result.values[5] =
            (
                2.0f *
                (
                    yz +
                    xw
                )
            ) *
            scale.y;
        
        result.values[6] =
            (
                2.0f *
                (
                    xz +
                    yw
                )
            ) *
            scale.z;

        result.values[7] =
            (
                2.0f *
                (
                    yz -
                    xw
                )
            ) *
            scale.z;

        result.values[8] =
            (
                1.0f -
                2.0f *
                    (
                        xx +
                        yy
                    )
            ) *
            scale.z;
        
        result.values[9] =
            position.x;

        result.values[10] =
            position.y;

        result.values[11] =
            position.z;

        return result;
    }
    
    [[nodiscard]]
    bool SampleChannel(
        const AnimationClip& clip,
        const std::string& identifier,
        const float frame,
        Transform& output)
    {
        const auto found =
            clip.lookup.find(
                identifier);

        if (found ==
            clip.lookup.end())
        {
            return false;
        }

        const AnimationChannel& channel =
            clip.channels[
                found->second];

        if (!channel.discreteKeys.empty())
        {
            const auto upper =
                std::upper_bound(
                    channel.discreteKeys.begin(),
                    channel.discreteKeys.end(),
                    frame,
                    [](
                        const float value,
                        const Key<Transform>& key)
                    {
                        return
                            value <
                            key.time;
                    });

            if (upper ==
                channel.discreteKeys.begin())
            {
                output =
                    channel.discreteKeys.front().
                        value;
            }
            else
            {
                output =
                    (
                        upper -
                        1
                    )->value;
            }

            return true;
        }

        const Vector3 scale =
            SampleKeys<Vector3>(
                channel.scaleKeys,
                frame,
                {
                    1.0f,
                    1.0f,
                    1.0f
                },
                [](
                    const Vector3 a,
                    const Vector3 b,
                    const float t)
                {
                    return
                        Lerp(
                            a,
                            b,
                            t);
                });

        const Vector3 position =
            SampleKeys<Vector3>(
                channel.positionKeys,
                frame,
                {},
                [](
                    const Vector3 a,
                    const Vector3 b,
                    const float t)
                {
                    return
                        Lerp(
                            a,
                            b,
                            t);
                });

        const Quaternion rotation =
            SampleKeys<Quaternion>(
                channel.rotationKeys,
                frame,
                {},
                [](
                    const Quaternion a,
                    const Quaternion b,
                    const float t)
                {
                    return
                        Slerp(
                            a,
                            b,
                            t);
                });

        output =
            BuildTransform(
                rotation,
                scale,
                position);

        return true;
    }

    Vector3 Add(
        const Vector3 a,
        const Vector3 b) noexcept
    {
        return
        {
            a.x + b.x,
            a.y + b.y,
            a.z + b.z
        };
    }

    Vector3 Multiply(
        const Vector3 value,
        const float scalar) noexcept
    {
        return
        {
            value.x * scalar,
            value.y * scalar,
            value.z * scalar
        };
    }

    Vector3 NormalizeVector(
        const Vector3 value) noexcept
    {
        const float lengthSquared =
            value.x * value.x +
            value.y * value.y +
            value.z * value.z;

        if (lengthSquared <=
            0.0000001f)
        {
            return
            {
                0.0f,
                1.0f,
                0.0f
            };
        }

        const float inverse =
            1.0f /
            std::sqrt(
                lengthSquared);

        return
        {
            value.x * inverse,
            value.y * inverse,
            value.z * inverse
        };
    }

    Vector3 TransformPoint(
        const Vector3 value,
        const Transform& transform) noexcept
    {
        return
        {
            value.x * transform.values[0] +
            value.y * transform.values[3] +
            value.z * transform.values[6] +
            transform.values[9],

            value.x * transform.values[1] +
            value.y * transform.values[4] +
            value.z * transform.values[7] +
            transform.values[10],

            value.x * transform.values[2] +
            value.y * transform.values[5] +
            value.z * transform.values[8] +
            transform.values[11]
        };
    }

    Vector3 TransformVector(
        const Vector3 value,
        const Transform& transform) noexcept
    {
        return
        {
            value.x * transform.values[0] +
            value.y * transform.values[3] +
            value.z * transform.values[6],

            value.x * transform.values[1] +
            value.y * transform.values[4] +
            value.z * transform.values[7],

            value.x * transform.values[2] +
            value.y * transform.values[5] +
            value.z * transform.values[8]
        };
    }

    Vector3 UnpackNormal(
        const std::uint32_t packed) noexcept
    {
        std::int32_t x =
            static_cast<std::int32_t>(
                packed &
                0x7FFu);

        std::int32_t y =
            static_cast<std::int32_t>(
                (
                    packed >>
                    11u
                ) &
                0x7FFu);

        std::int32_t z =
            static_cast<std::int32_t>(
                (
                    packed >>
                    22u
                ) &
                0x3FFu);

        if ((x & 0x400) !=
            0)
        {
            x -=
                0x800;
        }

        if ((y & 0x400) !=
            0)
        {
            y -=
                0x800;
        }

        if ((z & 0x200) !=
            0)
        {
            z -=
                0x400;
        }

        return
            NormalizeVector(
            {
                static_cast<float>(
                    x) /
                    1023.0f,

                static_cast<float>(
                    y) /
                    1023.0f,

                static_cast<float>(
                    z) /
                    511.0f
            });
    }
    
    std::uint32_t PackNormal(
        const Vector3 value) noexcept
    {
        const Vector3 normal =
            NormalizeVector(
                value);

        const std::int32_t x =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.x,
                    -1.0f,
                    1.0f) *
                1023.0f);

        const std::int32_t y =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.y,
                    -1.0f,
                    1.0f) *
                1023.0f);

        const std::int32_t z =
            static_cast<std::int32_t>(
                std::clamp(
                    normal.z,
                    -1.0f,
                    1.0f) *
                511.0f);

        return
            (
                static_cast<std::uint32_t>(
                    x) &
                0x7FFu
            ) |
            (
                (
                    static_cast<std::uint32_t>(
                        y) &
                    0x7FFu
                ) <<
                11u
            ) |
            (
                (
                    static_cast<std::uint32_t>(
                        z) &
                    0x3FFu
                ) <<
                22u
            );
    }

    [[nodiscard]]
    bool BuildAnimatedNodeTransforms(
        const AnimationClip& clip,
        const core::assets::VisualAsset& visual,
        const float frame,
        std::vector<Transform>& output,
        std::string& error)
    {
        output.clear();

        output.resize(
            visual.nodes.size());

        for (std::size_t index = 0;
             index < visual.nodes.size();
             ++index)
        {
            const auto& node =
                visual.nodes[index];

            Transform local =
                node.transform;

            SampleChannel(
                clip,
                node.identifier,
                frame,
                local);

            if (node.parentIndex <
                0)
            {
                output[index] =
                    local;

                continue;
            }

            const std::size_t parent =
                static_cast<std::size_t>(
                    node.parentIndex);

            if (parent >=
                    index ||
                parent >=
                    output.size())
            {
                error =
                    "Animated character contains invalid node hierarchy.";

                return false;
            }

            output[index] =
                Transform::Multiply(
                    local,
                    output[parent]);
        }

        return true;
    }
    
    [[nodiscard]]
    bool BuildPalette(
        const core::assets::VisualAsset& visual,
        const std::vector<std::string>& paletteNodes,
        const std::vector<Transform>& nodeTransforms,
        std::vector<Transform>& palette,
        std::string& error)
    {
        palette.clear();

        std::unordered_map<
            std::string,
            std::size_t>
            lookup;

        lookup.reserve(
            visual.nodes.size());

        for (std::size_t index = 0;
             index < visual.nodes.size();
             ++index)
        {
            lookup.emplace(
                visual.nodes[index].
                    identifier,
                index);
        }

        palette.reserve(
            paletteNodes.size());

        for (const std::string& name :
             paletteNodes)
        {
            const auto found =
                lookup.find(
                    name);

            if (found ==
                lookup.end())
            {
                error =
                    "Animated character bone not found: " +
                    name;

                return false;
            }

            palette.push_back(
                nodeTransforms[
                    found->second]);
        }

        return true;
    }
    
    [[nodiscard]]
    bool SkinMesh(
        const core::assets::MeshData& source,
        core::assets::MeshData& output,
        const std::vector<Transform>& skinPalette,
        std::string& error)
    {
        if (!source.skinned)
        {
            return true;
        }

        if (source.vertices.size() !=
            output.vertices.size())
        {
            error =
                "Animated character mesh vertex count mismatch.";

            return false;
        }

        for (std::size_t vertexIndex = 0;
             vertexIndex < source.vertices.size();
             ++vertexIndex)
        {
            const core::assets::MeshVertex& sourceVertex =
                source.vertices[vertexIndex];

            core::assets::MeshVertex& destination =
                output.vertices[vertexIndex];

            Vector3 finalPosition{};
            Vector3 finalNormal{};

            const Vector3 sourceNormal =
                UnpackNormal(
                    sourceVertex.packedNormal);

            float totalWeight =
                0.0f;

            for (std::size_t influence = 0;
                 influence < sourceVertex.boneWeights.size();
                 ++influence)
            {
                const float weight =
                    sourceVertex.boneWeights[
                        influence];

                if (std::abs(weight) <= 0.000001f)
                {
                    continue;
                }

                const std::size_t boneIndex =
                    sourceVertex.boneIndices[
                        influence];

                if (boneIndex >= skinPalette.size())
                {
                    error =
                        "Animated character contains invalid bone index.";

                    return false;
                }

                const Transform& skin =
                    skinPalette[
                        boneIndex];

                finalPosition =
                    Add(
                        finalPosition,
                        Multiply(
                            TransformPoint(
                                sourceVertex.position,
                                skin),
                            weight));

                finalNormal =
                    Add(
                        finalNormal,
                        Multiply(
                            TransformVector(
                                sourceNormal,
                                skin),
                            weight));

                totalWeight +=
                    weight;
            }

            if (totalWeight <= 0.000001f)
            {
                destination.position =
                    sourceVertex.position;

                destination.packedNormal =
                    sourceVertex.packedNormal;

                continue;
            }

            if (std::abs(totalWeight - 1.0f) > 0.0001f)
            {
                const float invWeight =
                    1.0f / totalWeight;

                finalPosition =
                    Multiply(
                        finalPosition,
                        invWeight);

                finalNormal =
                    Multiply(
                        finalNormal,
                        invWeight);
            }

            destination.position =
                finalPosition;

            destination.packedNormal =
                PackNormal(
                    finalNormal);
        }

        output.skinned =
            false;

        return true;
    }
}

namespace client::character
{
    struct Animator::State final
    {
        struct MeshBinding final
        {
            std::size_t sceneMeshIndex =
                0;

            core::assets::VisualAsset
                visual;

            std::vector<std::string>
                paletteNodes;

            core::assets::MeshData
                sourceMesh;

            core::assets::MeshData
                outputMesh;
        };

        AnimationClip
            idle;

        std::vector<MeshBinding>
            meshes;

        bool ready =
            false;
    };

    Animator::Animator()
        : state_(
            std::make_unique<State>())
    {
    }
    
    Animator::~Animator() =
        default;

    void Animator::Reset()
    {
        state_ =
            std::make_unique<State>();
    }
    
    bool Animator::LoadIdle(
        const core::resources::ResourceFileSystem& resources,
        std::string& error)
    {
        error.clear();

        if (!LoadAnimation(
                resources,
                state_->idle,
                error))
        {
            return false;
        }

        state_->ready =
            true;

        core::Log::Info(
            std::string(
                "Character idle selected: ") +
            IdleAnimationPath);

        return true;
    }
    
    bool Animator::AddMesh(
        const std::size_t sceneMeshIndex,
        const core::assets::VisualAsset& visual,
        const std::vector<std::string>& paletteNodes,
        core::assets::MeshData sourceMesh,
        std::string& error)
    {
        error.clear();

        if (!sourceMesh.skinned)
        {
            return true;
        }

        State::MeshBinding
            binding;

        binding.sceneMeshIndex =
            sceneMeshIndex;

        binding.visual =
            visual;

        binding.paletteNodes =
            paletteNodes;

        binding.sourceMesh =
            std::move(sourceMesh);

        binding.outputMesh =
            binding.sourceMesh;

        state_->meshes.push_back(
            std::move(binding));

        return true;
    }
    
    bool Animator::Update(
        const float elapsedSeconds,
        graphics::Renderer& renderer,
        std::string& error)
    {
        error.clear();

        if (!state_->ready)
        {
            return true;
        }

        const float frameCount =
            std::max(
                state_->idle.totalFrames,
                1.0f);

        float frame =
            elapsedSeconds *
            IdleFrameRate;

        frame =
            std::fmod(
                frame,
                frameCount);

        if (frame < 0.0f)
        {
            frame +=
                frameCount;
        }

        std::vector<Transform>
            nodeTransforms;

        std::vector<Transform>
            currentPalette;

        for (State::MeshBinding& binding :
             state_->meshes)
        {
            if (!BuildAnimatedNodeTransforms(
                    state_->idle,
                    binding.visual,
                    frame,
                    nodeTransforms,
                    error))
            {
                return false;
            }

            if (!BuildPalette(
                    binding.visual,
                    binding.paletteNodes,
                    nodeTransforms,
                    currentPalette,
                    error))
            {
                return false;
            }

            if (!SkinMesh(
                binding.sourceMesh,
                binding.outputMesh,
                currentPalette,
                error))
            {
                return false;
            }

            if (!renderer.UpdateMeshVertices(
                    binding.sceneMeshIndex,
                    binding.outputMesh,
                    error))
            {
                return false;
            }
        }

        return true;
    }
    
    bool Animator::IsReady() const noexcept
    {
        return
            state_ != nullptr &&
            state_->ready;
    }
}
