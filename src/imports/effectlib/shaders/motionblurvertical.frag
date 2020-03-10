#include "blur.glsllib"

void frag()
{
    gl_FragColor = GaussianBlur3TapPremultiplied(Texture0);
}
