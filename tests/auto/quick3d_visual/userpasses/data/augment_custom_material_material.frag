// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Nothing interesting: the material only has to be a custom material, so that
// the pass takes the custom material shader path. Red is what shows up if the
// augment shader never gets to overwrite it.

void MAIN()
{
    BASE_COLOR = vec4(1.0, 0.0, 0.0, 1.0);
}
