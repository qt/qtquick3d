/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>

QT_BEGIN_NAMESPACE

static int divisibleBy(int a, int b)
{
    return (a % b) ? a + b - (a % b) : a;
}
static int ceilDivide(int a, int b)
{
    int x = a / b;
    int y = (a % b) ? 1 : 0;
    return x + y;
}

void QSSGParticleBuffer::resize(int particleCount)
{
    int vec4PerParticle = ceilDivide(sizeof(QSSGParticle), 16);
    int vec4s = particleCount * vec4PerParticle;
    int width = divisibleBy(sqrt(vec4s), vec4PerParticle);
    int height = ceilDivide(vec4s, width);
    m_particlesPerSlice = width / vec4PerParticle;
    m_particleCount = particleCount;
    width = divisibleBy(width, 4);
    height = divisibleBy(height, 4);
    m_sliceStride = width * 16;
    m_size = QSize(width, height);
    m_particleBuffer.resize(m_sliceStride * height);
}

void QSSGParticleBuffer::setBounds(const QSSGBounds3& bounds)
{
    m_bounds = bounds;
}

char *QSSGParticleBuffer::pointer()
{
    return m_particleBuffer.data();
}

int QSSGParticleBuffer::particlesPerSlice() const
{
    return m_particlesPerSlice;
}

int QSSGParticleBuffer::sliceStride() const
{
    return m_sliceStride;
}

int QSSGParticleBuffer::particleCount() const
{
    return m_particleCount;
}

QSize QSSGParticleBuffer::size() const
{
    return m_size;
}

int QSSGParticleBuffer::sliceCount() const
{
    return m_size.height();
}

QByteArray QSSGParticleBuffer::data() const
{
    return m_particleBuffer;
}

QSSGBounds3 QSSGParticleBuffer::bounds() const
{
    return m_bounds;
}

QSSGRenderParticles::QSSGRenderParticles()
    : QSSGRenderNode(QSSGRenderGraphObject::Type::Particles)
{

}

QT_END_NAMESPACE
