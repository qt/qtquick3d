/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
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


#ifndef QSSGRENDERINDEXTABLE_P_H
#define QSSGRENDERINDEXTABLE_P_H


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

QT_BEGIN_NAMESPACE

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderInstanceTableEntry {
    QVector4D row0;
    QVector4D row1;
    QVector4D row2;
    QVector4D color;
    QVector4D instanceData;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderInstanceTable : public QSSGRenderNode
{
    QSSGRenderInstanceTable() : QSSGRenderNode(QSSGRenderGraphObject::Type::ModelInstance) {}

    int count() const { return instanceCount; }
    qsizetype dataSize() const { return table.size(); }
    const void *constData() const { return table.constData(); }
    void setData(const QByteArray &data, int count, int stride) { table = data; instanceCount = count; instanceStride = stride; ++instanceSerial; }
    void setInstanceCountOverride(int count) { instanceCount = count; }
    int serial() const { return instanceSerial; }
    int stride() const { return instanceStride; }
    bool hasTransparency() { return transparency; }
    void setHasTransparency( bool t) { transparency = t; }
    void setDepthSorting(bool enable) { depthSorting = enable; }
    bool isDepthSortingEnabled() { return depthSorting; }

private:
    int instanceCount = 0;
    int instanceSerial = 0;
    uint instanceStride = 0;
    bool transparency = false;
    bool depthSorting = false;
    QByteArray table;
};

QT_END_NAMESPACE

#endif // QSSGRENDERINDEXTABLE_P_H
