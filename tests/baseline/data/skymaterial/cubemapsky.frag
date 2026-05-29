// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

void MAIN()
{
    vec3 direction = normalize(qt_eyeDir);
    vec4 col0 = texture(cubeMapTexture0, direction);
    vec4 col1 = texture(cubeMapTexture1, direction);
    FRAGCOLOR = mix(col0, col1, mixT);
}
