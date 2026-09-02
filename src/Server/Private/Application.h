#pragma once

#include "Core/Runtime.h"

namespace server
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