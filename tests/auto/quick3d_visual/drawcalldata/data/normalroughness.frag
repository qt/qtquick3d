// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

void MAIN()
{
    // Reading the normal texture makes the layer run the normal pass for this
    // material.
    FRAGCOLOR = vec4(texture(NORMAL_ROUGHNESS_TEXTURE, vec2(0.5)).rgb * 0.5 + 0.5, 1.0);
}
