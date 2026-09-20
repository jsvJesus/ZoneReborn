#include "cb.hlsli"
#include "static_samplers.hlsli"
// ---------------------------------------------------------------------------
// Basic HLSL shader for MockVisual
// Simple vertex + pixel shader with MVP matrix and constant color
// ---------------------------------------------------------------------------


struct ObjectData
{

    //@semantic World
    float4x4 world;
    uint diffuseMap;
    uint materialID;
    uint2 _pad;//we will aligh structure in StructuredBuffer
};

struct MaterialData
{
    float4 color;
};

//@semantic Object
StructuredBuffer<ObjectData> _Objects : register(t0,space0);

//@semantic Material
StructuredBuffer<MaterialData> _Materials : register(t1,space0);


Texture2D g_Textures[] : register(t0, space1);

SamplerState g_Sampler : register(s0, space0);


// ---------------------------------------------------------------------------
// Vertex shader input/output
// ---------------------------------------------------------------------------
struct VSInput
{
    float3 pos : POSITION;
    float3 normal: NORMAL;
    float2 uv : TEXCOORD0;
    uint objectID : SV_InstanceID;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 normal : TEXCOORD0;
    float2 uv : TEXCOORD1; 
    uint diffuseMap : TEXCOORD2;
};

// ---------------------------------------------------------------------------
// Vertex shader
// ---------------------------------------------------------------------------
PSInput vs_main(VSInput input)
{
    PSInput output;
    ObjectData obj = _Objects[input.objectID];
    output.pos = mul(float4(input.pos, 1.0f),
                            mul(obj.world, viewProjection));
    output.normal = input.normal;
    output.uv = input.uv;
    output.diffuseMap = obj.diffuseMap;
    return output;
}


// ---------------------------------------------------------------------------
// Pixel shader
// ---------------------------------------------------------------------------
float4 ps_main(PSInput input) : SV_Target
{
    float4 texColor = g_Textures[input.diffuseMap].Sample(LinearWrap, input.uv);
    float3 lightDir = normalize(float3(0.5f, 1.0f, -0.3f));
    float NdotL = saturate(dot(normalize(input.normal), lightDir));
    float3 lit = texColor.rgb * (0.3f + 0.7f * NdotL);
    return float4(lit, texColor.a);
}