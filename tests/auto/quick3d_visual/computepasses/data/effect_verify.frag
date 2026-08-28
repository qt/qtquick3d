// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Second stage of the same Effects chain as effect_write.frag. Effects
// chain as separate, sequential render passes -- by the time this pass's
// first invocation runs, effect_write.frag's pass has completed for the
// entire framebuffer, so reading "volatile readonly" here is guaranteed to
// see the final post-write state, not a partial one.
//
// Paints the whole screen green only if the buffer counter reached exactly
// one atomicAdd per output pixel and the marker image was correctly written
// at every sampled position; red otherwise. The pixel count comes from
// OUTPUT_SIZE, as the framebuffer is larger than the 400x400 item with a
// device pixel ratio above 1.

STORAGE_BUFFER(counterBuffer, "volatile readonly") {
    uint counter[];
}

SHADER_IMAGE("rgba8", markerImage, image2D, "volatile readonly");

void MAIN()
{
    ivec2 size = imageSize(markerImage);
    vec2 texcoord = TEXTURE_UV;
    if (FRAMEBUFFER_Y_UP < 0.0)
        texcoord.y = 1.0 - texcoord.y;
    ivec2 coord = clamp(ivec2(texcoord * vec2(size)), ivec2(0), size - ivec2(1));

    vec4 marker = imageLoad(markerImage, coord);
    bool markerOk = (marker.g > 0.5 && marker.r < 0.5);
    bool countOk = (counter[0] == uint(OUTPUT_SIZE.x) * uint(OUTPUT_SIZE.y));

    FRAGCOLOR = (markerOk && countOk) ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
