#pragma once

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

        bool Initialize();
        void Shutdown();

        [[nodiscard]]
        bool IsInitialized() const noexcept;

    private:
        bool initialized_ = false;
    };
}