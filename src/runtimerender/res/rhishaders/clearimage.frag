// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    uvec4 clearColor;
} ubuf;

layout(binding = 1, rgba32ui) uniform restrict writeonly uimage2D qt_imgAux;
layout(binding = 2, r32ui) uniform restrict writeonly uimage2D qt_imgCounter;

void main()
{
    fragOutput = vec4(0.0);
    imageStore(qt_imgAux, ivec2(gl_FragCoord.xy), ubuf.clearColor);
    imageStore(qt_imgCounter, ivec2(0), uvec4(0));
}
