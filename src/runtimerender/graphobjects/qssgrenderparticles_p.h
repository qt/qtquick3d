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

#ifndef QSSG_RENDER_PARTICLES_H
#define QSSG_RENDER_PARTICLES_H

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
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

QT_BEGIN_NAMESPACE

struct QSSGRenderImage;
struct QSSGShaderMaterialAdapter;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGParticle
{
    QVector3D position;
    float size;
    QVector3D rotation;
    float age;
    QVector4D color;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGParticleBuffer
{
    void resize(int particleCount);
    void setBounds(const QSSGBounds3& bounds);

    char *pointer();
    int particlesPerSlice() const;
    int sliceStride() const;
    int particleCount() const;
    int sliceCount() const;
    QSize size() const;
    QByteArray data() const;
    QSSGBounds3 bounds() const;
private:
    int m_particlesPerSlice = 0;
    int m_sliceStride = 0;
    int m_particleCount = 0;
    QSize m_size;
    QByteArray m_particleBuffer;
    QSSGBounds3 m_bounds;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderParticles : public QSSGRenderNode
{
    enum class ParticleLighting : quint8
    {
        NoLighting = 0,
        VertexLighting,
        FragmentLighting
    };
    enum class BlendMode : quint8
    {
        SourceOver = 0,
        Screen,
        Multiply
    };

    Q_DISABLE_COPY(QSSGRenderParticles)

    QSSGParticleBuffer m_particleBuffer;

    QSSGRenderParticles::ParticleLighting m_lighting = ParticleLighting::NoLighting;
    QSSGRenderParticles::BlendMode m_blendMode = BlendMode::SourceOver;
    QVector4D m_diffuseColor{1.0f, 1.0f, 1.0f, 1.0f};
    QSSGRenderImage *m_sprite = nullptr;
    int m_spriteImageCount = 1;
    bool m_blendImages = true;
    bool m_billboard = true;

    QSSGRenderParticles();
    ~QSSGRenderParticles() = default;
};


QT_END_NAMESPACE

#endif
