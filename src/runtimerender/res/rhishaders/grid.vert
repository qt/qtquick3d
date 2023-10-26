// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;

layout(location=0) out vec3 v_nearPoint;
layout(location=1) out vec3 v_farPoint;

layout(std140, binding=0) uniform buf {
    mat4 viewProj; // viewProj
    mat4 invViewProj; // invViewProj
    float near;
    float far;
    float scale;
    float yFactor;
    uint gridFlags;
} u_buf;

out gl_PerVertex { vec4 gl_Position; };

vec3 unprojectPoint(float x, float y, float z, mat4 invViewProj) {

    vec4 unprojectedPoint = invViewProj * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz/unprojectedPoint.w;
}

void main(void)
{
    vec3 p = attr_pos;

    v_nearPoint = unprojectPoint(p.x, u_buf.yFactor * p.y, 0.0, u_buf.invViewProj).xyz;
    v_farPoint = unprojectPoint(p.x, u_buf.yFactor * p.y, 1.0, u_buf.invViewProj).xyz;

    gl_Position = vec4(p, 1.0);
}
