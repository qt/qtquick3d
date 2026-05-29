// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440
layout (location = 0) in vec3 aPos;

layout(std140, binding = 0) uniform buf {
    mat4 projection;
    mat4 view;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

// The normalize fragment shader works in gl_FragCoord space (texelFetch by layer),
// so we don't need to forward localPos. Keeping the cube geometry + view here is
// what produces the correct gl_FragCoord positions for each destination face.

void main()
{
    mat4 rotView = mat4(mat3(ubuf.view));
    vec4 clipPos = ubuf.projection * rotView * vec4(aPos, 1.0);

    gl_Position = clipPos.xyww;
}
