/****************************************************************************
**
** Copyright (C) 2015 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSG_RENDER_LIGHTMAPS_H
#define QSSG_RENDER_LIGHTMAPS_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermaterialdirty_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderImage;
struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderLightmaps : public QSSGRenderGraphObject
{
    enum class Usage : quint8
    {
        Dynamic = 0,
        Baked,
        DynamicAndBaked,
    };

    QSSGRenderImage *m_lightmapIndirect = nullptr;
    QSSGRenderImage *m_lightmapRadiosity = nullptr;
    QSSGRenderImage *m_lightmapShadow = nullptr;

    QSSGRenderLightmaps();
};
QT_END_NAMESPACE

#endif
