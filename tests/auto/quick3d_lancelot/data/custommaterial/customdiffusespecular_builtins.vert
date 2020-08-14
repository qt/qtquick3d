void MAIN()
{
    VERTEX.x += sin(uTime * 4.0 + VERTEX.y) * uAmplitude;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
