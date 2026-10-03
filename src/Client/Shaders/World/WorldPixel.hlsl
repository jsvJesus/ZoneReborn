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

    float staticHemi =
        1.0f;

    float staticSun =
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

        if (modelLightmapParameters.y > 0.5f)
        {
            // Terrain alpha is baked hemisphere occlusion, not opacity.
            staticHemi = saturate(modelSample.a);
        }

        const float tintMode =
            modelParameters.w;

        if (tintMode > 0.5f && tintMode < 1.5f)
        {
            baseColour = lerp(
                baseColour,
                baseColour * modelSkinColour.rgb,
                saturate(modelSkinColour.a));
        }
        else if (tintMode > 1.5f && tintMode < 2.5f)
        {
            baseColour = lerp(
                baseColour,
                baseColour * modelHairColour.rgb,
                saturate(modelHairColour.a));
        }
        else if (tintMode > 2.5f)
        {
            const float dyeMask = modelDyeMaskTexture.Sample(
                terrainTextureSampler,
                input.terrainUV).r;

            baseColour = lerp(
                baseColour,
                baseColour * modelDyeColour.rgb,
                saturate(dyeMask * modelDyeColour.a));
        }

        if (modelParameters.z > 0.5f)
        {
            const float4 overlay = modelOverlayTexture.Sample(
                terrainTextureSampler,
                input.terrainUV);

            baseColour = lerp(
                baseColour,
                overlay.rgb * modelOverlayColour.rgb,
                saturate(
                    overlay.a *
                    modelOverlayColour.a));
        }

        if (modelOverlayParameters.x > 0.5f)
        {
            const float4 tattoo = modelTattooTexture.Sample(
                terrainTextureSampler,
                input.terrainUV);

            baseColour = lerp(
                baseColour,
                tattoo.rgb * modelTattooColour.rgb,
                saturate(
                    tattoo.a *
                    modelTattooColour.a));
        }
    }

    if (modelLightmapParameters.x > 0.5f)
    {
        const float4 lightmap =
            modelLightmapTexture.Sample(
                terrainTextureSampler,
                modelLightmapParameters.y > 0.5f
                    ? input.terrainUV
                    : input.lightmapUV);

        if (modelLightmapParameters.y > 0.5f)
        {
            staticSun = saturate(lightmap.a);
        }
        else
        {
            staticHemi = saturate(lightmap.a);
            staticSun = saturate(lightmap.g);
        }
    }

    baseColour *=
        instanceColour.rgb;

    outputAlpha *=
        instanceColour.a;

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
        (
            0.35f * staticHemi +
            // X-Ray adds ambient independently of baked hemi occlusion.
            // This is the preview baseline until its environment is loaded.
            (modelLightmapParameters.z > 0.5f ? 0.15f : 0.0f)
        );

    const float3 directionalLighting =
        skySunColour.rgb *
        sunDiffuse *
        skySunDirectionDaylight.w *
        0.85f *
        staticSun;

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
