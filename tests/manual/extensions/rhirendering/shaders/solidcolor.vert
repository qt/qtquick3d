// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#version 440

layout(location = 0) in vec4 position;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec4 color;
};

void main()
{
    gl_Position = mvp * position;
}
