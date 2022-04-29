/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSSG_RENDER_ITEM2D_H
#define QSSG_RENDER_ITEM2D_H

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
#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>

QT_BEGIN_NAMESPACE

class QSGNode;
class QSGRenderer;
struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderItem2D : public QSSGRenderNode
{
    Q_DISABLE_COPY(QSSGRenderItem2D)

    QMatrix4x4 MVP;
    float combinedOpacity = 1.0;
    float zOrder = 0;

    QSGRenderer *m_renderer = nullptr;
    QRhiRenderPassDescriptor *m_rp = nullptr;
    QSSGRenderContextInterface *m_rci = nullptr;
    bool m_contextWarningShown = false;

    QSSGRenderItem2D();
    ~QSSGRenderItem2D();
};
QT_END_NAMESPACE

#endif
