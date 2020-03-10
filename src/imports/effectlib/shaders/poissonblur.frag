#include "blur.glsllib"
#include "luminance.glsllib"

vec4 PoissonBlur(sampler2D inSampler)
{
    float mult0 = (1.0 - poisson0.z);
    float mult1 = (1.0 - poisson1.z);
    float mult2 = (1.0 - poisson2.z);
    float mult3 = (1.0 - poisson3.z);
    float mult4 = (1.0 - poisson4.z);
    float mult5 = (1.0 - poisson5.z);
    float mult6 = (1.0 - poisson6.z);
    float mult7 = (1.0 - poisson7.z);

    float multTotal = mult0 + mult1 + mult2 + mult3 + mult4 + mult5 + mult6 + mult7;
    float multMultiplier = (multTotal > 0.0 ? 1.0 / multTotal : 0.0) * negativeBlurFalloffExp2;

    vec4 outColor = GetTextureValue(inSampler, TexCoord0, 1.0) * (mult0 * multMultiplier);
    outColor += GetTextureValue(inSampler, TexCoord1, 1.0) * (mult1 * multMultiplier);
    outColor += GetTextureValue(inSampler, TexCoord2, 1.0) * (mult2 * multMultiplier);
    outColor += GetTextureValue(inSampler, TexCoord3, 1.0) * (mult3 * multMultiplier);
    outColor += GetTextureValue(inSampler, TexCoord4, 1.0) * (mult4 * multMultiplier);
    outColor += GetTextureValue(inSampler, TexCoord5, 1.0) * (mult5 * multMultiplier);
    outColor += GetTextureValue(inSampler, TexCoord6, 1.0) * (mult6 * multMultiplier);
    outColor += GetTextureValue(inSampler, TexCoord7, 1.0) * (mult7 * multMultiplier);
    return outColor;
}

void frag()
{
    fragOutput = PoissonBlur(Texture0);
}
