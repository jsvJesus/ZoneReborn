#include "Account/AuthService.h"

namespace client::account
{
    AuthResult AuthService::Authenticate(
        const std::string& login,
        const std::string& password) const
    {
        AuthResult result;

        if (login.empty())
        {
            result.error =
                "Login is empty.";

            return result;
        }

        if (password.empty())
        {
            result.error =
                "Password is empty.";

            return result;
        }

        if (login !=
                "test" ||
            password !=
                "test123")
        {
            result.error =
                "Invalid login or password.";

            return result;
        }

        result.success =
            true;

        result.login =
            login;

        result.sessionToken =
            "local-test-session";

        return result;
    }
}