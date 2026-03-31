// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Augment shader for the blendEnabled test.
// Outputs white at 50% alpha so that blending with the blue clear colour produces
// a visible mix (neither fully white nor fully blue).

void MAIN_FRAGMENT_AUGMENT()
{
    fragOutput = vec4(1.0, 1.0, 1.0, 0.5);
}
