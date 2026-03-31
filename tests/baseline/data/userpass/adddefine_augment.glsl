// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Augment shader for the AddDefine baseline test.
// MY_DEFINE is injected as #define MY_DEFINE 1 when AddDefine is active.

void MAIN_FRAGMENT_AUGMENT()
{
#ifdef MY_DEFINE
    fragOutput = vec4(0.0, 1.0, 0.0, 1.0); // green: MY_DEFINE was injected
#else
    fragOutput = vec4(1.0, 0.0, 0.0, 1.0); // red: MY_DEFINE was NOT injected
#endif
}
