#pragma once

namespace client::states
{
    enum class ClientState
    {
        Boot,
        StartupSplash,
        Login,
        Authenticating,
        MainMenu,
        ServerSelect,
        LoadingWorld,
        World,
        Exit
    };
}