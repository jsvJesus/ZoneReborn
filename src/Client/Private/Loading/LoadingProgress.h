#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace client::loading
{
    namespace stage
    {
        inline constexpr std::string_view Preparing =
            "preparing";

        inline constexpr std::string_view World =
            "world";

        inline constexpr std::string_view Resources =
            "resources";

        inline constexpr std::string_view Textures =
            "textures";

        inline constexpr std::string_view Models =
            "models";

        inline constexpr std::string_view Terrain =
            "terrain";

        inline constexpr std::string_view Vegetation =
            "vegetation";

        inline constexpr std::string_view Effects =
            "effects";

        inline constexpr std::string_view Water =
            "water";

        inline constexpr std::string_view Collision =
            "collision";

        inline constexpr std::string_view Character =
            "character";

        inline constexpr std::string_view Renderer =
            "renderer";

        inline constexpr std::string_view Finalizing =
            "finalizing";

        inline constexpr std::string_view Ready =
            "ready";
    }

    using ProgressCallback =
        std::function<
            void(
                std::uint32_t percent,
                std::string_view stage)>;

    inline void Report(
        const ProgressCallback& callback,
        std::uint32_t percent,
        const std::string_view stage)
    {
        if (!callback)
        {
            return;
        }

        if (percent > 100)
        {
            percent = 100;
        }

        callback(
            percent,
            stage);
    }
}