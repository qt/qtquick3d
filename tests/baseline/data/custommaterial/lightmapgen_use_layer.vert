VARYING vec2 uv1;

void MAIN()
{
    uv1 = UV1;

   // The source texture is coming from a Qt Quick item layer. For OpenGL we need to flip the V coordinate.
    if (FRAMEBUFFER_Y_UP == 1.0)
        uv1.y = 1.0 - uv1.y;

    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
