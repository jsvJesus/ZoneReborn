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

        if (modelParameters.z > 0.5f)
        {
            // Brow_*.dds contains a single eyebrow, not a head-sized UV
            // overlay. Project it only onto the two brow areas and mirror
            // it around the centre of the face. This also keeps it off the
            // temples, ears and back of the head.
            const float side = abs(input.localPosition.x);
            const float horizontal = saturate(
                (side - 0.008f) /
                0.064f);
            const float vertical = saturate(
                (input.localPosition.y - 1.665f) /
                0.075f);

            const float2 overlayUV = float2(
                lerp(0.94f, 0.06f, horizontal),
                lerp(0.74f, 0.18f, vertical));

            const float4 overlay = modelOverlayTexture.Sample(
                terrainTextureSampler,
                overlayUV);

            const float browMask =
                smoothstep(0.004f, 0.012f, side) *
                (1.0f - smoothstep(0.068f, 0.078f, side)) *
                smoothstep(1.655f, 1.675f, input.localPosition.y) *
                (1.0f - smoothstep(1.735f, 1.750f, input.localPosition.y)) *
                smoothstep(0.070f, 0.105f, input.localPosition.z);

            baseColour = lerp(
                baseColour,
                modelOverlayColour.rgb,
                saturate(
                    overlay.a *
                    browMask *
                    modelOverlayColour.a));
        }
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
