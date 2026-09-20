#include "static_samplers.hlsli"

cbuffer CopyConstants : register(b0)
{
    uint2 srcSize;      // Исходный размер depth buffer
    uint2 dstSize;      // Целевой размер (может отличаться)
};

Texture2D<float> SourceDepth : register(t0);
RWTexture2D<float> DestHiZ : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= dstSize.x || id.y >= dstSize.y)
        return;
    
    // Вычисляем UV в source
    float2 uv = (float2(id.xy) + 0.5) / float2(dstSize);
    // Если размеры совпадают - point sample, иначе - может потребоваться bilinear
    float depth = SourceDepth.SampleLevel(PointClamp, uv, 0);
    
    // Конвертация non-linear depth to linear если нужно
    // Для reverse-Z обычно просто копируем
    DestHiZ[id.xy] = depth;
}