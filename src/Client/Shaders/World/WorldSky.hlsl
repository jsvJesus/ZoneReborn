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

    float3 colour =
        BuildFallbackSky(
            viewDirection);

    const float currentTime =
        frac(
            skyAtmosphere1.z /
            24.0f);

    if (skyAtmosphere1.w >
        0.5f)
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

    colour *=
        1.0f -
        turbidity *
            0.12f;

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