void frag()
{
    vec4 origColor = texture2D_0(TexCoord);
    vec2 uv = TexCoord.xy;

    vec4 bg = origColor;

    uv *= 1.0 - uv.yx;
    float vig = uv.x * uv.y * vignetteStrength;
    vig = pow(vig, vignetteRadius);

    vec4 vigColor = vec4(vignetteColor.rgb, vig) * vig;
    gl_FragColor = mix(vigColor, origColor, vig);
}
