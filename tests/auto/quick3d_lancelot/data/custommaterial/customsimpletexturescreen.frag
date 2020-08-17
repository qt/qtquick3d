VARYING vec2 texcoord;

void MAIN()
{
    BASE_COLOR = texture(tex1, texcoord);
    BASE_COLOR *= texture(SCREEN_TEXTURE, vec2(texcoord.x, 1.0 - texcoord.y)); // suitable for the Cube's tex coords
    EMISSIVE_COLOR = vec3(0.8);
}
