#pragma once

#include <string>

namespace client::account
{
    class AccountSession final
    {
    public:
        void Establish(
            std::string login,
            std::string token)
        {
            login_ =
                std::move(login);

            token_ =
                std::move(token);

            authenticated_ =
                true;
        }

        void Clear()
        {
            authenticated_ =
                false;

            login_.clear();
            token_.clear();
        }

        [[nodiscard]]
        bool IsAuthenticated() const noexcept
        {
            return authenticated_;
        }

        [[nodiscard]]
        const std::string& Login() const noexcept
        {
            return login_;
        }

        [[nodiscard]]
        const std::string& Token() const noexcept
        {
            return token_;
        }

    private:
        bool authenticated_ =
            false;

        std::string login_;
        std::string token_;
    };
}