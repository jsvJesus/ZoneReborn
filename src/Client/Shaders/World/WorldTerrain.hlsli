#ifndef WORLD_TERRAIN_HLSLI
#define WORLD_TERRAIN_HLSLI

#include "WorldShared.hlsli"

float2 ProjectTerrainUV(
    float3 position,
    float4 uProjection,
    float4 vProjection)
{
    const float4 p =
        float4(
            position,
            1.0f);

    return float2(
        dot(
            p,
            uProjection),
        dot(
            p,
            vProjection));
}

float3 SampleTerrain(
    PixelInput input)
{
    const float4 weights =
        terrainBlend.Sample(
            terrainBlendSampler,
            input.terrainUV);

    float3 colour =
        0.0f;

    if (terrainLayerCount >= 1)
    {
        colour +=
            terrainTexture0.Sample(
                terrainTextureSampler,
                ProjectTerrainUV(
                    input.localPosition,
                    terrainU[0],
                    terrainV[0])).rgb *
            weights.r;
    }

    if (terrainLayerCount >= 2)
    {
        colour +=
            terrainTexture1.Sample(
                terrainTextureSampler,
                ProjectTerrainUV(
                    input.localPosition,
                    terrainU[1],
                    terrainV[1])).rgb *
            weights.g;
    }

    if (terrainLayerCount >= 3)
    {
        colour +=
            terrainTexture2.Sample(
                terrainTextureSampler,
                ProjectTerrainUV(
                    input.localPosition,
                    terrainU[2],
                    terrainV[2])).rgb *
            weights.b;
    }

    if (terrainLayerCount >= 4)
    {
        colour +=
            terrainTexture3.Sample(
                terrainTextureSampler,
                ProjectTerrainUV(
                    input.localPosition,
                    terrainU[3],
                    terrainV[3])).rgb *
            weights.a;
    }

    return colour;
}

#endif