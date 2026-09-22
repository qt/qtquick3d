// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// This header is already what the test is about: it is preamble code, and a
// preamble spanning several lines is what used to break the generated shader.

void MAIN_FRAGMENT_AUGMENT()
{
    // Several lines in the body as well, for the same reason.
    vec3 colour = vec3(0.0, 1.0, 0.0);
    fragOutput = vec4(colour, 1.0);
}
