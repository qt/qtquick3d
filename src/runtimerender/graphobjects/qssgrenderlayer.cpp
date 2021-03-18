/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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


#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderLayer::QSSGRenderLayer()
    : QSSGRenderNode(QSSGRenderNode::Type::Layer)
    , firstEffect(nullptr)
    , antialiasingMode(QSSGRenderLayer::AAMode::NoAA)
    , antialiasingQuality(QSSGRenderLayer::AAQuality::High)
    , background(QSSGRenderLayer::Background::Transparent)
    , blendType(QSSGRenderLayer::BlendMode::SourceOver)
    , horizontalFieldValues(QSSGRenderLayer::HorizontalField::LeftWidth)
    , m_left(0)
    , leftUnits(QSSGRenderLayer::UnitType::Percent)
    , m_width(100.0f)
    , widthUnits(QSSGRenderLayer::UnitType::Percent)
    , m_right(0)
    , rightUnits(QSSGRenderLayer::UnitType::Percent)
    , verticalFieldValues(QSSGRenderLayer::VerticalField::TopHeight)
    , m_top(0)
    , topUnits(QSSGRenderLayer::UnitType::Percent)
    , m_height(100.0f)
    , heightUnits(QSSGRenderLayer::UnitType::Percent)
    , m_bottom(0)
    , bottomUnits(QSSGRenderLayer::UnitType::Percent)
    , aoStrength(0)
    , aoDistance(5.0f)
    , aoSoftness(50.0f)
    , aoBias(0)
    , aoSamplerate(2)
    , aoDither(false)
    , shadowStrength(0)
    , shadowDist(10)
    , shadowSoftness(100.0f)
    , shadowBias(0)
    , lightProbe(nullptr)
    , probeBright(100.0f)
    , fastIbl(false)
    , probeHorizon(-1.0f)
    , probeFov(180.0f)
    , lightProbe2(nullptr)
    , probe2Fade(1.0f)
    , probe2Window(1.0f)
    , probe2Pos(0.5f)
    , temporalAAEnabled(false)
    , temporalAAStrength(0.3f)
    , ssaaMultiplier(1.5f)
    , activeCamera(nullptr)
    , renderedCamera(nullptr)
{
    flags.setFlag(Flag::LayerRenderToTarget);
    flags.setFlag(Flag::LayerEnableDepthTest);
    flags.setFlag(Flag::LayerEnableDepthPrePass);
}

void QSSGRenderLayer::addEffect(QSSGRenderEffect &inEffect)
{
    // Effects need to be rendered in reverse order as described in the file.
    inEffect.m_nextEffect = firstEffect;
    firstEffect = &inEffect;
    inEffect.m_layer = this;
}

QSSGRenderEffect *QSSGRenderLayer::getLastEffect()
{
    if (firstEffect) {
        QSSGRenderEffect *theEffect = firstEffect;
        // Empty loop intentional
        for (; theEffect->m_nextEffect; theEffect = theEffect->m_nextEffect);
        Q_ASSERT(theEffect->m_nextEffect == nullptr);
        return theEffect;
    }
    return nullptr;
}

QT_END_NAMESPACE
