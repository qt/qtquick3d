// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Displays a ShaderImage full-screen, ignoring the incoming scene colour.
// Used by the ComputePass test scenes to make a compute-written image
// visible on screen for pixel comparisons.

SHADER_IMAGE("rgba8", sourceImage, image2D, "readonly");

void MAIN()
{
    ivec2 size = imageSize(sourceImage);
    vec2 texcoord = TEXTURE_UV;
    if (FRAMEBUFFER_Y_UP < 0.0)
        texcoord.y = 1.0 - texcoord.y;
    ivec2 coord = clamp(ivec2(texcoord * vec2(size)), ivec2(0), size - ivec2(1));
    FRAGCOLOR = imageLoad(sourceImage, coord);
}
