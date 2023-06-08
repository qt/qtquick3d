// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dextensionhelpers_p.h"
#include "qquick3dobject_p.h"

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
    // NOTE: Implementation detail (don't rely on this in user code).
    return quintptr(QQuick3DObjectPrivate::get(&node)->spatialNode);
}

/*!
    \return a \c QSSGResourceId that can be used to retrieve the corresponding \a resource object
    in the engine.

    //! \sa QSSGFrameData::getResource()
*/
QSSGResourceId QQuick3DExtensionHelpers::getResourceId(const QQuick3DObject &resource)
{
    // NOTE: Implementation detail (don't rely on this in user code).
    return quintptr(QQuick3DObjectPrivate::get(&resource)->spatialNode);
}

QT_END_NAMESPACE
