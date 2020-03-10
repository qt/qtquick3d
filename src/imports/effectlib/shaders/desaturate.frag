vec4 desaturate(vec3 color, float strength)
{
    vec3 lum = vec3(0.299, 0.587, 0.114); // lum values based on: ITU-R BT.601
    vec3 gray = vec3(dot(lum, color));
    return vec4(mix(color, gray, strength), 1.0);
}

void frag()
{
    vec4 origColor = texture2D_0(TexCoord);
    gl_FragColor = desaturate(origColor.rgb, amount);
}
