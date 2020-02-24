#include "blur.glsllib"

void frag()
{
    float centerMultiplier = GetDepthMultiplier(TexCoord, depthSampler, focusDistance,
                                                focusRange, focusRange);
    vec4 blurColor = PoissonDepthBlur(Texture0, Texture0Info.z, depthSampler,
                                      focusDistance, focusRange, focusRange);
    gl_FragColor = mix(texture2D_sourceSampler(TexCoord), blurColor, centerMultiplier);
}
