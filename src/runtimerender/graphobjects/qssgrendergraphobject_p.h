// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGRENDERGRAPHOBJECT_P_H
#define QSSGRENDERGRAPHOBJECT_P_H

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

#include <ssg/qssgrendergraphobject.h>

#include <ssg/qssgrenderbasetypes.h>

#include <QtQuick3DUtils/private/qssgassert_p.h>

namespace QSSGRenderGraphObjectUtils {
constexpr QSSGResourceId getResourceId(const QSSGRenderGraphObject &o)
{
    QSSG_ASSERT(QSSGRenderGraphObject::isResource(o.type), return QSSGResourceId::Invalid);
    return QSSGResourceId{ quintptr(&o) };
}

template <typename T = QSSGRenderGraphObject>
T *getResource(QSSGResourceId resId)
{
    return static_cast<T *>(reinterpret_cast<QSSGRenderGraphObject *>(resId));
}

constexpr QSSGNodeId getNodeId(const QSSGRenderGraphObject &o)
{
    QSSG_ASSERT(QSSGRenderGraphObject::isNodeType(o.type), return QSSGNodeId::Invalid);
    return QSSGNodeId{ quintptr(&o) };
}

template <typename T = QSSGRenderGraphObject>
T *getNode(QSSGNodeId nodeId)
{
    return static_cast<T *>(reinterpret_cast<QSSGRenderGraphObject *>(nodeId));
}

constexpr QSSGCameraId getCameraId(const QSSGRenderGraphObject &o)
{
    QSSG_ASSERT(QSSGRenderGraphObject::isCamera(o.type), return QSSGCameraId::Invalid);
    return QSSGCameraId{ quintptr(&o) };
}

template <typename T = QSSGRenderGraphObject>
T *getCamera(QSSGCameraId cameraId)
{
    return static_cast<T *>(reinterpret_cast<QSSGRenderGraphObject *>(cameraId));
}

constexpr QSSGExtensionId getExtensionId(const QSSGRenderGraphObject &o)
{
    QSSG_ASSERT(QSSGRenderGraphObject::isExtension(o.type), return QSSGExtensionId::Invalid);
    return QSSGExtensionId{ quintptr(&o) };
}

template <typename T = QSSGRenderGraphObject>
T *getExtension(QSSGExtensionId extensionId)
{
    return static_cast<T *>(reinterpret_cast<QSSGRenderGraphObject *>(extensionId));
}

template <typename QSSGTypeId>
constexpr bool isNull(QSSGTypeId id) { return (id == QSSGTypeId::Invalid); }
}

#endif // QSSGRENDERGRAPHOBJECT_P_H
