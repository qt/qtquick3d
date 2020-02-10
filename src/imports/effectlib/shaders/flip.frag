void frag()
{
    vec2 flippedCoord;
    flippedCoord.x = flipHorizontally ? 1.0 - TexCoord.x : TexCoord.x;
    flippedCoord.y = flipVertically ? 1.0 - TexCoord.y : TexCoord.y;

    gl_FragColor = texture2D_0(flippedCoord);
}
