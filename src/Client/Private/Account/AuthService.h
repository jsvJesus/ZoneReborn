#pragma once

#include <string>

namespace client::account
{
    struct AuthResult final
    {
        bool success =
            false;

        std::string login;
        std::string sessionToken;
        std::string error;
    };

    class AuthService final
    {
    public:
        [[nodiscard]]
        AuthResult Authenticate(
            const std::string& login,
            const std::string& password) const;
    };
}