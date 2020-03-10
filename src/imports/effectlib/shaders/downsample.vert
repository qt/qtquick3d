#include "blur.glsllib"

void vert()
{
    SetupBoxBlurCoords(vec2(Texture0Info.xy));
}
