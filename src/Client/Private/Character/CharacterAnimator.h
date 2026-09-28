#pragma once

#include "Character/CharacterAnimationCatalog.h"
#include "Character/CharacterAnimationState.h"

#include "Core/Assets/MeshData.h"
#include "Core/Assets/VisualAsset.h"
#include "Core/Resources/ResourceFileSystem.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace client::graphics
{
    class Renderer;
}

namespace client::character
{
    class Animator final
    {
    public:
        Animator();
        ~Animator();

        Animator(
            const Animator&) =
            delete;

        Animator& operator=(
            const Animator&) =
            delete;

        void Reset();

        [[nodiscard]]
        bool LoadAnimations(
            const core::resources::ResourceFileSystem& resources,
            const AnimationSet& animations,
            std::string& error);

        [[nodiscard]]
        bool AddMesh(
            std::size_t sceneMeshIndex,
            const core::assets::VisualAsset& visual,
            const std::vector<std::string>& paletteNodes,
            core::assets::MeshData sourceMesh,
            std::string& error);

        [[nodiscard]]
        bool SetFaceForm(
            std::span<const std::uint64_t> packed,
            std::string& error);

        [[nodiscard]]
        bool Update(
            AnimationState animationState,
            float stateTimeSeconds,
            graphics::Renderer& renderer,
            std::string& error);

        [[nodiscard]]
        float Duration(
            AnimationState animationState) const noexcept;

        [[nodiscard]]
        bool IsFinished(
            AnimationState animationState,
            float stateTimeSeconds) const noexcept;

        [[nodiscard]]
        bool IsReady() const noexcept;

    private:
        struct State;

        std::unique_ptr<State>
            state_;
    };
}