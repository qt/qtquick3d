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

vec4 AdvancedPoissonTiltShiftBlur(sampler2D inSampler, float inAlphaFlag, float inBarHeight,
                                  float inFocusWidth, bool inVertical, bool inInvert)
{
    float mult0 = (1.0 - poisson0.z) * AdvancedGetTiltShiftMultiplier(TexCoord0, inBarHeight,
                                                                      inFocusWidth, inVertical,
                                                                      inInvert);
    float mult1 = (1.0 - poisson1.z) * AdvancedGetTiltShiftMultiplier(TexCoord1, inBarHeight,
                                                                      inFocusWidth, inVertical,
                                                                      inInvert);
    float mult2 = (1.0 - poisson2.z) * AdvancedGetTiltShiftMultiplier(TexCoord2, inBarHeight,
                                                                      inFocusWidth, inVertical,
                                                                      inInvert);
    float mult3 = (1.0 - poisson3.z) * AdvancedGetTiltShiftMultiplier(TexCoord3, inBarHeight,
                                                                      inFocusWidth, inVertical,
                                                                      inInvert);
    float mult4 = (1.0 - poisson4.z) * AdvancedGetTiltShiftMultiplier(TexCoord4, inBarHeight,
                                                                      inFocusWidth, inVertical,
                                                                      inInvert);

    float multTotal = mult0 + mult1 + mult2 + mult3 + mult4;
    float multMultiplier = multTotal > 0.0 ? 1.0 / multTotal : 0.0;

    vec4 outColor = GetTextureValuePreMult(inSampler, TexCoord0) * (mult0 * multMultiplier);
    outColor += GetTextureValuePreMult(inSampler, TexCoord1) * (mult1 * multMultiplier);
    outColor += GetTextureValuePreMult(inSampler, TexCoord2) * (mult2 * multMultiplier);
    outColor += GetTextureValuePreMult(inSampler, TexCoord3) * (mult3 * multMultiplier);
    outColor += GetTextureValuePreMult(inSampler, TexCoord4) * (mult4 * multMultiplier);
    return outColor;
}

void frag()
{
    float centerMultiplier = AdvancedGetTiltShiftMultiplier(TexCoord, focusPosition, focusWidth,
                                                             isVertical, isInverted);
    vec4 blurColor = AdvancedPoissonTiltShiftBlur(Texture0, Texture0Info.z, focusPosition,
                                                  focusWidth, isVertical, isInverted);
    gl_FragColor = mix(texture2D_sourceSampler(TexCoord), blurColor, centerMultiplier);
}
