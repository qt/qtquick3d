// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>

#include <QtGui/qvectornd.h>
#include <QtGui/qmatrix4x4.h>

QT_BEGIN_NAMESPACE

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderInstanceTableEntry {
    QVector4D row0;
    QVector4D row1;
    QVector4D row2;
    QVector4D color;
    QVector4D instanceData;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderInstanceTable : public QSSGRenderGraphObject
{
    QSSGRenderInstanceTable() : QSSGRenderGraphObject(QSSGRenderGraphObject::Type::ModelInstance) {}

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
    QMatrix4x4 getTransform(int index) const;

private:
    int instanceCount = 0;
    int instanceSerial = 0;
    int instanceStride = 0;
    bool transparency = false;
    bool depthSorting = false;
    QByteArray table;
};

QT_END_NAMESPACE

#endif // QSSGRENDERINDEXTABLE_P_H
