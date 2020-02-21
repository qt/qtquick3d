#include "blur.glsllib"

vec2 ToRotatedPoissonTexCoord(vec3 poisson, vec2 inputTex, vec2 inc, mat2 rotation)
{
    vec2 rotatedPoisson = rotation * vec2(poisson.xy);
    return vec2(inputTex.x + rotatedPoisson.x * inc.x, inputTex.y + rotatedPoisson.y * inc.y);
}

void SetupPoissonBlurCoordsRotation(float inBlurAmount, vec2 inTexInfo, float inRotationRadians)
{
    float rotCos = cos(inRotationRadians);
    float rotSin = sin(inRotationRadians);
    mat2 rotMatrix = mat2(rotCos, rotSin, -rotSin, rotCos);
    vec2 incVec = vec2(inBlurAmount / inTexInfo.x, inBlurAmount / inTexInfo.y);

    TexCoord0 = ToRotatedPoissonTexCoord(poisson0, TexCoord, incVec, rotMatrix);
    TexCoord1 = ToRotatedPoissonTexCoord(poisson1, TexCoord, incVec, rotMatrix);
    TexCoord2 = ToRotatedPoissonTexCoord(poisson2, TexCoord, incVec, rotMatrix);
    TexCoord3 = ToRotatedPoissonTexCoord(poisson3, TexCoord, incVec, rotMatrix);
    TexCoord4 = ToRotatedPoissonTexCoord(poisson4, TexCoord, incVec, rotMatrix);
    TexCoord5 = ToRotatedPoissonTexCoord(poisson5, TexCoord, incVec, rotMatrix);
    TexCoord6 = ToRotatedPoissonTexCoord(poisson6, TexCoord, incVec, rotMatrix);
    TexCoord7 = ToRotatedPoissonTexCoord(poisson7, TexCoord, incVec, rotMatrix);
}

void vert ()
{
    SetupPoissonBlurCoordsRotation(5.0 * negativeBlurFalloffExp2, Texture0Info.xy, 0.0);
}
