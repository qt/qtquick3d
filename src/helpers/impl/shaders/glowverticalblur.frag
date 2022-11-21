
void MAIN() {
    //glow uses larger sigma for a more rounded blur effect
    vec2 pixelSize = vec2(1.0 / INPUT_SIZE.x, 1.0 / INPUT_SIZE.y);
    vec4 color = textureLod(INPUT, INPUT_UV + vec2(0.0, 0.0) * pixelSize, 0.0) * 0.288713;
    color += textureLod(INPUT, INPUT_UV + vec2(0.0, 1.0) * pixelSize, 0.0) * 0.233062;
    color += textureLod(INPUT, INPUT_UV + vec2(0.0, 2.0) * pixelSize, 0.0) * 0.122581;
    color += textureLod(INPUT, INPUT_UV + vec2(0.0, -1.0) * pixelSize, 0.0) * 0.233062;
    color += textureLod(INPUT, INPUT_UV + vec2(0.0, -2.0) * pixelSize, 0.0) * 0.122581;
    color *= glowStrength;

    FRAGCOLOR = color;
}
