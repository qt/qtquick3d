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

#include "qquick3dparticlespriteparticle_p.h"

#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>

QT_BEGIN_NAMESPACE

QQuick3DParticleSpriteParticle::QQuick3DParticleSpriteParticle(QQuick3DNode *parent)
    : QQuick3DParticle(parent)
{
    QObject::connect(this, &QQuick3DParticle::maxAmountChanged, [this]() {
        handleMaxAmountChanged(m_maxAmount);
    });
}

QQuick3DParticleSpriteParticle::~QQuick3DParticleSpriteParticle()
{
    for (auto connection : qAsConst(m_connections))
        QObject::disconnect(connection);
}

QQuick3DParticleSpriteParticle::Lighting QQuick3DParticleSpriteParticle::lighting() const
{
    return m_lighting;
}

QQuick3DParticleSpriteParticle::BlendMode QQuick3DParticleSpriteParticle::blendMode() const
{
    return m_blendMode;
}

QQuick3DTexture *QQuick3DParticleSpriteParticle::sprite() const
{
    return m_sprite;
}

int QQuick3DParticleSpriteParticle::spriteImages() const
{
    return m_spriteImageCount;
}

bool QQuick3DParticleSpriteParticle::blendImages() const
{
    return m_blendImages;
}

bool QQuick3DParticleSpriteParticle::receivesShadows() const
{
    return m_receivesShadows;
}

void QQuick3DParticleSpriteParticle::setLighting(Lighting lighting)
{
    if (m_lighting == lighting)
        return;
    m_lighting = lighting;
    m_dirty = true;
    Q_EMIT lightingChanged();
}

void QQuick3DParticleSpriteParticle::setBlendMode(BlendMode blendMode)
{
    if (m_blendMode == blendMode)
        return;
    m_blendMode = blendMode;
    m_dirty = true;
    Q_EMIT blendModeChanged();
}

void QQuick3DParticleSpriteParticle::setSprite(QQuick3DTexture *sprite)
{
    if (m_sprite == sprite)
        return;

    auto *sceneManager = QQuick3DObjectPrivate::get(this)->sceneManager;
    QQuick3DObjectPrivate::updatePropertyListener(sprite, m_sprite, sceneManager,
                                                  QByteArrayLiteral("sprite"), m_connections,
                                                  [this](QQuick3DObject *n) {
        setSprite(qobject_cast<QQuick3DTexture *>(n));
    });

    m_sprite = sprite;
    m_dirty = true;
    Q_EMIT spriteChanged();
}

void QQuick3DParticleSpriteParticle::setSpriteImages(int imageCount)
{
    if (m_spriteImageCount == imageCount)
        return;
    m_spriteImageCount = imageCount;
    m_dirty = true;
    Q_EMIT spriteImagesChanged();
}

void QQuick3DParticleSpriteParticle::setBlendImages(bool blend)
{
    if (m_blendImages == blend)
        return;
    m_blendImages = blend;
    m_dirty = true;
    Q_EMIT blendImagesChanged();
}

void QQuick3DParticleSpriteParticle::setReceivesShadows(bool receive)
{
    if (m_receivesShadows == receive)
        return;
    m_receivesShadows = receive;
    m_dirty = true;
    Q_EMIT receivesShadowsChanged();
}


void QQuick3DParticleSpriteParticle::itemChange(QQuick3DObject::ItemChange change,
                                          const QQuick3DObject::ItemChangeData &value)
{
    if (change == QQuick3DObject::ItemSceneChange)
        updateSceneManager(value.sceneManager);
}

static QSSGRenderParticles::BlendMode mapBlendMode(QQuick3DParticleSpriteParticle::BlendMode mode)
{
    switch (mode) {
    case QQuick3DParticleSpriteParticle::SourceOver:
        return QSSGRenderParticles::BlendMode::SourceOver;
    case QQuick3DParticleSpriteParticle::Screen:
        return QSSGRenderParticles::BlendMode::Screen;
    case QQuick3DParticleSpriteParticle::Multiply:
        return QSSGRenderParticles::BlendMode::Multiply;
    default:
        Q_ASSERT(false);
        return QSSGRenderParticles::BlendMode::SourceOver;
    }
}

static QSSGRenderParticles::ParticleLighting mapLighting(QQuick3DParticleSpriteParticle::Lighting mode)
{
    switch (mode) {
    case QQuick3DParticleSpriteParticle::NoLighting:
        return QSSGRenderParticles::ParticleLighting::NoLighting;
    case QQuick3DParticleSpriteParticle::VertexLighting:
        return QSSGRenderParticles::ParticleLighting::VertexLighting;
    case QQuick3DParticleSpriteParticle::FragmentLighting:
        return QSSGRenderParticles::ParticleLighting::FragmentLighting;
    default:
        Q_ASSERT(false);
        return QSSGRenderParticles::ParticleLighting::NoLighting;
    }
}

QSSGRenderGraphObject *QQuick3DParticleSpriteParticle::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderParticles();
    }

    QQuick3DNode::updateSpatialNode(node);
    auto particles = static_cast<QSSGRenderParticles *>(node);

    updateParticleBuffer();

    if (!m_dirty)
        return particles;

    if (m_sprite) {
        particles->m_sprite = m_sprite->getRenderImage();
        particles->m_spriteImageCount = m_spriteImageCount;
        particles->m_blendImages = m_blendImages;
    } else {
        particles->m_sprite = nullptr;
        particles->m_spriteImageCount = 1;
        particles->m_blendImages = true;
    }

    particles->m_lighting = mapLighting(m_lighting);
    particles->m_blendMode = mapBlendMode(m_blendMode);
    particles->m_diffuseColor = color::sRGBToLinear(color());
    // TODO: Disabled for now
    //particles->m_receiveShadows = m_receivesShadows;

    m_dirty = false;
    return particles;
}

void QQuick3DParticleSpriteParticle::handleMaxAmountChanged(int amount)
{
    m_particleData.clear();
    for (int i = 0; i < amount; i++) {
        QQuick3DParticleData data;
        m_particleData.append(data);
    }
    m_spriteParticleData.resize(amount);
    m_spriteParticleData.fill({});
}

void QQuick3DParticleSpriteParticle::componentComplete()
{
    if (!system() && qobject_cast<QQuick3DParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QQuick3DParticleSystem*>(parentItem()));

    QQuick3DNode::componentComplete();
}

void QQuick3DParticleSpriteParticle::reset()
{

}

void QQuick3DParticleSpriteParticle::setParticleData(int particleIndex,
                     const QVector3D &position,
                     const QVector3D &rotation,
                     const QVector4D &color,
                     float size, float age)
{
    auto &dst = m_spriteParticleData[particleIndex];
    dst = {position, rotation, color, size, age};
}

void QQuick3DParticleSpriteParticle::updateParticleBuffer()
{
    const auto &particles = m_spriteParticleData;
    QSSGRenderParticles *node = static_cast<QSSGRenderParticles *>(QQuick3DObjectPrivate::get(this)->spatialNode);
    if (!node)
        return;
    const int particleCount = particles.size();
    if (node->m_particleBuffer.particleCount() != particleCount)
        node->m_particleBuffer.resize(particleCount);

    char *dest = node->m_particleBuffer.pointer();
    const SpriteParticleData *src = particles.data();
    const int pps = node->m_particleBuffer.particlesPerSlice();
    const int ss = node->m_particleBuffer.sliceStride();
    const int slices = node->m_particleBuffer.sliceCount();
    int i = 0;
    QSSGBounds3 bounds;
    for (int s = 0; s < slices; s++) {
        QSSGParticle *dp = reinterpret_cast<QSSGParticle *>(dest);
        for (int p = 0; p < pps && i < particleCount; i++, p++) {
            if (src->size > 0.0f)
                bounds.include(src->position);
            dp->position = src->position;
            dp->rotation = src->rotation * float(M_PI / 180.0f);
            dp->color = src->color;
            dp->size = src->size * 5.0f;
            dp->age = src->age;
            dp++;
            src++;
        }
        dest += ss;
    }
    node->m_particleBuffer.setBounds(bounds);
}

void QQuick3DParticleSpriteParticle::updateSceneManager(QQuick3DSceneManager *sceneManager)
{
    // Check all the resource value's scene manager, and update as necessary.
    if (sceneManager) {
        QQuick3DObjectPrivate::refSceneManager(m_sprite, *sceneManager);
    } else {
        QQuick3DObjectPrivate::derefSceneManager(m_sprite);
    }
}

QT_END_NAMESPACE
