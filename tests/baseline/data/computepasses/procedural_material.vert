// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// An Unshaded CustomMaterial's fragment shader has none of the usual
// conveniences (UV0, NORMAL, ...) auto-forwarded to it -- those are only
// injected for Shaded materials. This vertex shader manually forwards UV0
// via a VARYING, matching the pattern in
// examples/quick3d/clusteredlighting/custom_lighting.vert.

VARYING vec2 vUV0;

void MAIN()
{
    vUV0 = UV0;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
