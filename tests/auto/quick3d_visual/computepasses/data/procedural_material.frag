// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Samples a ComputePass-generated ShaderImage directly as a CustomMaterial's
// surface colour, via imageLoad() rather than a regular sampler2D texture
// binding -- the compute-generated data never passes through a Texture
// object at all.
//
// UV0 isn't auto-forwarded to an Unshaded material's fragment shader (see
// procedural_material.vert), so it's received here via a matching VARYING
// instead of referenced directly.

VARYING vec2 vUV0;

SHADER_IMAGE("rgba8", proceduralImage, image2D, "readonly");

void MAIN()
{
    ivec2 size = imageSize(proceduralImage);
    ivec2 coord = clamp(ivec2(vUV0 * vec2(size)), ivec2(0), size - ivec2(1));
    FRAGCOLOR = imageLoad(proceduralImage, coord);
}
