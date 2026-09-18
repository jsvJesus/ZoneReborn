#pragma once

namespace client::states
{
    enum class ClientState
    {
        Boot,
        Frontend,
        LoadingWorld,
        World,
        Exit
    };
}