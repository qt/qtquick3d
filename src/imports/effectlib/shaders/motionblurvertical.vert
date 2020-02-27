#include "blur.glsllib"

void vert()
{
    SetupVerticalGaussianBlur3Tap(Texture0Info.y, 1.0, TexCoord);
}
