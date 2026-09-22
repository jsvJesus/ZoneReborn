#pragma once

#include <string>
#include <utility>

namespace client::account
{
    class AccountSession final
    {
    public:
        void Establish(
            std::string login,
            std::string token,
            std::string serverId)
        {
            login_ =
                std::move(login);

            token_ =
                std::move(token);

            serverId_ =
                std::move(serverId);

            authenticated_ =
                true;
        }

        void Clear()
        {
            authenticated_ =
                false;

            login_.clear();
            token_.clear();
            serverId_.clear();
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

        [[nodiscard]]
        const std::string& ServerId() const noexcept
        {
            return serverId_;
        }

    private:
        bool authenticated_ =
            false;

        std::string login_;
        std::string token_;
        std::string serverId_;
    };
}