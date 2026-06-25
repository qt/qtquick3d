// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

VARYING vec2 center_vec;

void MAIN()
{
    vec2 center_adj = vec2(center.x, 0.5 + (center.y - 0.5) * (-FRAMEBUFFER_Y_UP));
    center_vec = INPUT_UV - center_adj;
    // Multiply by x/y ratio to make the distortion round instead of an ellipse
    center_vec.y *= INPUT_SIZE.y / INPUT_SIZE.x;
}
