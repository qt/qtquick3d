// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Augment shader consuming a ShaderImage and a ShaderStorageBuffer exposed as
// properties of the RenderPass. Both are declared here, outside
// MAIN_FRAGMENT_AUGMENT(): the declarations are rewritten in place, so they
// have to sit at file scope like any other GLSL declaration.
//
// Outputs green only when both resources carry the values that
// augment_resources_fill.comp wrote, red otherwise. That distinguishes an
// unbound resource (reads as zero) from a correctly bound one.

STORAGE_BUFFER(checkBuffer, "readonly") {
    vec4 values[];
}

SHADER_IMAGE("rgba8", checkImage, image2D, "readonly");

void MAIN_FRAGMENT_AUGMENT()
{
    vec4 fromImage = imageLoad(checkImage, ivec2(0, 0));
    vec4 fromBuffer = values[0];

    bool imageOk = fromImage.r < 0.25 && fromImage.g > 0.75 && fromImage.b < 0.25;
    bool bufferOk = abs(fromBuffer.x - 0.25) < 0.02
                 && abs(fromBuffer.y - 0.5) < 0.02
                 && abs(fromBuffer.z - 0.75) < 0.02;

    fragOutput = (imageOk && bufferOk) ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
