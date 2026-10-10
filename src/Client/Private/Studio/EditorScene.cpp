#include "Studio/EditorScene.h"
#include "Studio/LevelCatalog.h"

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

        ++revision_;

        empty_ = true;
        dirty_ = false;
    }

    void EditorScene::Open(
        const std::filesystem::path& directory,
        const SceneStatistics& statistics)
    {
        directory_ = directory;

        name_ = PathToUtf8(
            directory.filename());

        statistics_ = statistics;

        ++revision_;

        empty_ = false;
        dirty_ = false;
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