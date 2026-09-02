#pragma once

#include "Core/Runtime.h"

namespace client::diagnostics
{
    [[nodiscard]]
    bool RunResourceValidation(
        core::Runtime& runtime);
}