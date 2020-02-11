#include "blur.glsllib"

void frag()
{
    gl_FragColor = GaussianBlur(Texture0, Texture0Info.z);
}
