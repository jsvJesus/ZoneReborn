Texture2D BackgroundTexture : register(t0);
SamplerState BackgroundSampler : register(s0);


struct BackgroundVertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};


BackgroundVertexOutput VSBackground(
    uint vertexId : SV_VertexID)
{
    BackgroundVertexOutput output;

    //
    // Fullscreen triangle.
    //
    // 0 -> (0, 0)
    // 1 -> (2, 0)
    // 2 -> (0, 2)
    //
    float2 uv =
        float2(
            (vertexId << 1) & 2,
            vertexId & 2);

    output.position =
        float4(
            uv.x * 2.0f - 1.0f,
            1.0f - uv.y * 2.0f,
            0.0f,
            1.0f);

    output.uv =
        uv;

    return output;
}


float4 PSBackground(
    BackgroundVertexOutput input)
    : SV_TARGET
{
    return
        BackgroundTexture.Sample(
            BackgroundSampler,
            input.uv);
}