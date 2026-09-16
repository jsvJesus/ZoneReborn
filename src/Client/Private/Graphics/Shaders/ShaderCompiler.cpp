#include "Graphics/Shaders/ShaderCompiler.h"

#include <Windows.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <array>
#include <filesystem>
#include <string>

namespace
{
    using Microsoft::WRL::ComPtr;

    bool ExecutableDirectory(
        std::filesystem::path& output,
        std::string& error)
    {
        std::array<
            wchar_t,
            32768>
            buffer{};

        const DWORD length =
            GetModuleFileNameW(
                nullptr,
                buffer.data(),
                static_cast<DWORD>(
                    buffer.size()));

        if (length == 0 ||
            length >=
                static_cast<DWORD>(
                    buffer.size()))
        {
            error =
                "Unable to resolve executable directory.";

            return false;
        }

        output =
            std::filesystem::path(
                std::wstring(
                    buffer.data(),
                    length))
                .parent_path();

        return true;
    }
}

namespace client::graphics::shaders
{
    bool CompileFromFile(
        const std::wstring_view relativePath,
        const char* entryPoint,
        const char* profile,
        ID3DBlob** output,
        std::string& error)
    {
        error.clear();

        if (output == nullptr)
        {
            error =
                "Shader output pointer is null.";

            return false;
        }

        *output =
            nullptr;

        if (relativePath.empty() ||
            entryPoint == nullptr ||
            profile == nullptr)
        {
            error =
                "Invalid shader compilation parameters.";

            return false;
        }

        std::filesystem::path
            executableDirectory;

        if (!ExecutableDirectory(
                executableDirectory,
                error))
        {
            return false;
        }

        const std::filesystem::path
            shaderPath =
                executableDirectory /
                L"Shaders" /
                std::filesystem::path(
                    relativePath);

        std::error_code
            fileError;

        if (!std::filesystem::is_regular_file(
                shaderPath,
                fileError))
        {
            error =
                "Shader file not found: ";

            error +=
                shaderPath.string();

            return false;
        }

        ComPtr<ID3DBlob>
            shader;

        ComPtr<ID3DBlob>
            errors;

        constexpr UINT CompileFlags =
            D3DCOMPILE_ENABLE_STRICTNESS |
            D3DCOMPILE_OPTIMIZATION_LEVEL3;

        const HRESULT result =
            D3DCompileFromFile(
                shaderPath.c_str(),
                nullptr,
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                entryPoint,
                profile,
                CompileFlags,
                0,
                shader.GetAddressOf(),
                errors.GetAddressOf());

        if (FAILED(result))
        {
            error =
                "Shader compilation failed: ";

            error +=
                shaderPath.string();

            if (errors &&
                errors->GetBufferPointer() !=
                    nullptr &&
                errors->GetBufferSize() >
                    0)
            {
                error +=
                    "\n";

                error.append(
                    static_cast<const char*>(
                        errors->GetBufferPointer()),
                    errors->GetBufferSize());
            }

            return false;
        }

        *output =
            shader.Detach();

        return true;
    }
}