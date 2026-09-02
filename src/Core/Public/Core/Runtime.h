#pragma once

#include "Core/Resources/ResourceFileSystem.h"

#include <filesystem>

namespace core
{
    class Runtime final
    {
    public:
        Runtime() = default;
        ~Runtime();

        Runtime(const Runtime&) = delete;
        Runtime& operator=(const Runtime&) = delete;

        Runtime(Runtime&&) = delete;
        Runtime& operator=(Runtime&&) = delete;

        [[nodiscard]]
        bool Initialize();

        void Shutdown();

        [[nodiscard]]
        bool IsInitialized() const noexcept;

        [[nodiscard]]
        resources::ResourceFileSystem& Resources() noexcept;

        [[nodiscard]]
        const resources::ResourceFileSystem& Resources() const noexcept;

        [[nodiscard]]
        const std::filesystem::path& GameRoot() const noexcept;

    private:
        std::filesystem::path gameRoot_;
        resources::ResourceFileSystem resources_;
        bool initialized_ = false;
    };
}