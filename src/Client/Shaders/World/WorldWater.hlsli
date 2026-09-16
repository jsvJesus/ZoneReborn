#ifndef WORLD_WATER_HLSLI
#define WORLD_WATER_HLSLI

#include "WorldShared.hlsli"

float2 GetScreenUV(
    PixelInput input)
{
    return
        input.position.xy /
        screenParameters.xy;
}

float3 ReconstructWorldPosition(
    float2 uv,
    float depth)
{
    float2 ndc;

    ndc.x =
        uv.x * 2.0f -
        1.0f;

    ndc.y =
        1.0f -
        uv.y * 2.0f;

    float4 position =
        mul(
            float4(
                ndc,
                depth,
                1.0f),
            inverseViewProjection);

    position.xyz /=
        max(
            position.w,
            0.00001f);

    return position.xyz;
}

float2 ProjectWorldPosition(
    float3 position,
    out float depth)
{
    const float4 clipPosition =
        mul(
            float4(
                position,
                1.0f),
            viewProjection);

    const float inverseW =
        1.0f /
        max(
            clipPosition.w,
            0.00001f);

    const float2 ndc =
        clipPosition.xy *
        inverseW;

    depth =
        clipPosition.z *
        inverseW;

    return float2(
        ndc.x * 0.5f +
            0.5f,
        -ndc.y * 0.5f +
            0.5f);
}

float3 CalculateWaterNormal(
    PixelInput input)
{
    const float textureScale =
        max(
            waterParameters2.x,
            0.001f);

    const float time =
        waterParameters1.w;

    const float wind =
        waterParameters1.z;

    const float2 baseUV =
        input.localPosition.xz /
        textureScale;

    const float2 uv1 =
        baseUV +
        waterScrollSpeed1.xy *
        wind *
        time;

    const float2 uv2 =
        baseUV +
        waterScrollSpeed2.xy *
        wind *
        time;

    const float2 normal1 =
        waterNormalTexture.Sample(
            terrainTextureSampler,
            uv1).rg *
        2.0f -
        1.0f;

    const float2 normal2 =
        waterNormalTexture.Sample(
            terrainTextureSampler,
            uv2).rg *
        2.0f -
        1.0f;

    float2 wave =
        (
            normal1 +
            normal2
        ) *
        0.5f;

    wave.x *=
        waterParameters1.x;

    wave.y *=
        waterParameters1.y;

    return
        normalize(
            float3(
                wave.x,
                1.0f,
                wave.y));
}

float3 TraceWaterReflection(
    float3 worldPosition,
    float3 normal,
    float2 fallbackUV)
{
    const float3 incident =
        normalize(
            worldPosition -
            cameraPosition.xyz);

    const float3 direction =
        normalize(
            reflect(
                incident,
                normal));

    float3 position =
        worldPosition +
        normal *
        0.05f;

    float stepLength =
        1.0f;

    float2 lastUV =
        fallbackUV;

    [loop]
    for (int index = 0;
         index < 20;
         ++index)
    {
        position +=
            direction *
            stepLength;

        float projectedDepth =
            0.0f;

        const float2 uv =
            ProjectWorldPosition(
                position,
                projectedDepth);

        if (uv.x <= 0.001f ||
            uv.x >= 0.999f ||
            uv.y <= 0.001f ||
            uv.y >= 0.999f ||
            projectedDepth <= 0.0f ||
            projectedDepth >= 1.0f)
        {
            break;
        }

        lastUV =
            uv;

        const float sceneDepth =
            sceneDepthTexture.SampleLevel(
                terrainBlendSampler,
                uv,
                0.0f).r;

        if (sceneDepth <
            0.99999f)
        {
            const float3 scenePosition =
                ReconstructWorldPosition(
                    uv,
                    sceneDepth);

            const float rayDistance =
                distance(
                    cameraPosition.xyz,
                    position);

            const float sceneDistance =
                distance(
                    cameraPosition.xyz,
                    scenePosition);

            const float difference =
                rayDistance -
                sceneDistance;

            if (difference >= 0.0f &&
                difference <
                    stepLength *
                    2.0f)
            {
                return
                    sceneColourTexture.SampleLevel(
                        terrainBlendSampler,
                        uv,
                        0.0f).rgb *
                    waterReflectionTint.rgb;
            }
        }

        stepLength =
            min(
                stepLength *
                1.18f,
                8.0f);
    }

    return
        sceneColourTexture.SampleLevel(
            terrainBlendSampler,
            lastUV,
            0.0f).rgb *
        waterReflectionTint.rgb;
}

float4 ShadeWater(
    PixelInput input)
{
    const float reflectionStrength =
        waterParameters0.x;

    const float refractionStrength =
        waterParameters0.y;

    const float fresnelConstant =
        waterParameters0.z;

    const float fresnelExponent =
        max(
            waterParameters0.w,
            0.001f);

    const float3 waterNormal =
        CalculateWaterNormal(
            input);

    const float3 viewDirection =
        normalize(
            cameraPosition.xyz -
            input.worldPosition);

    const float viewDot =
        saturate(
            dot(
                waterNormal,
                viewDirection));

    float fresnel =
        fresnelConstant +
        (
            1.0f -
            fresnelConstant
        ) *
        pow(
            1.0f -
                viewDot,
            fresnelExponent);

    fresnel =
        saturate(
            fresnel);

    const float2 screenUV =
        GetScreenUV(
            input);

    const float2 distortion =
        waterNormal.xz *
        0.0125f *
        refractionStrength;

    const float2 refractionUV =
        clamp(
            screenUV +
                distortion,
            float2(
                0.001f,
                0.001f),
            float2(
                0.999f,
                0.999f));

    float3 refractionColour =
        sceneColourTexture.SampleLevel(
            terrainBlendSampler,
            refractionUV,
            0.0f).rgb;

    refractionColour *=
        waterRefractionTint.rgb;

    const float sceneDepth =
        sceneDepthTexture.SampleLevel(
            terrainBlendSampler,
            refractionUV,
            0.0f).r;

    float verticalDepth =
        waterParameters3.x;

    if (sceneDepth <
        0.99999f)
    {
        const float3 scenePosition =
            ReconstructWorldPosition(
                refractionUV,
                sceneDepth);

        verticalDepth =
            max(
                0.0f,
                input.worldPosition.y -
                scenePosition.y);
    }

    const float configuredDepth =
        max(
            waterParameters3.x,
            0.01f);

    const float fadeDistance =
        waterParameters3.y >
            0.0f
            ? waterParameters3.y
            : configuredDepth;

    const float deepFactor =
        saturate(
            verticalDepth /
            max(
                fadeDistance,
                0.01f));

    refractionColour =
        lerp(
            refractionColour,
            waterDeepColour.rgb,
            deepFactor);

    const float3 reflectionColour =
        TraceWaterReflection(
            input.worldPosition,
            waterNormal,
            screenUV);

    const float reflectionWeight =
        saturate(
            fresnel *
            reflectionStrength);

    const float refractionWeight =
        saturate(
            (
                1.0f -
                fresnel
            ) *
            refractionStrength);

    const float totalWeight =
        max(
            reflectionWeight +
                refractionWeight,
            0.0001f);

    float3 colour =
        (
            reflectionColour *
                reflectionWeight +
            refractionColour *
                refractionWeight
        ) /
        totalWeight;

    const float foamIntersectionDistance =
        max(
            waterParameters2.y *
                0.00016f,
            0.025f);

    const float foamSceneDepth =
        sceneDepthTexture.SampleLevel(
            terrainBlendSampler,
            screenUV,
            0.0f).r;

    float foamDepth =
        foamIntersectionDistance;

    if (foamSceneDepth <
        0.99999f)
    {
        const float3 foamScenePosition =
            ReconstructWorldPosition(
                screenUV,
                foamSceneDepth);

        foamDepth =
            max(
                0.0f,
                input.worldPosition.y -
                foamScenePosition.y);
    }

    float foamAmount =
        1.0f -
        saturate(
            foamDepth /
            foamIntersectionDistance);

    foamAmount =
        smoothstep(
            0.55f,
            1.0f,
            foamAmount);

    foamAmount *=
        foamAmount;

    float foamTextureScale =
        waterParameters2.w;

    if (foamTextureScale <=
        0.0f)
    {
        foamTextureScale =
            max(
                waterParameters2.x,
                0.001f);
    }

    const float2 foamUV =
        input.localPosition.xz /
            foamTextureScale +
        waterScrollSpeed1.xy *
            waterParameters1.z *
            waterParameters1.w *
            0.18f;

    float foamSample =
        waterFoamTexture.Sample(
            terrainTextureSampler,
            foamUV).r;

    foamSample =
        smoothstep(
            0.62f,
            0.90f,
            foamSample);

    foamAmount *=
        foamSample;

    foamAmount *=
        saturate(
            waterParameters2.z);

    foamAmount *=
        0.38f;

    const float3 foamColour =
        lerp(
            waterDeepColour.rgb,
            float3(
                0.68f,
                0.72f,
                0.70f),
            0.72f);

    colour =
        lerp(
            colour,
            foamColour,
            saturate(
                foamAmount));

    const float3 lightDirection =
        normalize(
            float3(
                -0.35f,
                0.85f,
                -0.40f));

    const float3 halfDirection =
        normalize(
            lightDirection +
            viewDirection);

    float specular =
        pow(
            saturate(
                dot(
                    waterNormal,
                    halfDirection)),
            max(
                waterParameters3.w,
                1.0f));

    specular *=
        cameraPosition.w;

    colour +=
        specular;

    return float4(
        colour,
        1.0f);
}

#endif