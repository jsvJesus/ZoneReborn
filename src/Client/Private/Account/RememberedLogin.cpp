#include "Account/RememberedLogin.h"

#include <fstream>

namespace client::account
{
    bool RememberedLogin::Initialize(
        const std::filesystem::path& gameRoot)
    {
        if (gameRoot.empty())
        {
            return false;
        }

        const std::filesystem::path userDirectory =
            gameRoot /
            "user";

        std::error_code error;

        std::filesystem::create_directories(
            userDirectory,
            error);

        if (error)
        {
            return false;
        }

        filePath_ =
            userDirectory /
            "login.cfg";

        return true;
    }

    std::string RememberedLogin::Load() const
    {
        if (filePath_.empty())
        {
            return {};
        }

        std::ifstream stream(
            filePath_,
            std::ios::binary);

        if (!stream)
        {
            return {};
        }

        std::string login;

        std::getline(
            stream,
            login);

        return login;
    }

    bool RememberedLogin::Save(
        const std::string& login) const
    {
        if (filePath_.empty())
        {
            return false;
        }

        std::ofstream stream(
            filePath_,
            std::ios::binary |
            std::ios::trunc);

        if (!stream)
        {
            return false;
        }

        stream.write(
            login.data(),
            static_cast<std::streamsize>(
                login.size()));

        return
            static_cast<bool>(
                stream);
    }

    void RememberedLogin::Clear() const
    {
        if (filePath_.empty())
        {
            return;
        }

        std::error_code error;

        std::filesystem::remove(
            filePath_,
            error);
    }
}