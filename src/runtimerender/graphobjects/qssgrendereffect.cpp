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

#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffectsystem_p.h>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

QT_BEGIN_NAMESPACE

QSSGRenderEffect::QSSGRenderEffect() : QSSGRenderGraphObject(Type::Effect) {}

void QSSGRenderEffect::initialize()
{
    m_layer = nullptr;
    m_nextEffect = nullptr;
}

void QSSGRenderEffect::setActive(bool inActive, QSSGEffectSystem &inManager)
{
    if (flags.testFlag(Flag::Active) != inActive) {
        flags.setFlag(Flag::Active, inActive);
        if (m_context)
            inManager.resetEffectFrameData(*m_context);
        flags.setFlag(Flag::Dirty);
    }
}

void QSSGRenderEffect::reset(QSSGEffectSystem &inSystem)
{
    if (m_context)
        inSystem.resetEffectFrameData(*m_context);
    flags.setFlag(Flag::Dirty);
}

QT_END_NAMESPACE
