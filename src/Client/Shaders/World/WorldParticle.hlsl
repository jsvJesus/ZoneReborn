cbuffer ParticleConstants : register(b0)
{
    row_major float4x4 viewProjection;
};

Texture2D particleTexture :
    register(t0);

SamplerState particleSampler :
    register(s0);

struct ParticleVertexInput
{
    float3 position :
        POSITION;

    float2 uv :
        TEXCOORD0;

    float4 colour :
        COLOR0;
};

struct ParticleVertexOutput
{
    float4 position :
        SV_POSITION;

    float2 uv :
        TEXCOORD0;

    float4 colour :
        COLOR0;
};

ParticleVertexOutput VSParticle(
    ParticleVertexInput input)
{
    ParticleVertexOutput output;

    output.position =
        mul(
            float4(
                input.position,
                1.0f),
            viewProjection);

    output.uv =
        input.uv;

    output.colour =
        input.colour;

    return output;
}

float4 PSParticle(
    ParticleVertexOutput input) : SV_TARGET
{
    const float4 textureSample =
        particleTexture.Sample(
            particleSampler,
            input.uv);

    const float alpha =
        saturate(
            textureSample.a *
            input.colour.a);

    if (alpha <=
        0.001f)
    {
        discard;
    }

    return float4(
        textureSample.rgb *
            input.colour.rgb,
        alpha);
}