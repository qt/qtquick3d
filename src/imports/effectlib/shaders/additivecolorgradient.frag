void frag()
{
    vec4 origColor = texture2D_0(TexCoord);
    vec2 uv = TexCoord.xy;

    vec4 gradient = vec4(mix(vec4(bottomColor.rgb, 1.0), vec4(topColor.rgb, 1.0), uv.y));

    gl_FragColor = origColor + gradient;
}
