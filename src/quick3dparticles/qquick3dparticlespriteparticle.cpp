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

/*!
    \qmltype SpriteParticle3D
    \inherits Particle3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Particle using a 2D sprite texture.

    The SpriteParticle3D is a logical particle element that creates particles
    from a 2D sprite texture.
*/

QQuick3DParticleSpriteParticle::QQuick3DParticleSpriteParticle(QQuick3DNode *parent)
    : QQuick3DParticle(parent)
{
    m_connections.insert("maxAmount", QObject::connect(this, &QQuick3DParticle::maxAmountChanged, [this]() {
        handleMaxAmountChanged(m_maxAmount);
    }));
    m_connections.insert("system", QObject::connect(this, &QQuick3DParticle::systemChanged, [this]() {
       handleSystemChanged(system());
    }));
}

QQuick3DParticleSpriteParticle::~QQuick3DParticleSpriteParticle()
{
    for (const auto &connection : qAsConst(m_connections))
        QObject::disconnect(connection);
    if (m_particleUpdateNode) {
        m_particleUpdateNode->m_spriteParticle = nullptr;
        delete m_particleUpdateNode;
    }
}

/*!
    \qmlproperty enumeration SpriteParticle3D::BlendMode

    Defines the blending mode for the particles.

    \value SpriteParticle3D.SourceOver
        Blend particles with SourceOver mode.
    \value SpriteParticle3D.Screen
        Blend particles with Screen mode.
    \value SpriteParticle3D.Multiply
        Blend particles with Multiply mode.
*/

/*!
    \qmlproperty BlendMode SpriteParticle3D::blendMode

    This property defines the blending mode used for rendering the particles.

    The default value is \c SpriteParticle3D.SourceOver.
*/
QQuick3DParticleSpriteParticle::BlendMode QQuick3DParticleSpriteParticle::blendMode() const
{
    return m_blendMode;
}

/*!
    \qmlproperty Texture SpriteParticle3D::sprite

    This property defines the \l Texture used for the particles.

    For example, to use "snowFlake.png" as the particles texture:

    \qml
    SpriteParticle3D {
        id: snowParticle
        ...
        sprite: Texture {
            source: "images/snowflake.png"
        }
    }
    \endqml
*/
QQuick3DTexture *QQuick3DParticleSpriteParticle::sprite() const
{
    return m_sprite;
}

/*!
    \qmlproperty int SpriteParticle3D::frameCount

    This property defines the amount of image frames in \l sprite. Particle animates
    through these frames during its \l {ParticleEmitter3D::lifeSpan}{lifeSpan}.
    The frames should be laid out horizontally in the same image file. For example,
    \l sprite could be a \c {512x64} image, with \c frameCount of \c 8. If the particle
    \l {ParticleEmitter3D::lifeSpan}{lifeSpan} is \c 2000, each of those images will be
    visible for \c 250 milliseconds.

    The default value is \c 1.

    \sa interpolate
*/
int QQuick3DParticleSpriteParticle::frameCount() const
{
    return m_frameCount;
}

/*!
    \qmlproperty bool SpriteParticle3D::interpolate

    This property defines if the sprites are interpolated (blended) between frames
    to make the animation appear smoother.

    \note This property doesn't have an effect when the sprite has only a single
    frame (so \l frameCount is \c 1).

    The default value is \c true.

    \sa frameCount
*/
bool QQuick3DParticleSpriteParticle::interpolate() const
{
    return m_interpolate;
}

/*!
    \qmlproperty bool SpriteParticle3D::billboard

    This property defines if the particle texture should always be aligned
    face towards the screen.

    \note When set to \c true, \l Particle3D \l {Particle3D::alignMode}{alignMode}
    property does not have an effect.

    The default value is \c false.
*/
bool QQuick3DParticleSpriteParticle::billboard() const
{
    return m_billboard;
}

/*!
    \qmlproperty real SpriteParticle3D::particleScale

    This property defines the scale multiplier of the particles.
    To adjust the particles sizes in the emitter, use \ ParticleEmitter3D
    \l {ParticleEmitter3D::particleScale}{particleScale},
    \l {ParticleEmitter3D::particleEndScale}{particleEndScale}, and
    \l {ParticleEmitter3D::particleScaleVariation}{particleScaleVariation}
    properties.

    The default value is \c 5.0.
*/
float QQuick3DParticleSpriteParticle::particleScale() const
{
    return m_particleScale;
}

/*!
    \qmlproperty Texture SpriteParticle3D::colorTable

    This property defines the \l Texture used for coloring the particles.
    The image can be a 1D or a 2D texture. Horizontal pixels determine the particle color over its
    \l {ParticleEmitter3D::lifeSpan}{lifeSpan}. For example, when the particle is halfway through
    its life, it will have the color specified halfway across the image. If the image is 2D,
    vertical row is randomly selected for each particle. For example, a c {256 x 4} image
    contains \c 4 different coloring options for particles.
*/
QQuick3DTexture *QQuick3DParticleSpriteParticle::colorTable() const
{
    return m_colorTable;
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

    auto sceneManager = QQuick3DObjectPrivate::get(this)->sceneManager;
    QQuick3DObjectPrivate::updatePropertyListener(sprite, m_sprite, sceneManager,
                                                  QByteArrayLiteral("sprite"), m_connections,
                                                  [this](QQuick3DObject *n) {
        setSprite(qobject_cast<QQuick3DTexture *>(n));
    });

    m_sprite = sprite;
    m_dirty = true;
    Q_EMIT spriteChanged();
}

void QQuick3DParticleSpriteParticle::setFrameCount(int frameCount)
{
    if (m_frameCount == frameCount)
        return;
    m_frameCount = frameCount;
    m_dirty = true;
    Q_EMIT frameCountChanged();
}

void QQuick3DParticleSpriteParticle::setInterpolate(bool interpolate)
{
    if (m_interpolate == interpolate)
        return;
    m_interpolate = interpolate;
    m_dirty = true;
    Q_EMIT interpolateChanged();
}

void QQuick3DParticleSpriteParticle::setBillboard(bool billboard)
{
    if (m_billboard == billboard)
        return;
    m_billboard = billboard;
    m_dirty = true;
    Q_EMIT billboardChanged();
}

void QQuick3DParticleSpriteParticle::setParticleScale(float scale)
{
    if (qFuzzyCompare(scale, m_particleScale))
        return;
    m_particleScale = scale;
    m_dirty = true;
    Q_EMIT particleScaleChanged();
}

void QQuick3DParticleSpriteParticle::setColorTable(QQuick3DTexture *colorTable)
{
    if (m_colorTable == colorTable)
        return;

    auto sceneManager = QQuick3DObjectPrivate::get(this)->sceneManager;
    QQuick3DObjectPrivate::updatePropertyListener(colorTable, m_colorTable, sceneManager,
                                                  QByteArrayLiteral("colorTable"), m_connections,
                                                  [this](QQuick3DObject *n) {
        setColorTable(qobject_cast<QQuick3DTexture *>(n));
    });

    m_colorTable = colorTable;
    m_dirty = true;
    Q_EMIT colorTableChanged();
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

QSSGRenderGraphObject *QQuick3DParticleSpriteParticle::ParticleUpdateNode::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (m_spriteParticle) {
        node = m_spriteParticle->updateParticleNode(node);
        QQuick3DNode::updateSpatialNode(node);
    }
    return node;
}

QSSGRenderGraphObject *QQuick3DParticleSpriteParticle::updateParticleNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderParticles();
    }

    auto particles = static_cast<QSSGRenderParticles *>(node);

    updateParticleBuffer(particles);

    if (!m_dirty)
        return particles;

    if (m_sprite) {
        particles->m_sprite = m_sprite->getRenderImage();
        particles->m_spriteImageCount = m_frameCount;
        particles->m_blendImages = m_interpolate;
    } else {
        particles->m_sprite = nullptr;
        particles->m_spriteImageCount = 1;
        particles->m_blendImages = true;
    }

    if (m_colorTable)
        particles->m_colorTable = m_colorTable->getRenderImage();
    else
        particles->m_colorTable = nullptr;

    particles->m_blendMode = mapBlendMode(m_blendMode);
    particles->m_diffuseColor = color::sRGBToLinear(color());
    particles->m_billboard = m_billboard;

    m_dirty = false;
    return particles;
}

void QQuick3DParticleSpriteParticle::handleMaxAmountChanged(int amount)
{
    if (m_particleData.size() == amount)
        return;

    m_particleData.resize(amount);
    m_particleData.fill({});
    m_spriteParticleData.resize(amount);
    m_spriteParticleData.fill({});
}

void QQuick3DParticleSpriteParticle::handleSystemChanged(QQuick3DParticleSystem *system)
{
    if (m_particleUpdateNode)
        delete m_particleUpdateNode;
    m_particleUpdateNode = new ParticleUpdateNode(system);
    m_particleUpdateNode->m_spriteParticle = this;
}

void QQuick3DParticleSpriteParticle::componentComplete()
{
    if (!system() && qobject_cast<QQuick3DParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QQuick3DParticleSystem*>(parentItem()));

    QQuick3DParticle::componentComplete();
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

void QQuick3DParticleSpriteParticle::updateParticleBuffer(QSSGRenderGraphObject *spatialNode)
{
    const auto &particles = m_spriteParticleData;
    QSSGRenderParticles *node = static_cast<QSSGRenderParticles *>(spatialNode);
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
            dp->size = src->size * m_particleScale;
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
        QQuick3DObjectPrivate::refSceneManager(m_colorTable, *sceneManager);
    } else {
        QQuick3DObjectPrivate::derefSceneManager(m_sprite);
        QQuick3DObjectPrivate::derefSceneManager(m_colorTable);
    }
}

QT_END_NAMESPACE
