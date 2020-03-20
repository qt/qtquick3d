/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qquick3dspotlight_p.h"
#include "qquick3dobject_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SpotLight
    \inherits Light
    \inqmlmodule QtQuick3D
    \brief Defines a spot light in the scene.
    \since 5.15

    The spot light emits light towards one direction in a cone shape, which is defined by the
    \l {coneAngle} property. The light intensity diminishes when approaching the \l {coneAngle}.
    The angle at which the light intensity starts to diminish is defined by \l {innerConeAngle}.
    Both angles are defined in degrees.

    Inside the \l {innerConeAngle}, the spot light behaves similarly to the point light.
    There the light intensity diminishes according to inverse-square-law. However, the fade-off
    (and range) can be controlled with the \l {constantFade}, \l {linearFade}, and
    \l quadraticFade properties.

    \sa DirectionalLight, PointLight, AreaLight
*/

/*!
    \qmlproperty real SpotLight::constantFade

    This property is constant factor of the attenuation term of the light.
    The default value is 1.0.
 */

/*!
    \qmlproperty real SpotLight::linearFade

    This property increases the rate at which the lighting effect dims the light
    in proportion to the distance to the light. The default value is 0.0, which means the light
    doesn't have linear fade.
*/

/*!
    \qmlproperty real SpotLight::quadraticFade

    This property increases the rate at which the lighting effect dims the light
    in proportion to the inverse square law. The default value is 1.0, which means the spot light
    fade exactly follows the inverse square law, i.e. when distance to an object doubles the
    light intensity decreases to 1/4th.
*/

/*!
    \qmlproperty real SpotLight::coneAngle

    This property defines the cut-off angle beyond which the light doesn't affect the scene.
    Defined in degrees between 0 and 180. The default value is 40.
*/

/*!
    \qmlproperty real SpotLight::innerConeAngle

    This property defines the angle at which the light intensity starts to gradually diminish
    as it approaches \l {coneAngle}. Defined in degrees between 0 and 180. If the value is set
    larger than \l {coneAngle}, it'll behave as if it had the same value as \l {coneAngle}.
    The default value is 30.
*/

float QQuick3DSpotLight::constantFade() const
{
    return m_constantFade;
}

float QQuick3DSpotLight::linearFade() const
{
    return m_linearFade;
}

float QQuick3DSpotLight::quadraticFade() const
{
    return m_quadraticFade;
}

float QQuick3DSpotLight::coneAngle() const
{
    return m_coneAngle;
}

float QQuick3DSpotLight::innerConeAngle() const
{
    return m_innerConeAngle;
}

void QQuick3DSpotLight::setConstantFade(float constantFade)
{
    if (qFuzzyCompare(m_constantFade, constantFade))
        return;

    m_constantFade = constantFade;
    m_dirtyFlags.setFlag(DirtyFlag::FadeDirty);
    emit constantFadeChanged();
    update();
}

void QQuick3DSpotLight::setLinearFade(float linearFade)
{
    if (qFuzzyCompare(m_linearFade, linearFade))
        return;

    m_linearFade = linearFade;
    m_dirtyFlags.setFlag(DirtyFlag::FadeDirty);
    emit linearFadeChanged();
    update();
}

void QQuick3DSpotLight::setQuadraticFade(float quadraticFade)
{
    if (qFuzzyCompare(m_quadraticFade, quadraticFade))
        return;

    m_quadraticFade = quadraticFade;
    m_dirtyFlags.setFlag(DirtyFlag::FadeDirty);
    emit quadraticFadeChanged();
    update();
}

void QQuick3DSpotLight::setConeAngle(float coneAngle)
{
    if (coneAngle < 0.f)
        coneAngle = 0.f;
    else if (coneAngle > 180.f)
        coneAngle = 180.f;

    if (qFuzzyCompare(m_coneAngle, coneAngle))
        return;

    m_coneAngle = coneAngle;
    m_dirtyFlags.setFlag(DirtyFlag::AreaDirty);
    emit coneAngleChanged();
    update();
}

void QQuick3DSpotLight::setInnerConeAngle(float innerConeAngle)
{
    if (innerConeAngle < 0.f)
        innerConeAngle = 0.f;
    else if (innerConeAngle > 180.f)
        innerConeAngle = 180.f;

    if (qFuzzyCompare(m_innerConeAngle, innerConeAngle))
        return;

    m_innerConeAngle = innerConeAngle;
    m_dirtyFlags.setFlag(DirtyFlag::AreaDirty);
    emit innerConeAngleChanged();
    update();
}

QSSGRenderGraphObject *QQuick3DSpotLight::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        node = new QSSGRenderLight();
        QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);
        light->m_lightType = QSSGRenderLight::Type::Spot;
    }

    QQuick3DAbstractLight::updateSpatialNode(node);

    QSSGRenderLight *light = static_cast<QSSGRenderLight *>(node);

    if (m_dirtyFlags.testFlag(DirtyFlag::FadeDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::FadeDirty, false);
        light->m_constantFade = m_constantFade;
        light->m_linearFade = m_linearFade;
        light->m_quadraticFade = m_quadraticFade;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::AreaDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::AreaDirty, false);
        light->m_coneAngle = m_coneAngle;
        light->m_innerConeAngle = m_innerConeAngle;
    }

    return node;
}

QT_END_NAMESPACE
