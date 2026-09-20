#ifndef __CB_INCLUDE__
#define __CB_INCLUDE__

// ============================================================
//  Common Constant Buffer Definitions
//  Engine-wide contract. Layout must match C++ side exactly.
// ============================================================



// ============================================================
// Pass Buffer  (per render pass)
// ============================================================

#ifndef PASS_BUFFER_SLOT
    #define PASS_BUFFER_SLOT b0
#endif

//@semantic Pass
cbuffer _Pass : register(PASS_BUFFER_SLOT)
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;  //view-projection
    float4   resolution;
}

#endif