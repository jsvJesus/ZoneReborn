#include "Studio/EditorScene.h"
#include "Studio/LevelCatalog.h"

#include <utility>

namespace studio
{
    EditorScene::EditorScene()
    {
        New();
    }

    void EditorScene::New()
    {
        name_ = "Untitled";
        directory_.clear();

        statistics_ = {};
        collision_.reset();

        ++revision_;

        empty_ = true;
        dirty_ = false;
    }

    void EditorScene::Open(
        const std::filesystem::path& directory,
        const SceneStatistics& statistics,
        std::shared_ptr<const core::world::xray::CformCollision> collision)
    {
        directory_ = directory;

        name_ = PathToUtf8(
            directory.filename());

        statistics_ = statistics;
        collision_ = std::move(collision);

        ++revision_;

        empty_ = false;
        dirty_ = false;
    }

    const core::world::xray::CformCollision* EditorScene::Collision() const noexcept
    {
        return collision_.get();
    }

    void EditorScene::SetCollisionProbe(const CollisionProbeStatistics& probe) noexcept
    {
        statistics_.collisionProbe = probe;
    }

    void EditorScene::SetHomFrameStatistics(
        const std::size_t testedVisuals,
        const std::size_t culledVisuals) noexcept
    {
        statistics_.homTestedVisuals = testedVisuals;
        statistics_.homCulledVisuals = culledVisuals;
    }

    const std::string&
    EditorScene::Name() const noexcept
    {
        return name_;
    }

    const std::filesystem::path&
    EditorScene::Directory() const noexcept
    {
        return directory_;
    }

    const SceneStatistics&
    EditorScene::Statistics() const noexcept
    {
        return statistics_;
    }

    std::uint64_t
    EditorScene::Revision() const noexcept
    {
        return revision_;
    }

    bool EditorScene::IsEmpty() const noexcept
    {
        return empty_;
    }

    bool EditorScene::IsDirty() const noexcept
    {
        return dirty_;
    }
}
