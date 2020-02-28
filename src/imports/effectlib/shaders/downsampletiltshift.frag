#include "blur.glsllib"

float AdvancedGetTiltShiftMultiplier(vec2 inTexCoord, float inFocusBarHeight, float inFocusWidth,
                                     bool inVertical, bool inInvert)
{
    float texPos = inVertical ? inTexCoord.x : inTexCoord.y;
    float focusDiff = max(0.0, abs(texPos - inFocusBarHeight) - (inFocusWidth / 2.0))
            / inFocusWidth;
    float retval = clamp(focusDiff, 0.0, 1.0);
    return inInvert ? 1.0 - retval : retval;
}

vec4 AdvancedBoxTiltShiftBlur(sampler2D inBlurSampler, float inBlurSamplerAlphaFlag,
                              float inFocusBarHeight, float inFocusWidth,
                              bool inVertical, bool inInvert)
{
    float mult0 = .25 * AdvancedGetTiltShiftMultiplier(TexCoord0, inFocusBarHeight, inFocusWidth,
                                                       inVertical, inInvert);
    float mult1 = .25 * AdvancedGetTiltShiftMultiplier(TexCoord1, inFocusBarHeight, inFocusWidth,
                                                       inVertical, inInvert);
    float mult2 = .25 * AdvancedGetTiltShiftMultiplier(TexCoord2, inFocusBarHeight, inFocusWidth,
                                                       inVertical, inInvert);
    float mult3 = .25 * AdvancedGetTiltShiftMultiplier(TexCoord3, inFocusBarHeight, inFocusWidth,
                                                       inVertical, inInvert);
    float multTotal = mult0 + mult1 + mult2 + mult3;
    float totalDivisor = multTotal != 0.0 ? 1.0 / multTotal : 0.0;
    vec4 outCol = GetTextureValuePreMult(inBlurSampler, TexCoord0) * mult0;
    outCol += GetTextureValuePreMult(inBlurSampler, TexCoord1) * mult1;
    outCol += GetTextureValuePreMult(inBlurSampler, TexCoord2) * mult2;
    outCol += GetTextureValuePreMult(inBlurSampler, TexCoord3) * mult3;
    return outCol * totalDivisor;
}

void frag()
{
    gl_FragColor = AdvancedBoxTiltShiftBlur(Texture0, Texture0Info.z, focusPosition, focusWidth,
                                            isVertical, isInverted);
}
