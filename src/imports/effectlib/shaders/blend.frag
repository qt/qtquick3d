void frag()
{
    vec4 src = texture2D_0(TexCoord);
    vec4 dst = texture2D_sprite(TexCoord);

    colorOutput(src * (1.0 - dst.a) + dst);
}
