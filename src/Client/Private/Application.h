#pragma once

#include "Core/Runtime.h"

namespace client
{
    class Application final
    {
    public:
        int Run();

    private:
        bool Initialize();
        void Shutdown();

        core::Runtime runtime_;
    };
}