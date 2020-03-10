#include "blur.glsllib"

void vert()
{
    SetupPoissonBlurCoords(blurAmount, DestSize.xy);
}
