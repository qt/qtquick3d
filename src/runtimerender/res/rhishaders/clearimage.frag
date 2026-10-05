// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

#if QSHADER_VIEW_COUNT >= 2
layout(location = 0) flat in uint v_viewIndex;
#endif

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    uvec4 clearColor;
    ivec2 viewSize;
    uint samples;
    uint padding;
} ubuf;

#if QSSG_CLEAR_BUFFER == 1
layout(std430, binding = 1) buffer restrict coherent writeonly qt_imgAux
{
    uint auxData[];
};
#else
layout(binding = 1, r32ui) uniform restrict coherent writeonly uimage2D qt_imgAux;
#endif

void main()
{
    fragOutput = vec4(0.0);
    const ivec2 pix = ivec2(gl_FragCoord.xy);
    uint v = 0;
#if QSHADER_VIEW_COUNT >= 2
    v = v_viewIndex;
#endif
    for (uint s = 0; s < ubuf.samples; s++) {
        const ivec2 coord = ivec2(pix.x + int(s) * ubuf.viewSize.x, pix.y + int(v) * ubuf.viewSize.y);
#if QSSG_CLEAR_BUFFER == 1
        auxData[coord.y * ubuf.viewSize.x * ubuf.samples + coord.x] = ubuf.clearColor.x;
#else
        imageStore(qt_imgAux, coord, ubuf.clearColor);
#endif
    }
}
