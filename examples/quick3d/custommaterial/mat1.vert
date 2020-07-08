void MAIN()
{
    vec3 pos = VERTEX;
    pos.x += sin(uTime * 4.0 + pos.y) * uAmplitude;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
}
