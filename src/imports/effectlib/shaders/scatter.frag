uniform float AppFrame;  // frame number since app starts

void frag (void)
{
    float size = 15.0;
    float strength = amount / 127.0 * 0.4;

    vec2 uv = TexCoord * size;
    if (randomize)
        uv = fract(uv + 0.031 * AppFrame);

    uv = texture2D_noiseSample(fract(uv)).xy - 0.5;

    if (direction == 0)
        uv *= (vec2(1.5, 0.15) * strength);
    else if (direction == 1)
        uv *= (vec2(1.3, 0.0) * strength);
    else
        uv *= (vec2(0.0, 0.29) * strength);

    uv += TexCoord;

    vec2 halfPixelSize = 0.5 / Texture0Info.xy;
    colorOutput(texture2D_0(clamp(uv, halfPixelSize, 1.0 - halfPixelSize)));
}
