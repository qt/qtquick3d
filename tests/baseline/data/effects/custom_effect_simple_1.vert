// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

VARYING vec2 center_vec;

void MAIN()
{
    center_vec = INPUT_UV - vec2(0.5, 0.5);
    center_vec.y *= INPUT_SIZE.y / INPUT_SIZE.x;
}
