#pragma once

namespace client::states
{
    enum class ClientState
    {
        Boot,
        Login,
        Authenticating,
        MainMenu,
        ServerSelect,
        LoadingWorld,
        World,
        Exit
    };
}