/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSSG_RENDER_LIGHT_H
#define QSSG_RENDER_LIGHT_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendernode_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderImage;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderLight : public QSSGRenderNode
{
    QSSGRenderNode *m_scope;
    QVector3D m_diffuseColor; // colors are 0-1 normalized
    QVector3D m_specularColor; // colors are 0-1 normalized
    QVector3D m_ambientColor; // colors are 0-1 normalized

    // The variables below are in the same range as Studio
    // Only valid if node is a point light
    float m_brightness;
    float m_constantFade;
    float m_linearFade;
    float m_quadraticFade;

    float m_coneAngle; // 0-180
    float m_innerConeAngle; // 0-180

    bool m_castShadow; // true if this light produce shadows
    float m_shadowBias; // depth shift to avoid self-shadowing artifacts
    float m_shadowFactor; // Darkening factor for ESMs
    quint32 m_shadowMapRes; // Resolution of shadow map
    float m_shadowMapFar; // Far clip plane for the shadow map
    float m_shadowFilter; // Shadow map filter step size

    // Defaults to directional light
    explicit QSSGRenderLight(Type type = Type::DirectionalLight);
};
QT_END_NAMESPACE

#endif
