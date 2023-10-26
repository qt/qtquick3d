// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;

layout(std140, binding = 0) uniform buf {
    mat3 viewMatrix;
    mat4 inverseProjection;
    mat3 orientation;
    vec4 skyboxProperties;
} ubuf;

// skyboxProperties
// x: adjustY
// y: exposure
// z: blurAmount
// w: maxMipLevel

layout(location = 0) out vec3 eye_direction;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = vec4(attr_pos, 1.0);
    vec3 unprojected = (ubuf.inverseProjection * gl_Position).xyz;
    eye_direction = normalize(ubuf.viewMatrix * unprojected);
    eye_direction = normalize(ubuf.orientation * eye_direction);
    gl_Position.y *= ubuf.skyboxProperties.x;
}
