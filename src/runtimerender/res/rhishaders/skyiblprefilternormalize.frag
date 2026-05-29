// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440
layout(location = 0) out vec4 FragColor;

layout(binding = 1) uniform sampler2DArray accumulator;
layout(std140, binding = 2) uniform buf {
    int faceIndex;
    int _pad0;
    int _pad1;
    int _pad2;
} ubuf2;

// Reads the unnormalized partial sums for the currently bound
// accumulation mip level and emits the normalized prefiltered
// radiance for the destination mip level.
//
// RGB stores sum(color · NdotL) / N, while alpha stores
// sum(NdotL) / N, where N is the total sample count across all
// accumulation slices. Dividing RGB by alpha reconstructs the
// correctly weighted average radiance.

void main()
{
    ivec3 coord = ivec3(int(gl_FragCoord.x), int(gl_FragCoord.y), ubuf2.faceIndex);
    vec4 s = texelFetch(accumulator, coord, 0);
    vec3 rgb = s.a > 0.0 ? s.rgb / s.a : vec3(0.0);
    FragColor = vec4(rgb, 1.0);
}
