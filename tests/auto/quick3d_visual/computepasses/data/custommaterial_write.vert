// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Unshaded CustomMaterial fragment shaders get nothing auto-forwarded (see
// procedural_material.vert's comment); this vertex shader just needs to set
// POSITION since custommaterial_write.frag doesn't need UV0 (it addresses
// the marker image via gl_FragCoord instead).

void MAIN()
{
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
