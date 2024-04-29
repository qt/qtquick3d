// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#import "qquick3dxrvisionosmanager_p.h"

@implementation QQuick3DXRVisionOSManager

+ (instancetype)sharedManager {
    static QQuick3DXRVisionOSManager *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
        // Default initialization if needed
        sharedInstance.layerRenderer = NULL;  // Initialize with NULL or a suitable default
    });
    return sharedInstance;
}

@end
