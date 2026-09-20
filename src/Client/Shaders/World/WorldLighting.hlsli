#ifndef WORLD_LIGHTING_HLSLI
#define WORLD_LIGHTING_HLSLI

#include "WorldShared.hlsli"

void EvaluateOmniLights(
    float3 worldPosition,
    float3 surfaceNormal,
    out float3 diffuseLighting,
    out float3 specularLighting)
{
    diffuseLighting =
        float3(
            0.0f,
            0.0f,
            0.0f);

    specularLighting =
        float3(
            0.0f,
            0.0f,
            0.0f);

    const float3 normal =
        normalize(
            surfaceNormal);

    const float3 viewDirection =
        normalize(
            cameraPosition.xyz -
            worldPosition);

    [loop]
    for (uint index = 0;
         index < omniLightCount;
         ++index)
    {
        const OmniLightData light =
            omniLights[index];

        const float3 toLight =
            light.positionOuterRadius.xyz -
            worldPosition;

        const float distanceToLight =
            length(
                toLight);

        const float outerRadius =
            max(
                light.positionOuterRadius.w,
                0.001f);

        if (distanceToLight >=
            outerRadius)
        {
            continue;
        }

        const float innerRadius =
            clamp(
                light.parameters.x,
                0.0f,
                outerRadius);

        const float3 lightDirection =
            toLight /
            max(
                distanceToLight,
                0.0001f);

        float attenuation =
            1.0f;

        if (distanceToLight >
            innerRadius)
        {
            attenuation =
                saturate(
                    (
                        outerRadius -
                        distanceToLight
                    ) /
                    max(
                        outerRadius -
                        innerRadius,
                        0.001f));
        }

        attenuation *=
            attenuation;

        const float diffuse =
            saturate(
                dot(
                    normal,
                    lightDirection));

        const float3 lightColour =
            light.colourMultiplier.rgb *
            light.colourMultiplier.w;

        diffuseLighting +=
            lightColour *
            diffuse *
            attenuation;

        if (light.parameters.y >
            0.5f)
        {
            const float3 halfDirection =
                normalize(
                    lightDirection +
                    viewDirection);

            const float specular =
                pow(
                    saturate(
                        dot(
                            normal,
                            halfDirection)),
                    32.0f);

            specularLighting +=
                lightColour *
                specular *
                attenuation *
                0.35f;
        }
    }
}

void EvaluateSpotLights(
    float3 worldPosition,
    float3 surfaceNormal,
    out float3 diffuseLighting,
    out float3 specularLighting)
{
    diffuseLighting =
        float3(
            0.0f,
            0.0f,
            0.0f);

    specularLighting =
        float3(
            0.0f,
            0.0f,
            0.0f);

    const float3 normal =
        normalize(
            surfaceNormal);

    const float3 viewDirection =
        normalize(
            cameraPosition.xyz -
            worldPosition);

    [loop]
    for (uint index = 0;
         index < spotLightCount;
         ++index)
    {
        const SpotLightData light =
            spotLights[index];

        const float3 toLight =
            light.positionOuterRadius.xyz -
            worldPosition;

        const float distanceToLight =
            length(
                toLight);

        const float outerRadius =
            max(
                light.positionOuterRadius.w,
                0.001f);

        if (distanceToLight >=
            outerRadius)
        {
            continue;
        }

        const float3 surfaceToLight =
            toLight /
            max(
                distanceToLight,
                0.0001f);

        const float3 lightToSurface =
            -surfaceToLight;

        const float3 spotDirection =
            normalize(
                light.directionCosConeAngle.xyz);

        const float coneLimit =
            clamp(
                light.directionCosConeAngle.w,
                -1.0f,
                1.0f);

        const float coneCosine =
            dot(
                spotDirection,
                lightToSurface);

        if (coneCosine <
            coneLimit)
        {
            continue;
        }

        const float coneAttenuation =
            smoothstep(
                coneLimit,
                1.0f,
                coneCosine);

        const float innerRadius =
            clamp(
                light.parameters.x,
                0.0f,
                outerRadius);

        float distanceAttenuation =
            1.0f;

        if (distanceToLight >
            innerRadius)
        {
            distanceAttenuation =
                saturate(
                    (
                        outerRadius -
                        distanceToLight
                    ) /
                    max(
                        outerRadius -
                        innerRadius,
                        0.001f));
        }

        distanceAttenuation *=
            distanceAttenuation;

        const float attenuation =
            distanceAttenuation *
            coneAttenuation;

        const float diffuse =
            saturate(
                dot(
                    normal,
                    surfaceToLight));

        const float3 lightColour =
            light.colourMultiplier.rgb *
            light.colourMultiplier.w;

        diffuseLighting +=
            lightColour *
            diffuse *
            attenuation;

        if (light.parameters.y >
            0.5f)
        {
            const float3 halfDirection =
                normalize(
                    surfaceToLight +
                    viewDirection);

            const float specular =
                pow(
                    saturate(
                        dot(
                            normal,
                            halfDirection)),
                    32.0f);

            specularLighting +=
                lightColour *
                specular *
                attenuation *
                0.35f;
        }
    }
}

#endif