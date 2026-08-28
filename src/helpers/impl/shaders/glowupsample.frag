// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "texturesample.glsllib"
#include "glowblur.glsllib"

void MAIN() {
    vec2 halfpixel = vec2(0.5 / INPUT_SIZE.x, 0.5 / INPUT_SIZE.y);
    vec4 color = qt_dualFilterUp8(INPUT, INPUT_UV, halfpixel, 2.0);

    if (glowAddSelfLevel)
        color += SAMPLE_LOD(glowDownBuffer, INPUT_UV, 0.0);

    FRAGCOLOR = color;
}
