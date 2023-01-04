void MAIN() {
    //glow uses larger sigma for a more rounded blur effect
    //vec2 pix_size = INPUT_SIZE;
    vec2 pix_size = vec2(1.0 / INPUT_SIZE.x, 1.0 / INPUT_SIZE.y);
    //pix_size *= 0.5; //reading from larger buffer, so use more samples
    vec4 color;
    if (glowQualityHigh) {
        color = textureLod(INPUT, INPUT_UV + vec2(0.0, 0.0) * pix_size, 0.0) * 0.152781;
        color += textureLod(INPUT, INPUT_UV + vec2(1.0, 0.0) * pix_size, 0.0) * 0.144599;
        color += textureLod(INPUT, INPUT_UV + vec2(2.0, 0.0) * pix_size, 0.0) * 0.122589;
        color += textureLod(INPUT, INPUT_UV + vec2(3.0, 0.0) * pix_size, 0.0) * 0.093095;
        color += textureLod(INPUT, INPUT_UV + vec2(4.0, 0.0) * pix_size, 0.0) * 0.063327;
        color += textureLod(INPUT, INPUT_UV + vec2(-1.0, 0.0) * pix_size, 0.0) * 0.144599;
        color += textureLod(INPUT, INPUT_UV + vec2(-2.0, 0.0) * pix_size, 0.0) * 0.122589;
        color += textureLod(INPUT, INPUT_UV + vec2(-3.0, 0.0) * pix_size, 0.0) * 0.093095;
        color += textureLod(INPUT, INPUT_UV + vec2(-4.0, 0.0) * pix_size, 0.0) * 0.063327;

        color += textureLod(INPUT, INPUT_UV + vec2(0.0, 1.0) * pix_size, 0.0) * 0.152781;
        color += textureLod(INPUT, INPUT_UV + vec2(1.0, 1.0) * pix_size, 0.0) * 0.144599;
        color += textureLod(INPUT, INPUT_UV + vec2(2.0, 1.0) * pix_size, 0.0) * 0.122589;
        color += textureLod(INPUT, INPUT_UV + vec2(3.0, 1.0) * pix_size, 0.0) * 0.093095;
        color += textureLod(INPUT, INPUT_UV + vec2(4.0, 1.0) * pix_size, 0.0) * 0.063327;
        color += textureLod(INPUT, INPUT_UV + vec2(-1.0, 1.0) * pix_size, 0.0) * 0.144599;
        color += textureLod(INPUT, INPUT_UV + vec2(-2.0, 1.0) * pix_size, 0.0) * 0.122589;
        color += textureLod(INPUT, INPUT_UV + vec2(-3.0, 1.0) * pix_size, 0.0) * 0.093095;
        color += textureLod(INPUT, INPUT_UV + vec2(-4.0, 1.0) * pix_size, 0.0) * 0.063327;
        color *= 0.5;
    } else {
        color = textureLod(INPUT, INPUT_UV + vec2(0.0, 0.0) * pix_size, 0.0) * 0.174938;
        color += textureLod(INPUT, INPUT_UV + vec2(1.0, 0.0) * pix_size, 0.0) * 0.165569;
        color += textureLod(INPUT, INPUT_UV + vec2(2.0, 0.0) * pix_size, 0.0) * 0.140367;
        color += textureLod(INPUT, INPUT_UV + vec2(3.0, 0.0) * pix_size, 0.0) * 0.106595;
        color += textureLod(INPUT, INPUT_UV + vec2(-1.0, 0.0) * pix_size, 0.0) * 0.165569;
        color += textureLod(INPUT, INPUT_UV + vec2(-2.0, 0.0) * pix_size, 0.0) * 0.140367;
        color += textureLod(INPUT, INPUT_UV + vec2(-3.0, 0.0) * pix_size, 0.0) * 0.106595;
    }

    color *= glowStrength;

    if (isFirstPass) {
        color *= exposure;

        float luminance = max(color.r, max(color.g, color.b));
        float feedback = max(smoothstep(glowHDRMinimumValue, glowHDRMinimumValue + glowHDRScale, luminance), glowBloom);

        color = min(color * feedback, vec4(glowHDRMaximumValue));
    }
    FRAGCOLOR = color;
}
