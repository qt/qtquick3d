// Copyright (C) 2024 The Qt Company Ltd.*
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

VARYING vec2 texcoord;

// Parameters as smoothstep

float interpolate(float lower, float upper, float x)
{
#if 1
//smoothstep
    return smoothstep(lower, upper, x);
#else
//linear
    float n =  (min(upper, max(lower, x)) - lower) / (upper - lower);
    return n;
#endif
}

void MAIN()
{
    float dist = length(texcoord - vec2(0.5, 0.5));

    float dx = dFdx(dist);
    float dy = dFdy(dist);
    float df = length(vec2(dx, dy));

    float alphaval =  1.0 - interpolate(radius - df, radius + df, dist);

    const float contrast = 0.6;
    float light = 1.0 - contrast * interpolate(radius - 2*df, radius - df, dist);

    FRAGCOLOR = vec4(light, light, light, 1) * alphaval * opacity;
}
