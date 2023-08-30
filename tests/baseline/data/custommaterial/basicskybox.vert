VARYING vec3 coord;

void MAIN()
{
    coord = VERTEX;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
