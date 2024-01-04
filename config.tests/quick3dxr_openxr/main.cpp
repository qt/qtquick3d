// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include <openxr/openxr.h>

int main(void)
{
    uint32_t extensionCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
    return 0;
}
