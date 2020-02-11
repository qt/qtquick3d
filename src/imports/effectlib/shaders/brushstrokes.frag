void frag (void)
{
    if (flagnoiseSample != 0) {
        mat2 rotMat = mat2(cosAlpha, sinAlpha, -sinAlpha, cosAlpha);

        vec2 uv0 = TexCoord / brushSize * 1000.0;
        uv0 = (uv0.yx * rotMat).yx;

        vec2 uv1 = texture2D_noiseSample(fract(uv0)).xy - 0.5;
        uv1*= vec2(1.0, 0.01);
        uv1*= rotMat;
        vec2 uv2 = TexCoord + uv1 * 0.1 * brushLength;

        vec2 halfPixelSize = 0.5 / Texture0Info.xy;
        colorOutput(texture2D_0(clamp(uv2, halfPixelSize, 1.0 - halfPixelSize)));
    } else {
        colorOutput(texture2D_0(TexCoord));
    }
}
