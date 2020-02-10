void frag()
{
    vec4 origColor = texture2D_0(TexCoord);

    float step_w = amount;
    float step_h = amount;

    vec4 t1 = texture2D_0(vec2(TexCoord.x - step_w, TexCoord.y - step_h));
    vec4 t2 = texture2D_0(vec2(TexCoord.x, TexCoord.y - step_h));
    vec4 t3 = texture2D_0(vec2(TexCoord.x - step_w, TexCoord.y));
    vec4 t4 = texture2D_0(TexCoord);

    vec3 rr = -4.0 * t1.rgb - 4.0 * t2.rgb - 4.0 * t3.rgb + 12.0 * t4.rgb;
    float y = (rr.r + rr.g + rr.b) / 3.0;

    vec4 result = vec4(vec3(y, y, y) + 0.3, origColor.a);

    gl_FragColor = result;
}
