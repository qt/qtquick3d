// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRVISIONOSMANAGER_P_H
#define QQUICK3DXRVISIONOSMANAGER_P_H

#import <Foundation/Foundation.h>
#import <CompositorServices/CompositorServices.h>

#include <QObject>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


@interface QQuick3DXRVisionOSManager : NSObject

@property (nonatomic, assign) cp_layer_renderer_t layerRenderer;

+ (instancetype)sharedManager;

@end

#endif // QQUICK3DXRVISIONOSMANAGER_P_H
