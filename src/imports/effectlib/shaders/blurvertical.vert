#include "blur.glsllib"

void vert()
{
    SetupVerticalGaussianBlur(Texture0Info.y, amount, TexCoord);
}
