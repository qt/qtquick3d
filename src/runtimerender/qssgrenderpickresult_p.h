// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERPICKRESULT_H
#define QSSGRENDERPICKRESULT_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>
#include <limits>

QT_BEGIN_NAMESPACE

struct QSSGRenderPickResult
{
    const QSSGRenderGraphObject *m_hitObject = nullptr;
    float m_distanceSq = std::numeric_limits<float>::max();
    // The local coordinates in X,Y UV space where the hit occurred
    QVector2D m_localUVCoords;
    // The position in world coordinates
    QVector3D m_scenePosition;
    // The position in local coordinates
    QVector3D m_localPosition;
    // The normal of the hit face
    QVector3D m_faceNormal;
    // The subset index
    int m_subset = 0;
    int m_instanceIndex = -1;
};

Q_STATIC_ASSERT(std::is_trivially_destructible<QSSGRenderPickResult>::value);

struct QSSGPickResultProcessResult : public QSSGRenderPickResult
{
    QSSGPickResultProcessResult(const QSSGRenderPickResult &inSrc) : QSSGRenderPickResult(inSrc) {}
    QSSGPickResultProcessResult(const QSSGRenderPickResult &inSrc, bool consumed) : QSSGRenderPickResult(inSrc), m_wasPickConsumed(consumed) {}
    QSSGPickResultProcessResult() = default;
    bool m_wasPickConsumed = false;
};

QT_END_NAMESPACE

#endif // QSSGRENDERPICKRESULT_H
