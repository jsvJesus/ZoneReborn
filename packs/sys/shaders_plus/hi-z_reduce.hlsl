
#include "depth_buffer_macro.hlsli"

Texture2D<float> SourceMip : register(t0);
RWTexture2D<float> DestMip : register(u0);

cbuffer ReduceConstants : register(b0)
{
    uint2 srcSize;
    uint2 dstSize;
};

[numthreads(8, 8, 1)]
void main(
    uint3 id   : SV_DispatchThreadID,
    uint3 tid  : SV_GroupThreadID)
{
    if (id.x >= dstSize.x || id.y >= dstSize.y)
        return;

    // 2x2 block
    int2 baseCoord = int2(id.xy) * 2;

    float d0 = SourceMip[clamp(baseCoord + int2(0,0), int2(0,0), int2(srcSize)-1)];
    float d1 = SourceMip[clamp(baseCoord + int2(1,0), int2(0,0), int2(srcSize)-1)];
    float d2 = SourceMip[clamp(baseCoord + int2(0,1), int2(0,0), int2(srcSize)-1)];
    float d3 = SourceMip[clamp(baseCoord + int2(1,1), int2(0,0), int2(srcSize)-1)];

#ifdef REVERSE_Z_ON
    float result = min(min(d0, d1), min(d2, d3));
#else
    float result = max(max(d0, d1), max(d2, d3));
#endif

    DestMip[id.xy] = result;
}