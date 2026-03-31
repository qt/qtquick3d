// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Augment shader for the AddDefine test.
// When AddDefine { name: "MY_DEFINE"; value: 1 } is present in the render pass
// commands, the compiler sees "#define MY_DEFINE 1" and the shader outputs green.
// When the define is absent the shader outputs red.
// This makes it possible to visually verify that AddDefine actually injects
// preprocessor definitions into the augment shader.

void MAIN_FRAGMENT_AUGMENT()
{
#ifdef MY_DEFINE
    fragOutput = vec4(0.0, 1.0, 0.0, 1.0); // green: MY_DEFINE was injected
#else
    fragOutput = vec4(1.0, 0.0, 0.0, 1.0); // red: MY_DEFINE was NOT injected
#endif
}
