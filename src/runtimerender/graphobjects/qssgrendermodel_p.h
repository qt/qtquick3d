/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSG_RENDER_MODEL_H
#define QSSG_RENDER_MODEL_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendertessmodevalues_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

struct QSSGRenderDefaultMaterial;
class QSSGBufferManager;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderModel : public QSSGRenderNode
{
    // Complete path to the file;
    //*not* relative to the presentation directory
    QVector<QSSGRenderGraphObject *> materials;
    QSSGRenderGeometry *geometry = nullptr;
    QSSGRenderMeshPath meshPath;
    float edgeTessellation = 1.0f;
    float innerTessellation = 1.0f;
    TessellationModeValues tessellationMode = TessellationModeValues::NoTessellation;
    bool wireframeMode = false;
    bool castsShadows = true;
    bool receivesShadows = true;

    QSSGRenderModel();

    QSSGBounds3 getModelBounds(const QSSGRef<QSSGBufferManager> &inManager) const;
};
QT_END_NAMESPACE

#endif
