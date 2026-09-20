#ifndef __CB_INCLUDE__
#define __CB_INCLUDE__

#include "macro.hlsli"

// ============================================================
//  Common Constant Buffer Definitions
//  Engine-wide contract. Layout must match C++ side exactly.
// ============================================================



// ============================================================
// Pass Buffer  (per render pass)
// ============================================================

#ifndef PASS_BUFFER_SLOT
    #define PASS_BUFFER_SLOT b0
#endif

//@semantic Pass
cbuffer _Pass : register(PASS_BUFFER_SLOT)
{
    //@semantic View
    float4x4 view;
    //@semantic InvView;
    float4x4 invView;
    //@semantic Projection
    float4x4 projection;
    //@semantic ViewProjection
    float4x4 viewProj;
    float4   resolution;
    //@semantic FarPlane;
    float4 farPlane;
    //@semantic FogColour
    //@default float4(0,0,0,0)
    float4 fogColour;
    //@semantic FakeDirectionalAmbientColour
    //@default float4(0,0,0,0)
    float4 g_gakeDirectionalAmbientColour;

    //@semantic SkyLightMapTransform
    float4 skyLightMapTransform[2];
    //@semantic CameraPos;
    float3 wsCameraPos;
    //@semantic FogStart
    //@default 0
    float fogStart;
    //@semantic FogEnd
    //@default 1
    float fogEnd;
    //@semantic Time    
    float time;  
}

struct DirectionalLight
{
    float3 direction;
    float4 colour;
};

struct PointLight
{
    float3 position;
    float4 colour;
    float3 attenuation;
};

struct SpotLight
{
    float3 position;
    float4 colour;
    float3 attenuation;
    float3 direction;
};

struct PlaneLight
{
    float3 position;
    float2 attenuation;
    float4 normalsD[6]; // xyz - normal, w - расстояние до плоскости d
};

struct AmbientPointLight
{
    float3 position;
    float4 colour;
    float2 attenuation;
};


struct MaterialData
{
    #if defined(BLOOMED)

    //@editable
    //@ui_name "Bloom Power"
    //@ui_desc ""
    //@ui_min  0
    //@ui_max  10
    //@ui_digits 2
    //@ui_group "Bloom(only in WorldEditor)"
    //@default 1.0
    float bloomPower;

#if defined( SPECUL_COLOR ) || defined( SPECULAR_MAP )
    //@editable
#endif
    //@ui_name "Bloom Only Specular"
    //@ui_desc ""
    //@ui_group "Bloom(only in WorldEditor)"
    //@default false
    bool bloomOnlySpecular;

    //@editable
    //@ui_name "Mask mode"
    //@ui_desc "Bloom mask mode"
    //@ui_min  1
    //@ui_max  5
    //@ui_digits 2
    //@ui_group "Bloom(only in WorldEditor)"
    //@default 1.0
    int bloomMaskMode;

#endif


#if defined( SPECUL_COLOR ) || defined( SPECULAR_MAP )
    //@editable
#endif
    //@ui_name "Specular Power"
    //@ui_desc ""
    //@ui_min 0
    //@ui_max 64.0
    //@ui_digits 0
    //@default 32
    float specPower;

<
#if defined( SPECUL_COLOR ) || defined( SPECULAR_MAP )
    //@editable
#endif
    //@ui_name "Additional Specular Alpha"
    //@ui_desc ""
    //@ui_min  0
    //@ui_max  2.f
    //@ui_digits 2
    //default 0
    float additionalSpecularAlpha;

#if defined( SPECUL_COLOR )
    //@editable
#endif
    //@ui_widget "Color"
    //@ui_name "Specular Colour"
    //@ui_desc "The specular colour for the material"
    //@ui_group LOCALIZE("DECAL")
    //@ui_min  0
    //@ui_max  2.f
    //@ui_digits 1
    //default 0
    float4 materialSpecular;


#if defined( LIGHTMAP )
    //@editable
#endif
    //@ui_name "Lightmap strength, %"
    //@ui_max    300
    //@ui_min    0
    //@ui_digits 0
    //@default 100.f
    float _lm_strength;

    #if defined( LIGHTMAP )
        //@editable
    #endif
    //@ui_name = ""Diffuse falloff, %"
    //@ui_max 100
    //@default 9
    float _lm_falloff;

#if defined(CHROME)

    //@editable
    //@ui_name "Fresnel Falloff"
    //@ui_desc "Fresnel term edging"
    //@ui_group "Chrome"
    //@ui_min 1.0
    //@ui_max 7.0
    //@ui_digits 2
    //@default 5.0
    float fresnelExp;


    //@editable
    //@ui_name "Fresnel Constant"
    //@ui_desc "Fresnel constant"
    //@ui_group "Chrome"
    //@ui_min 0.0
    //@ui_max 0.5
    //@ui_digits 4
    //@default 0.5
    float fresnelConstant;


    //@editable
    //@ui_name "Reflection Amount"
    //@ui_desc "Scaling factor for reflection"
    //@ui_group "Chrome"
    //@ui_min 0.0
    //@ui_max 2.0
    //@ui_digits 2
    //@default 1.0
    float reflectionAmount;

    //@editable
    //@ui_name "Dynamic Cubemap"
    //@ui_desc "Use dynamic cubemap"
    //@ui_group "Chrome"
    //@default false
    bool isDynamicCubeMap;

#endif

#if defined(GLOW)

    //@editable
    //@ui_name LOCALIZE("GLOW/GLOW_FACTOR")
    //@ui_desc "Glow Factor"
    //@ui_group LOCALIZE("GLOW/GROUP")
    //@ui_min 0.0
    //@ui_max 2.0
    //@ui_digits 1
    //@default 0.0
    float glowFactor;

#endif

#if defined(COLOUR_MASK)

    //@editable
    //@ui_name "Offset Green Blue"
    //@ui_desc "Green Blue channel offset"
    //@ui_group "Colour Mask"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default float2(0,0)
    float2 offsetGreenBlue;


    //@editable
    //@ui_name "Offset Red Green"
    //@ui_desc "Red Green channel offset"
    //@ui_group "Colour Mask"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default float2(0,0)
    float2 offsetRedGreen;


    //@editable
    //@ui_name "Offset Blue"
    //@ui_desc "Blue channel offset"
    //@ui_group "Colour Mask"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default float2(0,0)
    float2 offsetBlue;


    //@editable
    //@ui_name "Offset Green"
    //@ui_desc "Green channel offset"
    //@ui_group "Colour Mask"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default float2(0,0)
    float2 offsetGreen;


    //@editable
    //@ui_name "Offset Red"
    //@ui_desc "Red channel offset"
    //@ui_group "Colour Mask"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default float2(0,0)
    float2 offsetRed;


    //@editable
    //@ui_name "UV2 scale"
    //@ui_group "Colour Mask"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default float2(1,1)
    float2 maskedTextureScale;

#endif

#ifdef INVISIBLE_WALL
    //@semantic InvisibleWallsAlpha
    //@default 0.5f;
    float invWallsAlpha;
#endif

#if defined(WAVE_ANIMATION)

    //@editable
    //@ui_name "Inverse Wave Origin Direction"
    //@ui_group "Wave Animation"
    //@default false
    int inverseDirection;


    //@editable
    //@ui_name "Max Displacement"
    //@ui_desc "Maximum wave displacement"
    //@ui_group "Wave Animation"
    //@ui_widget "Slider"
    //@ui_min 0
    //@ui_max 20
    //@ui_digits 2
    //@default 1.0
    float waveMaxDisplacement;


    //@editable
    //@ui_name "Z Wave Offset"
    //@ui_group "Wave Animation"
    //@ui_widget "Slider"
    //@ui_min -10
    //@ui_max 10
    //@ui_digits 2
    //@default 0.0
    float waveOffsetZ;


    //@editable
    //@ui_name "Wave Axis Angle"
    //@ui_group "Wave Animation"
    //@ui_widget "Slider"
    //@ui_min -180
    //@ui_max 180
    //@ui_digits 0
    //@default 90.0
    float waveBoundAxisAngle;


    //@editable
    //@ui_name "Wave Height"
    //@ui_group "Wave Animation/Delayed Axis"
    //@ui_widget "Slider"
    //@ui_min -10
    //@ui_max 10
    //@ui_digits 2
    //@default 0.5
    float waveDelayedHeight;


    //@editable
    //@ui_name "Wave Height"
    //@ui_group "Wave Animation/Main Axis"
    //@ui_widget "Slider"
    //@ui_min -10
    //@ui_max 10
    //@ui_digits 2
    //@default 1.0
    float waveMainHeight;


    //@editable
    //@ui_name "Wave Power"
    //@ui_group "Wave Animation/Delayed Axis"
    //@ui_widget "Slider"
    //@ui_min -10
    //@ui_max 10
    //@ui_digits 1
    //@default 1.0
    float waveDelayOffset;


    //@editable
    //@ui_name "Wave Speed"
    //@ui_group "Wave Animation"
    //@ui_widget "Slider"
    //@ui_min 0
    //@ui_max 100
    //@ui_digits 2
    //@default 1.0
    float waveSpeed;


    //@editable
    //@ui_name "Wave Direction"
    //@ui_group "Wave Animation"
    //@default float3(1,0,0)
    float3 waveDirection;


    //@editable
    //@ui_name "Wave Power"
    //@ui_group "Wave Animation/Main Axis"
    //@ui_widget "Slider"
    //@ui_min 0
    //@ui_max 50
    //@ui_digits 1
    //@default 1.0
    float wavePower;


    //@editable
    //@ui_name "Wind Importance"
    //@ui_group "Wave Animation/Wind"
    //@ui_widget "Slider"
    //@ui_min 0
    //@ui_max 1
    //@ui_digits 2
    //@default 0.0
    float waveWindImportance;


    //@editable
    //@ui_name "Wind Power"
    //@ui_group "Wave Animation/Wind"
    //@ui_widget "Slider"
    //@ui_min -50
    //@ui_max 50
    //@ui_digits 1
    //@default 1.0
    float waveWindPower;


    //@semantic Wind
    float4 wind;


    //@editable
    //@ui_name "Enable Debug Axis"
    //@ui_group "Wave Animation/Debug"
    //@default false
    int enableDebugAxis;


    //@editable
    //@ui_name "Axis Thickness"
    //@ui_group "Wave Animation/Debug"
    //@ui_widget "Slider"
    //@ui_min 0
    //@ui_max 1
    //@default 0.01
    float debugAxisThickness;

#endif


#ifdef TRANSFORM_DIFFUSE_UV
    //@editable
    //@ui_name "U Transform Diffuse"
    //@ui_desc "The U-transform vector for the diffuse material"
    //@ui_widget "Spinner"
    //@ui_min -100
    //@ui_max 100
    //@default float4(1,0,0,0)
    float4 uTransform_diffuse;

    //@editable
    //@ui_name "V Transform Diffuse"
    //@ui_desc "The V-transform vector for the diffuse material"
    //@ui_widget "Spinner"
    //@ui_min -100
    //@ui_max 100
    //@default float4(0,1,0,0)
    float4 vTransform_diffuse;
#endif

#ifdef UV_TRANSFORM

    //@editable
    //@ui_name "Texture Operation"
    //@ui_desc "D3D Texture Stage operation to use for blending the layer"
    //@enum_type "TEXTUREOP"
    //@ui_min 1
    //@ui_max 27
    //@ui_digits 2
    //@default 18
    int textureOperation;

    //@editable
    //@ui_name "U Transform"
    //@ui_desc "The U-transform vector for the material"
    //@ui_widget "Spinner"
    //@ui_min -100
    //@ui_max 100
    //@default float4(1,0,0,0)
    float4 uTransform;

    //@editable
    //@ui_name "V Transform"
    //@ui_desc "The V-transform vector for the material"
    //@ui_widget "Spinner"
    //@ui_min -100
    //@ui_max 100
    //@default float4(0,1,0,0)
    float4 vTransform;

    #ifdef GLOW_BY_BRIGHTNESS
    //@editable
    //@ui_name "Brightness Amplification"
    //@ui_desc "Amplification vector for glow by brightness"
    //@ui_group "Glow by Brightness"
    //@default float4(0,0,0,0)
    float4 brightnessAmpl;

    //@editable
    //@ui_name "Brightness Offset"
    //@ui_desc "Offset for glow by brightness"
    //@ui_group "Glow by Brightness"
    //@default 1.0
    float brightnessOffset;

    //@editable
    //@ui_name "Brightness Scale"
    //@ui_desc "Scaling factor for glow by brightness"
    //@ui_group "Glow by Brightness"
    //@default 1.0
    float brightnessScale;
    #endif
#endif
#ifdef PIXEL_UP
    //@editable
    //@ui_name "Pitch Angle"
    //@ui_group "Pixel Up"
    //@ui_min -180.0
    //@ui_max 180.0
    //@ui_digits 2
    //@default -90.0
    float _PixelUpPitch;

    //@editable
    //@ui_name "Yaw Angle"
    //@ui_group "Pixel Up"
    //@ui_min -180.0
    //@ui_max 180.0
    //@ui_digits 2
    //@default 0.0
    float _PixelUpYaw;

    //@editable
    //@ui_name "Blend Roughness"
    //@ui_group "Pixel Up"
    //@ui_min 1.0
    //@ui_max 50.0
    //@ui_digits 2
    //@default 1.0
    float _PixelUpBlendRoughness;

    //@editable
    //@ui_name "Power"
    //@ui_group "Pixel Up"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default 0.0
    float _PixelUpPower;

    //@editable
    //@ui_name "Scale"
    //@ui_desc "Scale pixel up texture"
    //@ui_group "Pixel Up"
    //@ui_widget "Slider"
    //@ui_min 0.01
    //@ui_max 10.0
    //@ui_digits 2
    //@default 1.0
    float g_pixelUP_scale;

    //@editable
    //@ui_name "Horizontal shift(U-shift)"
    //@ui_desc "Offset pixel up mask texture"
    //@ui_group "Pixel Up/Mask"
    //@ui_min 0.0
    //@ui_max 100.0
    //@ui_digits 2
    //@default 0.0
    float _MaskHorizontalShift;

    //@editable
    //@ui_name "Vertical shift(V-shift)"
    //@ui_desc "Offset pixel up mask texture"
    //@ui_group "Pixel Up/Mask"
    //@ui_min 0.0
    //@ui_max 100.0
    //@ui_digits 2
    //@default 0.0
    float _MaskVerticalShift;

    //@editable
    //@ui_name "Horizontal Scale"
    //@ui_desc "Horizontal scale pixel up mask texture"
    //@ui_group "Pixel Up/Mask"
    //@ui_widget "Slider"
    //@ui_min 0.01
    //@ui_max 10.0
    //@ui_digits 2
    //@default 1.0
    float _MaskHorizontalScale;

    //@editable
    //@ui_name "Vertical Scale"
    //@ui_desc "Vertical scale pixel up mask texture"
    //@ui_group "Pixel Up/Mask"
    //@ui_widget "Slider"
    //@ui_min 0.01
    //@ui_max 10.0
    //@ui_digits 2
    //@default 1.0
    float _MaskVerticalScale;

    //@editable
    //@ui_name "Scale"
    //@ui_desc "Scale pixel up mask texture"
    //@ui_group "Pixel Up/Mask"
    //@ui_widget "Slider"
    //@ui_min 0.01
    //@ui_max 10.0
    //@ui_digits 2
    //@default 1.0
    float _MaskScale;

    //@editable
    //@ui_name "Use Mask"
    //@ui_desc "Enable Using Mask"
    //@ui_group "Pixel Up/Mask"
    //@default false
    bool _UseMask;
#endif
//-------------------------Self Illumination------------------------------------------
    //@editable
    //@ui_name "Self Illumination"
    //@ui_desc "The self illumination factor for the material"
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 1
    //@default 0.0
float selfIllumination;

//-------------------------Colourise------------------------------------------
#if defined(COLOURISE) || defined(COLOURISE_TEXTURE)
    //@editable
    //@ui_name "Scale Mask"
    //@ui_desc "Scale Mask"
    //@ui_min 0.0
    //@ui_max 100.0
    //@ui_digits 1
    //@default 1.0
float gScale;

    //@editable
    //@ui_name "U Shift Mask"
    //@ui_desc "Shift U mask"
    //@ui_min -10.0
    //@ui_max 10.0
    //@ui_digits 2
    //@default 0.0
float gShiftMaskU;

    //@editable
    //@ui_name "V Shift Mask"
    //@ui_desc "Shift V mask"
    //@ui_min -10.0
    //@ui_max 10.0
    //@ui_digits 2
    //@default 0.0
float gShiftMaskV;

#ifdef COLOURISE_TEXTURE
    //@editable
    //@ui_name "is Colourise diff 0"
    //@ui_desc "Colourise diffuse 0 ?"
    //@default false
    bool gColouriseDiffuse0;

    //@editable
    //@ui_name "is Colourise diff 1"
    //@ui_desc "Colourise diffuse 1 ?"
    //@default false
    bool gColouriseDiffuse1;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Colour 0"
    //@ui_desc "Dirt colour 0"
    float4 dirtColour0;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Dirt Spec 0"
    //@ui_desc "Dirt Specular 0"
    float4 dirtSpec0;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Colour 1"
    //@ui_desc "Dirt colour 1"
    float4 dirtColour1;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Dirt Spec 1"
    //@ui_desc "Dirt Specular 1"
    float4 dirtSpec1;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Colour 2"
    //@ui_desc "Dirt colour 2"
    float4 dirtColour2;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Dirt Spec 2"
    //@ui_desc "Dirt Specular 2"
    float4 dirtSpec2;
#endif // COLOURISE_TEXTURE

#ifdef COLOURISE
    //@editable
    //@ui_name "U Transform Mask"
    //@ui_desc "Transform U Mask"
    //@ui_min 0.0
    //@ui_max 100.0
    //@ui_digits 1
    //@default 1.0
    float gScaleU;

    //@editable
    //@ui_name "V Transform Mask"
    //@ui_desc "Transform V Mask"
    //@ui_min 0.0
    //@ui_max 100.0
    //@ui_digits 1
    //@default 1.0
    float gScaleV;

    //@editable
    //@ui_name "Rotate Mask"
    //@ui_desc "Rotate Mask"
    //@ui_min 0.0
    //@ui_max 360.0
    //@ui_digits 0
    //@default 0.0
    float gRotateMask;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Colour R"
    //@ui_desc "Custom colour for channel R"
    float4 colouriseR;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Colour G"
    //@ui_desc "Custom colour for channel G"
    float4 colouriseG;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Colour B"
    //@ui_desc "Custom colour for channel B"
    float4 colouriseB;

    //@editable
    //@ui_widget "Color"
    //@ui_name "Colour A"
    //@ui_desc "Custom colour for channel A"
    float4 colouriseA;
#endif // COLOURISE
#endif // COLOURISE || COLOURISE_TEXTURE

//-------------------------Decal------------------------------------------
#ifdef DECAL
    //@editable
    //@ui_name "Scale Decal"
    //@ui_desc "Scale Decal"
    //@ui_group LOCALIZE("DECAL")
    //@ui_min 0.1
    //@ui_max 20.0
    //@ui_digits 1
    //@default 1.0
    float gScaleDecal;

    //@editable
    //@ui_name "U Shift Decal"
    //@ui_desc "Shift U Decal"
    //@ui_group LOCALIZE("DECAL")
    //@ui_min -1.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default 0.0
    float gShiftDecalU;

    //@editable
    //@ui_name "V Shift Decal"
    //@ui_desc "Shift V Decal"
    //@ui_group LOCALIZE("DECAL")
    //@ui_min -1.0
    //@ui_max 1.0
    //@ui_digits 2
    //@default 0.0
    float gShiftDecalV;

    //@editable
    //@ui_name "Rotate Decal"
    //@ui_desc "Rotate around the origin"
    //@ui_group LOCALIZE("DECAL")
    //@ui_min 0.0
    //@ui_max 360.0
    //@ui_digits 1
    //@default 0.0
    float gRotateAngleDecal;
#endif // DECAL

#if defined(MULTI_TEX) || defined(COLOURISE_TEXTURE)

#ifdef MULTI_TEX
    // ----------------------- Texture Layer 2 -----------------------
    //@ui_group LOCALIZE("TEXTURE_LAYER_2/GROUP")
    TRANSFORM_BUNDLE(_shiftX, "SHIFT_U",
                     _shiftY, "SHIFT_V",
                     _transformX, "SCALE_U",
                     _transformY, "SCALE_V",
                     _scale, "SCALE_UV",
                     _Grad_angle, "ROTATE_UV", "");

    TRANSFORM_BUNDLE(_shiftX2, "SHIFT_U",
                     _shiftY2, "SHIFT_V",
                     _transformX2, "SCALE_U",
                     _transformY2, "SCALE_V",
                     _scale2, "SCALE_UV",
                     _Grad_angle2, "ROTATE_UV", LOCALIZE("TEXTURE_LAYER_2/GROUP"));

    //@ui_name "Use Chrome"
    //@ui_desc "Use Chrome on Texture 2 layer"
    //@ui_group LOCALIZE("TEXTURE_LAYER_2/GROUP")
    //@default false
    bool _useChrome2;

    #ifdef USE_MASK
        TRANSFORM_BUNDLE(_shiftXMask2, "SHIFT_U",
                         _shiftYMask2, "SHIFT_V",
                         _transformXMask2, "SCALE_U",
                         _transformYMask2, "SCALE_V",
                         _scaleMask2, "SCALE_UV",
                         _Grad_angleMask2, "ROTATE_UV", LOCALIZE("TEXTURE_LAYER_2/GROUP")->LOCALIZE("MASK"));

        //@ui_name LOCALIZE("USE_FIRST_UV_FOR_MASK")
        //@ui_group LOCALIZE("TEXTURE_LAYER_2/GROUP")->LOCALIZE("MASK")
        //@default false
        bool _useFirstUVMask2;
    #endif
#endif // MULTI_TEX

// ----------------------- Texture Layer 1 -----------------------
TRANSFORM_BUNDLE(_shiftX1, "SHIFT_U",
                 _shiftY1, "SHIFT_V",
                 _transformX1, "SCALE_U",
                 _transformY1, "SCALE_V",
                 _scale1, "SCALE_UV",
                 _Grad_angle1, "ROTATE_UV", LOCALIZE("TEXTURE_LAYER_1/GROUP"));

    //@ui_name "Use Chrome"
    //@ui_desc "Use Chrome on Texture 1 layer"
    //@ui_group LOCALIZE("TEXTURE_LAYER_1/GROUP")
    //@default false
    bool _useChrome1;

#if defined(USE_MASK) || defined(COLOURISE_TEXTURE)
    TRANSFORM_BUNDLE(_shiftXMask1, "SHIFT_U",
                     _shiftYMask1, "SHIFT_V",
                     _transformXMask1, "SCALE_U",
                     _transformYMask1, "SCALE_V",
                     _scaleMask1, "SCALE_UV",
                     _Grad_angleMask1, "ROTATE_UV", LOCALIZE("TEXTURE_LAYER_1/GROUP")->LOCALIZE("MASK"));

    #if defined(MULTI_TEX)
        //@ui_name LOCALIZE("USE_FIRST_UV_FOR_MASK")
        //@ui_group LOCALIZE("TEXTURE_LAYER_1/GROUP")->LOCALIZE("MASK")
        //@default false
        bool _useFirstUVMask1;
    #endif

    //@ui_name "Mask Bump Strength"
    //@ui_desc "Mask bumping"
    //@ui_min -1.0
    //@ui_max 1.0
    //@ui_group LOCALIZE("TEXTURE_LAYER_1/GROUP")->LOCALIZE("MASK")
    //@default 0.0
    float maskBumpStrength;
#endif // USE_MASK || COLOURISE_TEXTURE

#ifdef COLOURISE_TEXTURE
    // ----------------------- Dirt Masks -----------------------
    TRANSFORM_BUNDLE(_shiftXDirtMask0, "U Shift dirt mask 0",
                     _shiftYDirtMask0, "V Shift dirt mask 0",
                     _transformXDirtMask0, "U Transform dirt mask 0",
                     _transformYDirtMask0, "V Transform dirt mask 0",
                     _scaleDirtMask0, "UV Scale dirt mask 0",
                     _rotateDirtMask0, "Rotate dirt mask 0", "");

    TRANSFORM_BUNDLE(_shiftXDirtMask1, "U Shift dirt mask 1",
                     _shiftYDirtMask1, "V Shift dirt mask 1",
                     _transformXDirtMask1, "U Transform dirt mask 1",
                     _transformYDirtMask1, "V Transform dirt mask 1",
                     _scaleDirtMask1, "UV Scale dirt mask 1",
                     _rotateDirtMask1, "Rotate dirt mask 1", "");

    TRANSFORM_BUNDLE(_shiftXDirtMask2, "U Shift dirt mask 2",
                     _shiftYDirtMask2, "V Shift dirt mask 2",
                     _transformXDirtMask2, "U Transform dirt mask 2",
                     _transformYDirtMask2, "V Transform dirt mask 2",
                     _scaleDirtMask2, "UV Scale dirt mask 2",
                     _rotateDirtMask2, "Rotate dirt mask 2", "");
#endif

#endif // MULTI_TEX || COLOURISE_TEXTURE

//-------------------------SubSurface--------------------------------------
#ifdef SUBSURFACE
    //@editable
    //@ui_name "Sub Surface Blend Power"
    //@ui_desc "The sub-surface blend power for the material"
    //@ui_min 0.0
    //@ui_max 2.0
    //@ui_digits 1
    //@default 1.0
    float subSurfaceBlendPower;
#endif // SUBSURFACE

//-------------------------Colour------------------------------------------
#ifdef COLOUR_ONLY
    //@editable
    //@ui_name "Colour"
    //@ui_desc "Colour"
    //@ui_widget "Color"
    float4 g_colour;
#endif
//-------------------------UV Scale----------------------------------------
#ifdef TEXEL_DENSITY
    //@editable
    //@ui_min 0.0
    //@ui_max 1.0
    //@ui_digits 1
    float4 g_UVScale;
#endif
//-------------------------LOD Man Colours---------------------------------
#ifdef MAN_LOD

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/CLOTHES/COLOR_JACKET")
    //@ui_desc LOCALIZE("LOD_MAN/CLOTHES/COLOR_JACKET")
    //@ui_group LOCALIZE("LOD_MAN/CLOTHES/GROUP")
    //@default 0.0
    float4 g_colourJacket;

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/CLOTHES/COLOR_BOOTS")
    //@ui_desc LOCALIZE("LOD_MAN/CLOTHES/COLOR_BOOTS")
    //@ui_group LOCALIZE("LOD_MAN/CLOTHES/GROUP")
    //@default 0.0
    float4 g_colourBoots;

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/CLOTHES/COLOR_LEGS")
    //@ui_desc LOCALIZE("LOD_MAN/CLOTHES/COLOR_LEGS")
    //@ui_group LOCALIZE("LOD_MAN/CLOTHES/GROUP")
    //@default 0.0
    float4 g_colourLegs;

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/CLOTHES/COLOR_ON_HEAD")
    //@ui_desc LOCALIZE("LOD_MAN/CLOTHES/COLOR_ON_HEAD")
    //@ui_group LOCALIZE("LOD_MAN/CLOTHES/GROUP")
    //@default 0.0
    float4 g_colourOnHead;

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/EQUIP/COLOR_BACKPACK")
    //@ui_desc LOCALIZE("LOD_MAN/EQUIP/COLOR_BACKPACK")
    //@ui_group LOCALIZE("LOD_MAN/EQUIP/GROUP")
    //@default 0.0
    float4 g_colourBackpack;

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/EQUIP/COLOR_ARMOR")
    //@ui_desc LOCALIZE("LOD_MAN/EQUIP/COLOR_ARMOR")
    //@ui_group LOCALIZE("LOD_MAN/EQUIP/GROUP")
    //@default 0.0
    float4 g_colourArmor;

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/EQUIP/COLOR_FACE")
    //@ui_desc LOCALIZE("LOD_MAN/EQUIP/COLOR_FACE")
    //@ui_group LOCALIZE("LOD_MAN/EQUIP/GROUP")
    //@default 0.0
    float4 g_colourFace;

    //@editable
    //@ui_widget "Color"
    //@ui_name LOCALIZE("LOD_MAN/EQUIP/COLOR_HAND")
    //@ui_desc LOCALIZE("LOD_MAN/EQUIP/COLOR_HAND")
    //@ui_group LOCALIZE("LOD_MAN/EQUIP/GROUP")
    //@default 0.0
    float4 g_colourHand;
#endif // MAN_LOD

#ifdef SKINNED


    //@default 1.0
    float4 g_channelColorR;


    //@default 1.0
    float4 g_channelColorG;



    //@default 1.0
    float4 g_channelColorB;


    //@default 0.0
    float4 g_specularColor;

#endif // SKINNED

#ifdef TRANSFORM_MAIN_UV
    TRANSFORM_BUNDLE(   sShiftX_,       "SHIFT_U",
                        sShiftY_,       "SHIFT_V",
                        sTransformX_,   "SCALE_U",
                        sTransformY_,   "SCALE_V",
                        sScale_,        "SCALE_UV",
                        sRotate_,       "ROTATE_UV",
                        LOCALIZE( "SHIFTY_SCALE_GROUP" ) );
#endif
}

static const int g_numBones = 150;

struct ObjectData
{

#ifdef SKINNED
    //@semantic WorldPalette 
    float4 world[g_numBones];
#else
    //@semantic World 
    float4x4 world : World;
#endif
#if defined(WAVE_ANIMATION)
    //@semantic WorldIT;
    float4x4 inverseWorld;
#endif

    //@semantic Ambient
    float4 ambientColour;
    //@semantic DirectionalLights
    DirectionalLight directionalLights[2];
    //@semantic PointLights
    PointLight pointLights[4];
    //@semantic SpotLights;
    SpotLight spotLights[2];
    //@sematic PlaneLights
    PlaneLight planeLight[4];
    //@semantic AmbientPointLights
    AmbientPointLight ambientPointLights[4];

    //@semantic DirectionalLights
    int nDirectionalLights;
    //@semantic PointLightsCount;
    int nPointLights;
    //@semantic SpotLightCount
    int nSpotLights;
    //@semantic PlaneLightCount
    int nPlaneLights;
    //semantic AmbientPointLightCount
    int nAmbientPointLights;

    //@semantic WetConstant
    //@default 0
    float wetAmmount;

    //@semantic ObjectID
    float g_objectID;
    //@semantic ZoneID
    uint zoneID;
}


#endif