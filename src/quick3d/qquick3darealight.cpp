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

#include "qquick3darealight_p.h"
#include "qquick3dobject_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype AreaLight
    \inherits Light
    \inqmlmodule QtQuick3D
    \brief Defines an area light in the scene.

    The area light is similar to the directional light, but instead of emitting equally bright
    light across the whole scene, the area light emits directional light from a rectangle shaped
    object. Aside from the size, an area light has the same characteristics and properties as
    the directional light.

    Rotating, scaling and moving actions will all effect an area light.

    \sa DirectionalLight, PointLight, SpotLight
*/

/*!
 * \qmlproperty real AreaLight::width
 *
 * This property controls the width of an Area lights rectangle.
 *
 */

/*!
 * \qmlproperty real AreaLight::height
 *
 * This property controls the height of an Area lights rectangle
 *
 */

float QQuick3DAreaLight::width() const
{
    return m_width;
}

float QQuick3DAreaLight::height() const
{
    return m_height;
}

void QQuick3DAreaLight::setWidth(float width)
{
    if (qFuzzyCompare(m_width, width))
        return;

    m_width = width;
    m_dirtyFlags.setFlag(DirtyFlag::AreaDirty);
    emit widthChanged();
    update();
}

void QQuick3DAreaLight::setHeight(float height)
{
    if (qFuzzyCompare(m_height, height))
        return;

    m_height = height;
    m_dirtyFlags.setFlag(DirtyFlag::AreaDirty);
    emit heightChanged();
    update();
}

QSSGRenderGraphObject *QQuick3DAreaLight::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderLight();
        QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);
        light->m_lightType = QSSGRenderLight::Type::Area;
    }

    QQuick3DAbstractLight::updateSpatialNode(node);

    QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);

    if (m_dirtyFlags.testFlag(DirtyFlag::AreaDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::AreaDirty, false);
        light->m_areaWidth = m_width;
        light->m_areaHeight = m_height;
    }

    return node;
}

QT_END_NAMESPACE
