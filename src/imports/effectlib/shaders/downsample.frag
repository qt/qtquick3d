#include "blur.glsllib"

void frag()
{
    gl_FragColor = BoxDepthBlur(depthSampler, Texture0, Texture0Info.z, focusDistance,
                                focusRange, focusRange);
}
