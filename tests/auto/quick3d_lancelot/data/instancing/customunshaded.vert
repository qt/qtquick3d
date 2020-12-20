VARYING vec3 pos;
VARYING vec2 coord;

void MAIN()
{
    pos = VERTEX;
    pos.x += sin(time * 4.0 + pos.y) * amplitude;
    coord = UV0;
    POSITION = INSTANCE_MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
}
