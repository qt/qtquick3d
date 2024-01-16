// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

#include <openxr/openxr.h>

#if XR_CURRENT_API_VERSION < XR_MAKE_VERSION(1, 0, 29)
#error openxr.h is too old, needs >= 1.0.29
#endif

int main(void)
{
    uint32_t extensionCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
    return 0;
}
