/****************************************************************************
**
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

#include "qquick3ddirectionallight_p.h"
#include "qquick3dobject_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype DirectionalLight
    \inherits Light
    \inqmlmodule QtQuick3D
    \brief Defines a directional light in the scene.

    The directional light emits light in one direction from an unidentifiable source located
    infinitely far away. This is similar to the way sunlight works in real life. A directional
    light has infinite range and does not diminish.

    If \l {Light::castsShadow}{castsShadow} is enabled, shadows will be parallel to the light
    direction.

    Moving a directional light does not have any effect. The light will always be emitted in the
    direction of the light's Z axis.

    Rotating the light along its X or Y axis will change the direction of the light emission.

    Scaling a directional light will only have an effect in the following cases:
    \list
    \li If Z scale is set to a negative number, the light will be emitted in the opposite direction.
    \li If the scale of any axis is set to 0, the light will be emitted along the world's Z axis.
    \note Rotating the light will then have no effect.
    \endlist

    \sa AreaLight, PointLight, SpotLight
*/

QSSGRenderGraphObject *QQuick3DDirectionalLight::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderLight();
        QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);
        light->m_lightType = QSSGRenderLight::Type::Directional;
    }

    QQuick3DAbstractLight::updateSpatialNode(node);

    return node;
}

QT_END_NAMESPACE
