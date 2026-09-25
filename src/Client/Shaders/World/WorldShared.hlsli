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

    uint useTerrain;
    uint terrainLayerCount;
    uint useModelTexture;
    uint useWater;

    float4 modelParameters;
    float4 modelTint;
    float4 modelOverlayColour;
    float4 modelOverlayParameters;

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
};

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

SamplerState terrainTextureSampler : register(s0);
SamplerState terrainBlendSampler : register(s1);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 localPosition : TEXCOORD0;
    float2 terrainUV : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
};

#endif
