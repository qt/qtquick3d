VARYING vec2 coord;

void MAIN()
{
    coord = UV0;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
