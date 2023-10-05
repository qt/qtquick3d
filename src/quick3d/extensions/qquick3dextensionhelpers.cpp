// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dextensionhelpers.h"
#include "qquick3dobject_p.h"

#include <QtQuick3DUtils/private/qssgassert_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuick3DExtensionHelpers
    \inmodule QtQuick3D
    \since 6.6

    \brief Helper functions for the Extensions APIs.
*/

QQuick3DExtensionHelpers::QQuick3DExtensionHelpers()
{

}

/*!
    \return a \c QSSGNodeId that can be used to retrieve the object in the engine
    corresponding to \a node.

    //! \sa QSSGFrameData::getNode()
*/
QSSGNodeId QQuick3DExtensionHelpers::getNodeId(const QQuick3DObject &node)
{
    auto *po = QQuick3DObjectPrivate::get(&node);
    QSSG_ASSERT_X(QSSGRenderGraphObject::isNodeType(po->type), "Type is not a node", return QSSGNodeId::Invalid);
    // NOTE: Implementation detail (don't rely on this in user code).
    return static_cast<QSSGNodeId>(quintptr(QQuick3DObjectPrivate::get(&node)->spatialNode));
}

/*!
    \return a \c QSSGResourceId that can be used to retrieve the corresponding \a resource object
    in the engine.

    //! \sa QSSGFrameData::getResource()
*/
QSSGResourceId QQuick3DExtensionHelpers::getResourceId(const QQuick3DObject &resource)
{
    auto *po = QQuick3DObjectPrivate::get(&resource);
    QSSG_ASSERT_X(QSSGRenderGraphObject::isResource(po->type), "Type is not a resource", return QSSGResourceId::Invalid);
    // NOTE: Implementation detail (don't rely on this in user code).
    return static_cast<QSSGResourceId>(quintptr(QQuick3DObjectPrivate::get(&resource)->spatialNode));
}

/*!
    \return a \c QSSGCameraId that can be used to retrieve the corresponding \a camera object
    in the engine.

    //! \sa QSSGFrameData::getNode()
*/
QSSGCameraId QQuick3DExtensionHelpers::getCameraId(const QQuick3DObject &camera)
{
    auto *po = QQuick3DObjectPrivate::get(&camera);
    QSSG_ASSERT_X(QSSGRenderGraphObject::isCamera(po->type), "Type is not a camera", return QSSGCameraId::Invalid);
    // NOTE: Implementation detail (don't rely on this in user code).
    return static_cast<QSSGCameraId>(quintptr(po->spatialNode));
}

QT_END_NAMESPACE
