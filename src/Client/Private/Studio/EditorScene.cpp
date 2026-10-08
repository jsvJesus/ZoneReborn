#include "Studio/EditorScene.h"

namespace studio
{
    EditorScene::EditorScene()
    {
        New();
    }

    void EditorScene::New()
    {
        name_ = "Untitled";

        ++revision_;

        empty_ = true;
        dirty_ = false;
    }

    const std::string&
    EditorScene::Name() const noexcept
    {
        return name_;
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