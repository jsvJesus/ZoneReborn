#ifndef WORLD_SHARED_HLSLI
#define WORLD_SHARED_HLSLI

cbuffer SceneConstants : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 viewProjection;
    row_major float4x4 inverseViewProjection;

    float4 terrainU[4];
    float4 terrainV[4];

    float4 groupColour;
    float4 instanceColour;

    uint useTerrain;
    uint terrainLayerCount;
    uint useModelTexture;
    uint useWater;

    float4 modelParameters;
    float4 modelSkinColour;
    float4 modelHairColour;
    float4 modelDyeColour;
    float4 modelOverlayColour;
    float4 modelOverlayParameters;
    float4 modelTattooColour;
    // x: lightmap bound, y: X-Ray terrain layout, z: X-Ray lighting.
    float4 modelLightmapParameters;

    float4 waterDeepColour;
    float4 waterReflectionTint;
    float4 waterRefractionTint;

    float4 waterParameters0;
    float4 waterParameters1;
    float4 waterScrollSpeed1;
    float4 waterScrollSpeed2;
    float4 waterParameters2;
    float4 waterParameters3;

    float4 cameraPosition;
    float4 screenParameters;
};

struct OmniLightData
{
    float4 positionOuterRadius;
    float4 colourMultiplier;
    float4 parameters;
};

cbuffer OmniLightConstants : register(b1)
{
    OmniLightData omniLights[32];

    uint omniLightCount;
    uint omniLightPadding0;
    uint omniLightPadding1;
    uint omniLightPadding2;
};

struct SpotLightData
{
    float4 positionOuterRadius;
    float4 directionCosConeAngle;
    float4 colourMultiplier;
    float4 parameters;
};

cbuffer SpotLightConstants : register(b2)
{
    SpotLightData spotLights[32];

    uint spotLightCount;
    uint spotLightPadding0;
    uint spotLightPadding1;
    uint spotLightPadding2;
};

cbuffer SkyConstants : register(b3)
{
    float4 skySunDirectionDaylight;
    float4 skySunColour;
    float4 skyAmbientColour;

    float4 skyAtmosphere0;
    float4 skyAtmosphere1;
    float4 skyHemisphereColour;
    float4 weatherSkyColour;

    // xyz = fog RGB, w = fog distance
    float4 weatherFogColourDistance;

    // x = enabled
    // y = density
    // z = far plane
    // w = rain density
    float4 weatherFogParameters;

    // rgb = clouds RGB
    // w = clouds multiplier
    float4 weatherCloudColour;

    // x = clouds parameter
    // y = wind velocity
    // z = wind direction
    // w = elapsed seconds
    float4 weatherCloudParameters;
};

float3 ApplyWeatherFog(
    float3 colour,
    float3 worldPosition)
{
    if (weatherFogParameters.x < 0.5f)
        return colour;

    const float fogDistance =
        max(
            1.0f,
            min(
                weatherFogColourDistance.w,
                weatherFogParameters.z));

    const float density =
        clamp(
            weatherFogParameters.y,
            0.0f,
            10.0f);

    const float distanceToCamera =
        distance(
            worldPosition,
            cameraPosition.xyz);

    const float fogStart =
        fogDistance * 0.08f;

    const float linearFog =
        saturate(
            (distanceToCamera - fogStart) /
            max(fogDistance - fogStart, 1.0f));

    const float fogExponent =
        max(
            0.3f,
            1.4f - density * 0.85f);

    const float fogFactor =
        pow(
            linearFog,
            fogExponent);

    return lerp(
        colour,
        weatherFogColourDistance.rgb,
        fogFactor);
}

Texture2D terrainTexture0 : register(t0);
Texture2D terrainTexture1 : register(t1);
Texture2D terrainTexture2 : register(t2);
Texture2D terrainTexture3 : register(t3);

Texture2D terrainBlend : register(t4);
Texture2D modelTexture : register(t5);

Texture2D sceneColourTexture : register(t6);
Texture2D sceneDepthTexture : register(t7);
Texture2D waterNormalTexture : register(t8);
Texture2D waterFoamTexture : register(t9);
Texture2D skyGradientTexture : register(t10);
Texture2D modelOverlayTexture : register(t11);
Texture2D modelDyeMaskTexture : register(t12);
Texture2D modelTattooTexture : register(t13);
Texture2D modelLightmapTexture : register(t14);

SamplerState terrainTextureSampler : register(s0);
SamplerState terrainBlendSampler : register(s1);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float2 lightmapTexcoord : TEXCOORD1;
    float2 bakedLighting : TEXCOORD2;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 localPosition : TEXCOORD0;
    float2 terrainUV : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float2 lightmapUV : TEXCOORD3;
    float2 bakedLighting : TEXCOORD4;
};

#endif
