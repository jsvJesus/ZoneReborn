#pragma once

#include <string>
#include <string_view>

struct ID3DBlob;

namespace client::graphics::shaders
{
    [[nodiscard]]
    bool CompileFromFile(
        std::wstring_view relativePath,
        const char* entryPoint,
        const char* profile,
        ID3DBlob** output,
        std::string& error);
}