cbuffer FlareConstants : register(b0)
{
    row_major float4x4 viewProjection;

    float4 sourcePositionSize;
    float4 flareColour;

    float4 flareParameters;

    float4 cameraPosition;
    float4 screenParameters;
};

Texture2D flareTexture :
    register(t0);

Texture2D sceneDepthTexture :
    register(t1);

SamplerState flareSampler :
    register(s0);

struct FlareVertexOutput
{
    float4 position :
        SV_POSITION;

    float2 uv :
        TEXCOORD0;

    float2 sourceUV :
        TEXCOORD1;

    float sourceDepth :
        TEXCOORD2;

    float valid :
        TEXCOORD3;
};

FlareVertexOutput VSFlare(
    uint vertexId : SV_VertexID)
{
    static const float2 corners[4] =
    {
        float2(-1.0f, -1.0f),
        float2(-1.0f,  1.0f),
        float2( 1.0f, -1.0f),
        float2( 1.0f,  1.0f)
    };

    static const float2 uvs[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    FlareVertexOutput output;

    const float4 sourceClip =
        mul(
            float4(
                sourcePositionSize.xyz,
                1.0f),
            viewProjection);

    const float valid =
        sourceClip.w >
            0.0001f
            ? 1.0f
            : 0.0f;

    const float safeW =
        max(
            sourceClip.w,
            0.0001f);

    const float3 sourceNdc =
        sourceClip.xyz /
        safeW;

    const float depthFactor =
        flareParameters.w;

    const float2 elementCentre =
        sourceNdc.xy *
        depthFactor;

    const float aspect =
        max(
            screenParameters.x /
                max(
                    screenParameters.y,
                    1.0f),
            0.001f);

    const float flareSize =
        sourcePositionSize.w;

    const float2 halfExtent =
        float2(
            flareSize /
                aspect,
            flareSize);

    const float2 corner =
        corners[
            vertexId];

    output.position =
        float4(
            elementCentre +
                corner *
                halfExtent,
            sourceNdc.z,
            1.0f);

    output.uv =
        uvs[
            vertexId];

    output.sourceUV =
        float2(
            sourceNdc.x *
                0.5f +
                0.5f,

            -sourceNdc.y *
                0.5f +
                0.5f);

    output.sourceDepth =
        sourceNdc.z;

    output.valid =
        valid;

    return output;
}

float SampleVisibility(
    float2 uv,
    float sourceDepth)
{
    const float sceneDepth =
        sceneDepthTexture.SampleLevel(
            flareSampler,
            uv,
            0.0f).r;

    return
        sceneDepth +
            0.0025f >=
        sourceDepth
            ? 1.0f
            : 0.0f;
}

float4 PSFlare(
    FlareVertexOutput input) : SV_TARGET
{
    if (input.valid <
        0.5f)
    {
        discard;
    }

    if (input.sourceDepth <=
            0.0f ||
        input.sourceDepth >=
            1.0f)
    {
        discard;
    }

    if (input.sourceUV.x <=
            0.0f ||
        input.sourceUV.x >=
            1.0f ||
        input.sourceUV.y <=
            0.0f ||
        input.sourceUV.y >=
            1.0f)
    {
        discard;
    }

    const float2 texelSize =
        1.0f /
        max(
            screenParameters.xy,
            float2(
                1.0f,
                1.0f));

    const float sampleArea =
        max(
            flareParameters.y,
            1.0f);

    const float2 sampleOffset =
        texelSize *
        sampleArea *
        2.0f;

    float visibility =
        0.0f;

    visibility +=
        SampleVisibility(
            input.sourceUV,
            input.sourceDepth);

    visibility +=
        SampleVisibility(
            input.sourceUV +
                float2(
                    sampleOffset.x,
                    0.0f),
            input.sourceDepth);

    visibility +=
        SampleVisibility(
            input.sourceUV -
                float2(
                    sampleOffset.x,
                    0.0f),
            input.sourceDepth);

    visibility +=
        SampleVisibility(
            input.sourceUV +
                float2(
                    0.0f,
                    sampleOffset.y),
            input.sourceDepth);

    visibility +=
        SampleVisibility(
            input.sourceUV -
                float2(
                    0.0f,
                    sampleOffset.y),
            input.sourceDepth);

    visibility /=
        5.0f;

    visibility =
        pow(
            saturate(
                visibility),
            max(
                flareParameters.z,
                0.001f));

    const float distanceToSource =
        distance(
            cameraPosition.xyz,
            sourcePositionSize.xyz);

    float distanceFade =
        1.0f;

    const float maxDistance =
        flareParameters.x;

    if (maxDistance >
        0.001f)
    {
        distanceFade =
            1.0f -
            smoothstep(
                maxDistance *
                    0.75f,
                maxDistance,
                distanceToSource);
    }

    const float edgeDistance =
        min(
            min(
                input.sourceUV.x,
                1.0f -
                    input.sourceUV.x),
            min(
                input.sourceUV.y,
                1.0f -
                    input.sourceUV.y));

    const float edgeFade =
        saturate(
            edgeDistance *
            20.0f);

    const float4 textureSample =
        flareTexture.Sample(
            flareSampler,
            input.uv);

    const float intensity =
        flareColour.a *
        textureSample.a *
        visibility *
        distanceFade *
        edgeFade;

    return float4(
        textureSample.rgb *
            flareColour.rgb *
            intensity,
        intensity);
}