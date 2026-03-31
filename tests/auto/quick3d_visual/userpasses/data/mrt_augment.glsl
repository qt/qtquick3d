// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Augment shader that writes specific colors to multiple render targets
// This validates that MRT (Multiple Render Targets) functionality works

void MAIN_FRAGMENT_AUGMENT()
{
    // Write different colors to each attachment to prove MRT works

    // Attachment 0 (GBUFFER0) - Red
    GBUFFER0 = vec4(1.0, 0.0, 0.0, 1.0);

    // Attachment 1 (GBUFFER1) - Yellow (test displays this)
    GBUFFER1 = vec4(1.0, 1.0, 0.0, 1.0);

    // Attachment 2 (GBUFFER2) - Blue
    GBUFFER2 = vec4(0.0, 0.0, 1.0, 1.0);
}
