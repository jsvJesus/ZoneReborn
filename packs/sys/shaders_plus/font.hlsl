#include "cb.hlsli"


// ---------------------------------------------------------------------------
// Shader for instanced font rendering 
// ---------------------------------------------------------------------------
struct ObjectData
{
    float2 pos;
    float2 size;
    float4 uvRect;
    float4 color;
    uint atlasID;
};


struct FontData
{
    uint atlas;
};


//@semantic Object
StructuredBuffer<ObjectData> _glyphs : register(t0,space0);

//@semantic Material
StructuredBuffer<FontData> _Materials : register(t1,space0);


Texture2D g_Textures[] : register(t0, space1);

SamplerState g_Sampler : register(s0, space0);


// ---------------------------------------------------------------------------
// Vertex shader input/output(поступать должен чисти quad дефолный)
// ---------------------------------------------------------------------------
struct VSInput
{
    float3 pos : POSITION;
    uint glyphsID : SV_InstanceID;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0; // передаем цвет в пиксель
    nointerpolation uint atlasID : TEXCOORD1;
    nointerpolation float4 color : TEXCOORD2;
};


// ---------------------------------------------------------------------------
// Vertex shader
// ---------------------------------------------------------------------------
PSInput vs_main(VSInput input)
{
    PSInput output;
    ObjectData data = _glyphs[input.glyphsID];
        // quad assumed 0..1
    float2 local = input.pos.xy;
    // scale
    float2 scaled = local * data.size;
    // translate
    float4 clipPos = float4(scaled + data.pos, 0.0f, 1.0f);

   //TODO пока считаем что снап позиций на CPU - достаточен. там поглядим.

    output.pos = clipPos;
    float2 uv;
    uv.x = lerp(data.uvRect.x, data.uvRect.z, local.x);
    uv.y = lerp(data.uvRect.y,data.uvRect.w, local.y);
    output.atlasID =  data.atlasID;
    output.uv = uv;
    output.color = data.color;
    return output;
}

float4 ps_main(PSInput input) : SV_Target
{
    float4 glyphColor = g_Textures[input.atlasID].Sample(g_Sampler, input.uv);
    return glyphColor*input.color;
}