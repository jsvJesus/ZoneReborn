#include "WorldShared.hlsli"
#include "WorldTerrain.hlsli"
#include "WorldLighting.hlsli"
#include "WorldWater.hlsli"

float4 PSMain(
    PixelInput input) : SV_TARGET
{
    const float3 normal =
        normalize(
            input.normal);

    float3 baseColour =
        groupColour.rgb;

    float outputAlpha =
        1.0f;

    if (useWater != 0)
    {
        return
            ShadeWater(
                input);
    }

    if (useTerrain != 0)
    {
        baseColour =
            SampleTerrain(
                input);
    }
    else if (useModelTexture != 0)
    {
        const float4 modelSample =
            modelTexture.Sample(
                terrainTextureSampler,
                input.terrainUV);

        const float alphaMode =
            modelParameters.y;

        if (alphaMode > 0.5f &&
            alphaMode < 1.5f)
        {
            clip(
                modelSample.a -
                modelParameters.x);
        }

        if (alphaMode > 1.5f)
        {
            outputAlpha =
                modelSample.a;
        }

        baseColour =
            modelSample.rgb;

        baseColour = lerp(
            baseColour,
            baseColour * modelTint.rgb,
            saturate(modelTint.a));
    }

    const float3 sunDirection =
        normalize(
            skySunDirectionDaylight.xyz);

    const float sunDiffuse =
        saturate(
            dot(
                normal,
                sunDirection));

    const float3 ambientLighting =
        skyAmbientColour.rgb *
        0.35f;

    const float3 directionalLighting =
        skySunColour.rgb *
        sunDiffuse *
        skySunDirectionDaylight.w *
        0.85f;

    float3 omniDiffuse =
        0.0f;

    float3 omniSpecular =
        0.0f;

    EvaluateOmniLights(
        input.worldPosition,
        normal,
        omniDiffuse,
        omniSpecular);

    float3 spotDiffuse =
        0.0f;

    float3 spotSpecular =
        0.0f;

    EvaluateSpotLights(
        input.worldPosition,
        normal,
        spotDiffuse,
        spotSpecular);

    float3 finalColour =
        baseColour *
        (
            ambientLighting +
            directionalLighting +
            omniDiffuse +
            spotDiffuse
        );

    finalColour +=
        omniSpecular +
        spotSpecular;

    return
        float4(
            finalColour,
            outputAlpha);
}
