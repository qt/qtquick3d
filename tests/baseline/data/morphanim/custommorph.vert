// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

void MAIN()
{
    vec4 clipWRow = vec4(INSTANCE_MODELVIEWPROJECTION_MATRIX[0][3],
                         INSTANCE_MODELVIEWPROJECTION_MATRIX[1][3],
                         INSTANCE_MODELVIEWPROJECTION_MATRIX[2][3],
                         INSTANCE_MODELVIEWPROJECTION_MATRIX[3][3]);

    // Reduce angle before sin/cos: dot(clipWRow, offset) can be in the hundreds of
    // thousands, and GPU sin/cos argument reduction for large float32 values is
    // hardware-dependent. mod() is plain IEEE 754 arithmetic and gives consistent
    // results everywhere.
    float weight = mod(dot(clipWRow, offset), 2.0 * 3.14159265358979);
    vec3 pos = sin(weight) * MORPH_POSITION(0) + cos(weight) * MORPH_POSITION(1) +
                (1 - sin(weight) - cos(weight)) * VERTEX;

    POSITION = INSTANCE_MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
}
