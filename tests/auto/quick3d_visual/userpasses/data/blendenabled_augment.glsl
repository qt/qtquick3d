// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Augment shader for the blendEnabled test.
// Sets the fragment output to white with 50% alpha so that when alpha blending
// is enabled with SrcAlpha mode, the result is a blend of white and the clear
// colour (blue), producing a light blue/cyan tint.
// Without blending, the output is pure white (or near-white).

void MAIN_FRAGMENT_AUGMENT()
{
    fragOutput = vec4(1.0, 1.0, 1.0, 0.5);
}
