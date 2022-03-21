/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
