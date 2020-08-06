VARYING vec3 vNormal;

void MAIN()
{
    vNormal = normalize(NORMAL_MATRIX * NORMAL);
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
