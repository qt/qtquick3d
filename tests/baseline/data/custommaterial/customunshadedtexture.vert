VARYING vec2 texcoord;
VARYING vec3 normal;

void MAIN()
{
    texcoord = UV0;
    texcoord.x *= coordXFactor;
    texcoord.y *= coordYFactor;
    normal = NORMAL;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
