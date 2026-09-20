#ifndef __STATIC_SAMPLERS__
#define __STATIC_SAMPLERS__



//point sampling
SamplerState PointWrap    : register(s0);
SamplerState PointClamp   : register(s1);

// Linear sampling
SamplerState LinearWrap   : register(s2);
SamplerState LinearClamp  : register(s3);

// Anisotropic sampling
SamplerState AnisoWrap    : register(s4);
SamplerState AnisoClamp   : register(s5);

// Comparison sampler (для shadow maps)
SamplerComparisonState ShadowCmp : register(s6);


#endif