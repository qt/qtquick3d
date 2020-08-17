VARYING vec2 texcoord;

void MAIN()
{
    BASE_COLOR = texture(tex1, texcoord);
    BASE_COLOR *= texture(tex2, texcoord);
    EMISSIVE_COLOR = vec3(0.8);
}
