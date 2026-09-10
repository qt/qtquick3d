// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// The material's MAIN() has already sampled the unbound sampler2DArray and
// encoded the verdict into BASE_COLOR. All this has to do is put it on screen,
// unlit, so the grab is a flat colour.

void MAIN_FRAGMENT_AUGMENT()
{
    fragOutput = vec4(BASE_COLOR.rgb, 1.0);
}
