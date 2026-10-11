#include "WorldShared.hlsli"

struct DetailInstance
{
    float4 positionScale;
    float4 rotationLighting;
};
StructuredBuffer<DetailInstance> detailInstances : register(t19);
cbuffer DetailConstants : register(b4)
{
    // min Y, inverse model height, waving enabled, first instance.
    float4 detailModel;
    // elapsed seconds, wind velocity, wind direction (radians), draw distance.
    float4 detailWeather;
};

PixelInput VSMain(VertexInput input, uint instanceId : SV_InstanceID)
{
    PixelInput output;
    DetailInstance instance = detailInstances[(uint)detailModel.w + instanceId];
    float3 base = instance.positionScale.xyz;
    float c = instance.rotationLighting.x, s = instance.rotationLighting.y;
    float fade = saturate((detailWeather.w-distance(base,cameraPosition.xyz))/4.0f);
    float3 local = input.position*instance.positionScale.w*fade;
    float3 position = base+float3(local.x*c+local.z*s,local.y,-local.x*s+local.z*c);
    float height = saturate((input.position.y-detailModel.x)*detailModel.y);
    float phase = detailWeather.x*1.8f+base.x*0.65f+base.z*0.43f;
    float sway = sin(phase)*(0.03f+min(max(detailWeather.y,0.0f),20.0f)*0.008f);
    position.xz += float2(sin(detailWeather.z),cos(detailWeather.z))*sway*height*detailModel.z*fade;
    output.position = mul(float4(position,1.0f),viewProjection);
    output.normal = normalize(float3(input.normal.x*c+input.normal.z*s,input.normal.y,
        -input.normal.x*s+input.normal.z*c));
    output.localPosition = input.position;
    output.terrainUV = input.texcoord;
    output.lightmapUV = input.lightmapTexcoord;
    output.bakedLighting = instance.rotationLighting.zw;
    output.worldPosition = position;
    return output;
}
