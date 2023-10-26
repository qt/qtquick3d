// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;

layout(std140, binding = 0) uniform buf {
    vec4 aoProperties;
    vec4 aoProperties2;
    vec4 aoScreenConst;
    vec4 uvToEyeConst;
    vec2 cameraProperties;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = vec4(attr_pos.xy, 0.5, 1.0 );
}
