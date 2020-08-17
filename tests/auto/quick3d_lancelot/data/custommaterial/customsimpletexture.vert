VARYING vec2 texcoord;

void MAIN()
{
    texcoord = UV0;
    VERTEX.x += sin(time * 4.0 + VERTEX.y) * amplitude;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
