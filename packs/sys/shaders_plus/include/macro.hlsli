#ifndef _MACROS_FXH_NEW_
#define _MACROS_FXH_NEW_

//---------------------------LOCALIZE-----------------------------------------------------
#define LOCALIZE(STR) "MODELEDITOR/PAGES/PAGE_MATERIALS/SHADERS/" STR

//---------------------------UV SHIFT / TRANSFORM ----------------------------------------

#define SHIFT_VARIABLES(shiftX, shiftXName, shiftY, shiftYName, Group)   \
    //@ui_name LOCALIZE(shiftXName)                                      \
    //@ui_desc "Shift U vector"                                          \
    //@ui_group Group                                                    \
    //@ui_min -2.0                                                        \
    //@ui_max 2.0                                                         \
    //@ui_digits 4                                                        \
    //@default 0.0                                                        \
    float shiftX;                                                        \
                                                                         \
    //@ui_name LOCALIZE(shiftYName)                                      \
    //@ui_desc "Shift V vector"                                          \
    //@ui_group Group                                                    \
    //@ui_min -2.0                                                        \
    //@ui_max 2.0                                                         \
    //@ui_digits 4                                                        \
    //@default 0.0                                                        \
    float shiftY;

#define TRANS_VARIABLES(transX, transXName, transY, transYName, Group)  \
    //@ui_name LOCALIZE(transXName)                                      \
    //@ui_desc "Transform U vector"                                      \
    //@ui_group Group                                                    \
    //@ui_min 0.0                                                         \
    //@ui_max 100.0                                                       \
    //@ui_digits 4                                                        \
    //@default 1.0                                                        \
    float transX;                                                        \
                                                                         \
    //@ui_name LOCALIZE(transYName)                                      \
    //@ui_desc "Transform V vector"                                      \
    //@ui_group Group                                                    \
    //@ui_min 0.0                                                         \
    //@ui_max 100.0                                                       \
    //@ui_digits 4                                                        \
    //@default 1.0                                                        \
    float transY;

#define SCALE_VARIABLES(scale, scaleName, Group)                         \
    //@ui_name LOCALIZE(scaleName)                                       \
    //@ui_desc "Scale UV"                                                \
    //@ui_group Group                                                    \
    //@ui_min 0.01                                                        \
    //@ui_max 10.0                                                        \
    //@ui_digits 4                                                        \
    //@default 1.0                                                        \
    float scale;

#define ROTATE_VARIABLES(rotateAngle, rotateAngleName, Group)           \
    //@ui_name LOCALIZE(rotateAngleName)                                 \
    //@ui_desc "Rotate around the origin"                                \
    //@ui_group Group                                                    \
    //@ui_min -360.0                                                      \
    //@ui_max 360.0                                                       \
    //@ui_digits 2                                                        \
    //@default 0.0                                                        \
    float rotateAngle;

//---------------------------BUNDLE--------------------------------------------------------

#define TRANSFORM_BUNDLE(shiftX, shiftXName, shiftY, shiftYName, transX, transXName, transY, transYName, scale, scaleName, rotateAngle, rotateAngleName, Group) \
    SHIFT_VARIABLES(shiftX, shiftXName, shiftY, shiftYName, Group) \
    TRANS_VARIABLES(transX, transXName, transY, transYName, Group) \
    SCALE_VARIABLES(scale, scaleName, Group) \
    ROTATE_VARIABLES(rotateAngle, rotateAngleName, Group)

//---------------------------ALPHA BLEND / TEST ------------------------------------------

#define ALPHABLEND_BUNDLE(inversAlphablend, inversAlphablendName, inversAlphablendGroup, \
                           blendFactor, blendFactorName, blendFactorGroup,                 \
                           blendPoint, blendPointName, blendPointGroup)                    \
    //@ui_name LOCALIZE(inversAlphablendName)                                           \
    //@ui_group LOCALIZE(inversAlphablendGroup)                                         \
    //@default false                                                                     \
    bool inversAlphablend;                                                               \
                                                                                         \
    //@ui_name LOCALIZE(blendFactorName)                                                \
    //@ui_group LOCALIZE(blendFactorGroup)                                              \
    //@ui_min 0.0                                                                        \
    //@ui_max 1.0                                                                        \
    //@ui_digits 2                                                                       \
    //@default 0.001                                                                     \
    float blendFactor;                                                                   \
                                                                                         \
    //@ui_name LOCALIZE(blendPointName)                                                 \
    //@ui_group LOCALIZE(blendPointGroup)                                               \
    //@ui_min -1.0                                                                       \
    //@ui_max 1.0                                                                        \
    //@ui_digits 2                                                                       \
    //@default 0.001                                                                     \
    float blendPoint;

#define ALPHATEST_BUNDLE(inversAlphatest, inversAlphatestName, alphatestFactor, alphatestFactorName) \
    //@ui_name inversAlphatestName                                                   \
    //@default false                                                                 \
    bool inversAlphatest;                                                            \
                                                                                     \
    //@ui_name alphatestFactorName                                                  \
    //@ui_desc "Alphatest factor"                                                  \
    //@ui_min 0.0                                                                    \
    //@ui_max 1.0                                                                    \
    //@ui_digits 4                                                                   \
    //@default 0.001                                                                 \
    float alphatestFactor;

#endif //_MACROS_FXH_NEW_