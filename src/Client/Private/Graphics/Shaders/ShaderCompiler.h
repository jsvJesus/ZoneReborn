#pragma once

#include <d3dcommon.h>

#include <string>
#include <string_view>

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