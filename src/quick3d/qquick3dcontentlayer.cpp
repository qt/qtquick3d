// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qquick3dcontentlayer_p.h"

/*!
    \qmltype ContentLayer
    \inqmlmodule QtQuick3D
    \since 6.11
    \brief Provides enumeration of content layers available in a QtQuick3D.

 This class is used to represent a layer of content in a 3D scene. It can be used
 to manage different layers of content, such as background, foreground, or UI elements.

 The \l ContentLayer enumeration defines the available layer flags that can be used,
 for example, to group scene nodes together or filter which nodes are rendered by the camera.

 The predefined layer names are not necessarily descriptive enough for specific use cases, in
 which case you can define your own layers by aliasing the \l ContentLayer enumeration values.
 Consider a city scene where you have different layers for buildings, streets, and vehicles.
 You can assign each of these layers a specific flag, such as \c layerBuildings, \c layerStreets,
 and \c layerVehicles by aliasing the \l ContentLayer enumeration values. Once the layers are defined,
 you can assign these flags to the nodes in the scene and have the camera filter which layers to render
 by setting the same \l {Node::layer}{layer} property on the \l Camera.

 \qml
 readonly property int layerBuildings: ContentLayer.Layer1
 readonly property int layerStreets: ContentLayer.Layer2
 readonly property int layerVehicles: ContentLayer.Layer3 | ContentLayer.Layer4
 \endqml

 \sa {Qt Quick 3D - Layers Example}{Layers Example}

 \note Some of the layers have special meanings:

 \table
 \header
     \li Layer
     \li Description
 \row
     \li LayerNone
     \li No layer assigned, used to indicate that a node does not belong to any layer and should not be rendered.
 \row
     \li Layer0
     \li The main layer, used for the primary content in the scene. Nodes are assigned to this layer by default.
 \row
     \li Layer1-23
     \li Freely assignable layers, used for grouping nodes together based on their purpose or functionality.
 \row
     \li Layer24 and up
     \li Reserved layers, used for special purposes or future extensions.
 \row
     \li LayerAll
     \li Indicates that a node belongs to all layers, used for nodes that should be rendered in every layer.
 \endtable

 \note The layers are defined as flags, which means that you can combine multiple layers

 \note This class is not intended to be instantiated directly. Instead, it is used as a
 singleton in QML to access the layer flags.

 */

QT_BEGIN_NAMESPACE

QQuick3DContentLayer::QQuick3DContentLayer(QObject *parent)
    : QObject(parent)
{

}

QQuick3DContentLayer::~QQuick3DContentLayer()
{

}

QT_END_NAMESPACE
