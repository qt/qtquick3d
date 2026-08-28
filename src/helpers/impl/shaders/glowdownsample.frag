// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "texturesample.glsllib"
#include "glowblur.glsllib"

float qt_glowFireflyWeight(vec4 c)
{
    return 1.0 / (1.0 + max(c.r, max(c.g, c.b)));
}

vec4 qt_glowFireflyAverage4(vec4 a, vec4 b, vec4 c, vec4 d)
{
    float wa = qt_glowFireflyWeight(a);
    float wb = qt_glowFireflyWeight(b);
    float wc = qt_glowFireflyWeight(c);
    float wd = qt_glowFireflyWeight(d);
    return (a * wa + b * wb + c * wc + d * wd) / (wa + wb + wc + wd);
}

void MAIN() {
    vec2 texel_size = vec2(1.0 / INPUT_SIZE.x, 1.0 / INPUT_SIZE.y);
    vec4 color;
    if (glowSuppressFireflies && isFirstPass && !glowQualityHigh) {
        vec4 c0 = SAMPLE_LOD(INPUT, INPUT_UV, 0.0);
        vec4 c1 = SAMPLE_LOD(INPUT, INPUT_UV + vec2(-texel_size.x, -texel_size.y), 0.0);
        vec4 c2 = SAMPLE_LOD(INPUT, INPUT_UV + vec2( texel_size.x, -texel_size.y), 0.0);
        vec4 c3 = SAMPLE_LOD(INPUT, INPUT_UV + vec2(-texel_size.x,  texel_size.y), 0.0);
        vec4 c4 = SAMPLE_LOD(INPUT, INPUT_UV + vec2( texel_size.x,  texel_size.y), 0.0);
        float w0 = 0.5   * qt_glowFireflyWeight(c0);
        float w1 = 0.125 * qt_glowFireflyWeight(c1);
        float w2 = 0.125 * qt_glowFireflyWeight(c2);
        float w3 = 0.125 * qt_glowFireflyWeight(c3);
        float w4 = 0.125 * qt_glowFireflyWeight(c4);
        color = (c0 * w0 + c1 * w1 + c2 * w2 + c3 * w3 + c4 * w4) / (w0 + w1 + w2 + w3 + w4);
    } else if (glowQualityHigh && isFirstPass) {
        vec4 a = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2(-2.0,  2.0), 0.0);
        vec4 b = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2( 0.0,  2.0), 0.0);
        vec4 c = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2( 2.0,  2.0), 0.0);
        vec4 d = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2(-2.0,  0.0), 0.0);
        vec4 e = SAMPLE_LOD(INPUT, INPUT_UV, 0.0);
        vec4 f = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2( 2.0,  0.0), 0.0);
        vec4 g = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2(-2.0, -2.0), 0.0);
        vec4 h = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2( 0.0, -2.0), 0.0);
        vec4 i = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2( 2.0, -2.0), 0.0);
        vec4 j = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2(-1.0,  1.0), 0.0);
        vec4 k = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2( 1.0,  1.0), 0.0);
        vec4 l = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2(-1.0, -1.0), 0.0);
        vec4 m = SAMPLE_LOD(INPUT, INPUT_UV + texel_size * vec2( 1.0, -1.0), 0.0);
        if (glowSuppressFireflies) {
            color = qt_glowFireflyAverage4(a, b, d, e) * 0.125;
            color += qt_glowFireflyAverage4(b, c, e, f) * 0.125;
            color += qt_glowFireflyAverage4(d, e, g, h) * 0.125;
            color += qt_glowFireflyAverage4(e, f, h, i) * 0.125;
            color += qt_glowFireflyAverage4(j, k, l, m) * 0.5;
        } else {
            color = e * 0.125;
            color += (a + c + g + i) * 0.03125;
            color += (b + d + f + h) * 0.0625;
            color += (j + k + l + m) * 0.125;
        }
    } else {
        color = qt_dualFilterDown5(INPUT, INPUT_UV, texel_size);
    }

    color *= glowStrength;

    if (isFirstPass)
        color = qt_glowThreshold(color, exposure, glowHDRMinimumValue, glowHDRScale, glowBloom, glowHDRMaximumValue);

    FRAGCOLOR = color;
}
