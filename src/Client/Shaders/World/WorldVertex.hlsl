#include "WorldShared.hlsli"

PixelInput VSMain(
    VertexInput input)
{
    PixelInput output;

    const float4 worldPosition =
        mul(
            float4(
                input.position,
                1.0f),
            world);

    output.position =
        mul(
            worldPosition,
            viewProjection);

    output.normal =
        normalize(
            mul(
                float4(
                    input.normal,
                    0.0f),
                world).xyz);

    output.localPosition =
        input.position;

    output.terrainUV =
        input.texcoord;

    output.worldPosition =
        worldPosition.xyz;

    return output;
}