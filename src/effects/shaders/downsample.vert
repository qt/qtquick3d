VARYING vec2 TexCoord0;
VARYING vec2 TexCoord1;
VARYING vec2 TexCoord2;
VARYING vec2 TexCoord3;

void MAIN()
{
    float xIncrement = .5 / INPUT_SIZE.x;
    float yIncrement = .5 / INPUT_SIZE.y;
    TexCoord0 = vec2( INPUT_UV.x + xIncrement, INPUT_UV.y + yIncrement );
    TexCoord1 = vec2( INPUT_UV.x - xIncrement, INPUT_UV.y - yIncrement );
    TexCoord2 = vec2( INPUT_UV.x - xIncrement, INPUT_UV.y + yIncrement );
    TexCoord3 = vec2( INPUT_UV.x + xIncrement, INPUT_UV.y - yIncrement );
}
