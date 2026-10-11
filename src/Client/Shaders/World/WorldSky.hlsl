#include "WorldShared.hlsli"

struct SkyVertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

SkyVertexOutput VSSky(
    uint vertexId : SV_VertexID)
{
    const float2 vertices[3] =
    {
        float2(
            -1.0f,
            -1.0f),

        float2(
            -1.0f,
            3.0f),

        float2(
            3.0f,
            -1.0f)
    };

    const float2 position =
        vertices[
            vertexId];

    SkyVertexOutput
        output;

    output.position =
        float4(
            position,
            0.0f,
            1.0f);

    output.uv =
        float2(
            position.x *
                0.5f +
                0.5f,

            1.0f -
            (
                position.y *
                    0.5f +
                0.5f
            ));

    return output;
}

float3 BuildFallbackSky(
    float3 viewDirection)
{
    const float height =
        saturate(
            viewDirection.y *
                0.5f +
            0.5f);

    const float horizonFactor =
        pow(
            height,
            max(
                0.1f,
                0.65f +
                skyAtmosphere0.w));

    const float3 horizonColour =
        lerp(
            skyAmbientColour.rgb,
            skySunColour.rgb,
            0.32f);

    const float3 zenithColour =
        skyAmbientColour.rgb *
            0.55f +
        skySunColour.rgb *
            0.10f;

    return
        lerp(
            horizonColour,
            zenithColour,
            horizonFactor);
}

float3 BuildWeatherSky(
    float3 viewDirection)
{
    const float height =
        saturate(
            viewDirection.y * 0.5f + 0.5f);

    const float3 fogColour =
        saturate(
            weatherFogColourDistance.rgb);

    const float3 skyTint =
        saturate(
            weatherSkyColour.rgb);

    const float daylight =
        saturate(
            dot(
                skySunColour.rgb,
                float3(0.2126f, 0.7152f, 0.0722f)) *
            skySunDirectionDaylight.w *
            1.5f);

    const float3 zenithColour =
        lerp(
            fogColour * 0.55f,
            float3(0.20f, 0.34f, 0.54f) * skyTint,
            saturate(daylight * 0.8f + 0.1f));

    float3 colour =
        lerp(
            fogColour,
            zenithColour,
            smoothstep(
                0.1f,
                0.95f,
                height));

    const float3 cloudsColour =
        saturate(
            weatherCloudColour.rgb);

    const float rain =
        saturate(
            weatherFogParameters.w);

    const float cloudBrightness =
        max(
            cloudsColour.r,
            max(
                cloudsColour.g,
                cloudsColour.b));

    const float cloudsMultiplier =
        saturate(
            weatherCloudColour.a);

    const float coverage =
        saturate(
            cloudBrightness * 0.65f +
            rain * 0.45f) *
        cloudsMultiplier;

    if (coverage > 0.001f)
    {
        const float rotation =
            radians(
                weatherSkyColour.w);

        const float azimuth =
            atan2(
                viewDirection.x,
                viewDirection.z) + rotation;

        float2 coordinates =
            float2(
                azimuth * 2.5f,
                viewDirection.y * 6.0f);

        const float windAngle =
            radians(
                weatherCloudParameters.z);

        const float2 wind =
            float2(
                cos(windAngle),
                sin(windAngle));

        const float windOffset =
            weatherCloudParameters.y *
            weatherCloudParameters.w *
            0.000003f;

        coordinates +=
            wind * windOffset;

        const float cloudPattern =
            0.5f +
            0.22f *
                sin(
                    coordinates.x * 2.1f +
                    coordinates.y * 1.3f) +
            0.17f *
                sin(
                    coordinates.x * 3.7f -
                    coordinates.y * 2.4f) +
            0.10f *
                sin(
                    coordinates.x * 6.3f +
                    coordinates.y * 4.9f);

        const float cloudMask =
            smoothstep(
                1.0f - coverage - 0.20f,
                1.0f - coverage + 0.15f,
                cloudPattern);

        const float cloudContrast =
            lerp(
                0.7f,
                1.0f,
                saturate(
                    weatherCloudParameters.x));

        const float3 cloudSurface =
            cloudsColour *
            lerp(
                0.50f,
                0.90f,
                daylight);

        colour =
            lerp(
                colour,
                cloudSurface,
                cloudMask *
                    cloudContrast);
    }

    return max(
        colour,
        float3(0.0f, 0.0f, 0.0f));
}

float4 PSSky(
    SkyVertexOutput input) : SV_TARGET
{
    const float2 ndc =
        float2(
            input.uv.x *
                2.0f -
                1.0f,

            (
                1.0f -
                input.uv.y
            ) *
                2.0f -
                1.0f);

    float4 worldFar =
        mul(
            float4(
                ndc,
                1.0f,
                1.0f),
            inverseViewProjection);

    worldFar.xyz /=
        max(
            abs(
                worldFar.w),
            0.00001f);

    const float3 viewDirection =
        normalize(
            worldFar.xyz -
            cameraPosition.xyz);

    const float height =
        saturate(
            viewDirection.y *
                0.5f +
            0.5f);

    const bool weatherEnabled =
        weatherFogParameters.x > 0.5f;

    float3 colour =
        weatherEnabled
            ? BuildWeatherSky(viewDirection)
            : BuildFallbackSky(viewDirection);

    const float currentTime =
        frac(
            skyAtmosphere1.z /
            24.0f);

    if (!weatherEnabled &&
        skyAtmosphere1.w > 0.5f)
    {
        const float2 gradientUv =
            float2(
                currentTime,
                1.0f -
                    height);

        const float3 gradient =
            skyGradientTexture.SampleLevel(
                terrainTextureSampler,
                gradientUv,
                0.0f).rgb;

        colour =
            lerp(
                colour,
                gradient,
                0.88f);
    }

    const float turbidity =
        saturate(
            skyAtmosphere0.y +
            skyAtmosphere0.z *
            (
                1.0f -
                height
            ));

    if (!weatherEnabled)
    {
        colour *=
            1.0f -
            turbidity * 0.12f;
    }

    const float3 sunDirection =
        normalize(
            skySunDirectionDaylight.xyz);

    const float sunAlignment =
        saturate(
            dot(
                viewDirection,
                sunDirection));

    const float miePower =
        max(
            12.0f,
            skyAtmosphere1.y *
                6.0f);

    const float mieGlow =
        pow(
            sunAlignment,
            miePower) *
        skyAtmosphere0.x *
        skySunDirectionDaylight.w;

    colour +=
        skySunColour.rgb *
        mieGlow *
        0.35f;

    const float sunDisk =
        pow(
            sunAlignment,
            max(
                256.0f,
                skyAtmosphere1.y *
                    64.0f)) *
        skySunDirectionDaylight.w;

    colour +=
        skySunColour.rgb *
        sunDisk *
        1.6f;

    return
        float4(
            colour,
            1.0f);
}