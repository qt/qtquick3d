VARYING vec2 TexCoord0;
VARYING vec2 TexCoord1;
VARYING vec2 TexCoord2;

void setupHorizontalGaussianBlur3Tap( float inDestWidth, float inBlurAmount, vec2 inTexCoord )
{
    float increment = inBlurAmount/inDestWidth;
    TexCoord0 = vec2(inTexCoord.x + increment            , inTexCoord.y );
    TexCoord1 = vec2(inTexCoord.x                        , inTexCoord.y);
    TexCoord2 = vec2(inTexCoord.x - increment            , inTexCoord.y);
}

void MAIN()
{
    setupHorizontalGaussianBlur3Tap(INPUT_SIZE.x, 1.0, INPUT_UV);
}
