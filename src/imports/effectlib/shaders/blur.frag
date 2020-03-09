void frag()
{
    vec4 c1 = texture2D_0(vec2(TexCoord.x - amount, TexCoord.y - amount));
    vec4 c2 = texture2D_0(vec2(TexCoord.x + amount, TexCoord.y - amount));
    vec4 c3 = texture2D_0(vec2(TexCoord.x - amount, TexCoord.y + amount));
    vec4 c4 = texture2D_0(vec2(TexCoord.x + amount, TexCoord.y + amount));
    vec4 c5 = texture2D_0(vec2(TexCoord.x - (amount * 2.0), TexCoord.y));
    vec4 c6 = texture2D_0(vec2(TexCoord.x, TexCoord.y - (amount * 2.0)));
    vec4 c7 = texture2D_0(vec2(TexCoord.x, TexCoord.y + (amount * 2.0)));
    vec4 c8 = texture2D_0(vec2(TexCoord.x + (amount * 2.0), TexCoord.y));

    gl_FragColor = (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8) / 8.0;
}
