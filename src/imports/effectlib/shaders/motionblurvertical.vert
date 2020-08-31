VARYING vec2 TexCoord0;
VARYING vec2 TexCoord1;
VARYING vec2 TexCoord2;

void setupVerticalGaussianBlur3Tap( float inDestHeight, float inBlurAmount, vec2 inTexCoord )
{
    float increment = inBlurAmount/inDestHeight;
    TexCoord0 = vec2(inTexCoord.x, inTexCoord.y + increment );
    TexCoord1 = vec2(inTexCoord.x, inTexCoord.y);
    TexCoord2 = vec2(inTexCoord.x, inTexCoord.y - increment);
}

void MAIN()
{
    setupVerticalGaussianBlur3Tap(INPUT_SIZE.y, 1.0, INPUT_UV);
}
