#include "cb.hlsli"

struct ObjectData
{
#if defined(RECT)
    float4 rect;
    float4 color;
    float depth;
#endif
#if defined(LINE)
    float3 start;
    float3 end;
    float4 color;
#endif    
};


//@semantic Object
StructuredBuffer<ObjectData> _Objects : register(t0,space0);

// ---------------------------------------------------------------------------
// Vertex shader input/output
// ---------------------------------------------------------------------------
struct VSInput
{
    float3 pos : POSITION;
    uint objectID : SV_InstanceID;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : TEXCOORD0; // передаем цвет в пиксель
};

// ---------------------------------------------------------------------------
// Vertex shader
// ---------------------------------------------------------------------------

#if defined(RECT)
PSInput vs_rect(VSInput input)
{
    PSInput output;
    ObjectData inst = _Objects[input.objectID];
    float2 pos;

    pos.x = inst.rect.x + input.pos.x * inst.rect.z;
    pos.y = inst.rect.y + input.pos.y * inst.rect.w;

    output.position = float4(pos.xy, inst.depth, 1.0);
    output.color = inst.color;
    return output;
}
#endif

#if defined(LINE)
PSInput vs_line(VSInput input)
{
    PSInput output;
    ObjectData inst = _Objects[input.objectID];

    float3 dir = inst.end - inst.start;

    float3 worldPos = inst.start + dir * input.pos.z;
    output.position = mul(float4(worldPos,1),viewProjection);
    output.color = inst.color;

    return output;
}

#endif

// ---------------------------------------------------------------------------
// Pixel shader
// ---------------------------------------------------------------------------
float4 ps_main(PSInput input) : SV_Target
{
    return input.color; 
}


//////////////////////////////////LINE//////////////////////////////////////////////




