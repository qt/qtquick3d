#include "blur.glsllib"

void vert()
{
    SetupHorizontalGaussianBlur3Tap(Texture0Info.x, 1.0, TexCoord);
}
