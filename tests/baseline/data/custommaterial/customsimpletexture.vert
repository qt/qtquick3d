// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

VARYING vec2 texcoord;

void MAIN()
{
    texcoord = UV0;
    VERTEX.x += sin(time * 4.0 + VERTEX.y) * amplitude;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
}
