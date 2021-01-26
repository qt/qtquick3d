/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderLight::QSSGRenderLight()
    : QSSGRenderNode(QSSGRenderGraphObject::Type::Light)
    , m_lightType(QSSGRenderLight::Type::Directional)
    , m_scope(nullptr)
    , m_diffuseColor(1, 1, 1)
    , m_specularColor(1, 1, 1)
    , m_ambientColor(0, 0, 0)
    , m_brightness(100)
    , m_constantFade(1)
    , m_linearFade(0)
    , m_quadraticFade(1)
    , m_areaWidth(100)
    , m_areaHeight(100)
    , m_coneAngle(40.0f)
    , m_innerConeAngle(30.0f)
    , m_castShadow(false)
    , m_shadowBias(0.0f)
    , m_shadowFactor(5.0f)
    , m_shadowMapRes(9)
    , m_shadowMapFar(5000.0f)
    , m_shadowFilter(35.0f)
{
    flags.setFlag(Flag::PointLight, false);
}

QT_END_NAMESPACE
