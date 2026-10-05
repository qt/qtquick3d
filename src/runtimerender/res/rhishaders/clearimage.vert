// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;

out gl_PerVertex {
    vec4 gl_Position;
};

#if QSHADER_VIEW_COUNT >= 2
layout(location = 0) flat out uint v_viewIndex;
#endif

void main()
{
    vec3 p = attr_pos;
    gl_Position = vec4(p, 1.0);
#if QSHADER_VIEW_COUNT >= 2
    v_viewIndex = gl_ViewIndex;
#endif
}
