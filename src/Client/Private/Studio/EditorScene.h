#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace studio
{
    struct SceneStatistics final
    {
        std::size_t visuals = 0;
        std::size_t meshes = 0;
        std::size_t vertices = 0;
        std::size_t triangles = 0;
        std::size_t textures = 0;
        std::size_t missingTextures = 0;
        bool homPresent = false;
        std::size_t homTriangles = 0;
        std::size_t homTestedVisuals = 0;
        std::size_t homCulledVisuals = 0;
    };

    class EditorScene final
    {
    public:
        EditorScene();

        void New();

        void Open(
            const std::filesystem::path& directory,
            const SceneStatistics& statistics);

        void SetHomFrameStatistics(
            std::size_t testedVisuals,
            std::size_t culledVisuals) noexcept;

        [[nodiscard]]
        const std::string& Name() const noexcept;

        [[nodiscard]]
        const std::filesystem::path& Directory() const noexcept;

        [[nodiscard]]
        const SceneStatistics& Statistics() const noexcept;

        [[nodiscard]]
        std::uint64_t Revision() const noexcept;

        [[nodiscard]]
        bool IsEmpty() const noexcept;

        [[nodiscard]]
        bool IsDirty() const noexcept;

    private:
        std::string name_;
        std::filesystem::path directory_;

        SceneStatistics statistics_{};

        std::uint64_t revision_ = 0;

        bool empty_ = true;
        bool dirty_ = false;
    };
}
