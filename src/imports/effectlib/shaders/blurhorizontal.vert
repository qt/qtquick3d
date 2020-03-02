#include "blur.glsllib"

void vert()
{
    SetupHorizontalGaussianBlur(Texture0Info.x, amount, TexCoord);
}
