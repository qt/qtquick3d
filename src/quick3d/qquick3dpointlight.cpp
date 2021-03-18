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

#include "qquick3dpointlight_p.h"
#include "qquick3dobject_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointLight
    \inherits Light
    \inqmlmodule QtQuick3D
    \brief Defines a point light in the scene.

    The point light can be described as a sphere, emitting light with equal strength in all
    directions from the center of the light. This is similar to the way a light bulb emits light.

    Rotating or scaling a point light does not have any effect. Moving a point light will change
    the position from where the light is emitted.

    By default, a point light intensity diminishes according to inverse-square-law. However, the fade-off
    (and range) can be controlled with the \l {constantFade}, \l {linearFade}, and
    \l quadraticFade properties.

    \sa AreaLight, DirectionalLight, SpotLight
*/

/*!
    \qmlproperty real PointLight::constantFade

    This property is constant factor of the attenuation term of the light.
    The default value is 1.0.
 */

/*!
    \qmlproperty real PointLight::linearFade

    This property increases the rate at which the lighting effect dims the light
    in proportion to the distance to the light. The default value is 0.0 meaning the light doesn't
    have linear fade.
*/

/*!
    \qmlproperty real PointLight::quadraticFade

    This property increases the rate at which the lighting effect dims the light
    in proportion to the inverse square law. The default value is 1.0 meaning the point light
    fade exactly follows the inverse square law i.e. when distance to an object doubles the
    light intensity decreases to 1/4th.
*/

float QQuick3DPointLight::constantFade() const
{
    return m_constantFade;
}

float QQuick3DPointLight::linearFade() const
{
    return m_linearFade;
}

float QQuick3DPointLight::quadraticFade() const
{
    return m_quadraticFade;
}

void QQuick3DPointLight::setConstantFade(float constantFade)
{
    if (qFuzzyCompare(m_constantFade, constantFade))
        return;

    m_constantFade = constantFade;
    m_dirtyFlags.setFlag(DirtyFlag::FadeDirty);
    emit constantFadeChanged();
    update();
}

void QQuick3DPointLight::setLinearFade(float linearFade)
{
    if (qFuzzyCompare(m_linearFade, linearFade))
        return;

    m_linearFade = linearFade;
    m_dirtyFlags.setFlag(DirtyFlag::FadeDirty);
    emit linearFadeChanged();
    update();
}

void QQuick3DPointLight::setQuadraticFade(float quadraticFade)
{
    if (qFuzzyCompare(m_quadraticFade, quadraticFade))
        return;

    m_quadraticFade = quadraticFade;
    m_dirtyFlags.setFlag(DirtyFlag::FadeDirty);
    emit quadraticFadeChanged();
    update();
}

QSSGRenderGraphObject *QQuick3DPointLight::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderLight();
        QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);
        light->m_lightType = QSSGRenderLight::Type::Point;
    }

    QQuick3DAbstractLight::updateSpatialNode(node);

    QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);

    if (m_dirtyFlags.testFlag(DirtyFlag::FadeDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::FadeDirty, false);
        light->m_constantFade = m_constantFade;
        light->m_linearFade = m_linearFade;
        light->m_quadraticFade = m_quadraticFade;
    }

    return node;
}

QT_END_NAMESPACE
