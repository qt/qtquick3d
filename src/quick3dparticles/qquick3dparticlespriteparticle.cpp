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
#include "qquick3dparticleemitter_p.h"

#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderparticles_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SpriteParticle3D
    \inherits Particle3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Particle using a 2D sprite texture.
    \since 6.2

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
    m_connections.insert("sortMode", QObject::connect(this, &QQuick3DParticle::sortModeChanged, [this]() {
        markNodesDirty();
    }));
}

QQuick3DParticleSpriteParticle::~QQuick3DParticleSpriteParticle()
{
    if (m_spriteSequence)
        m_spriteSequence->m_parentParticle = nullptr;
    for (const auto &connection : qAsConst(m_connections))
        QObject::disconnect(connection);
    deleteNodes();
}

void QQuick3DParticleSpriteParticle::deleteNodes()
{
    for (PerEmitterData &value : m_perEmitterData) {
        value.particleUpdateNode->m_spriteParticle = nullptr;
        delete value.particleUpdateNode;
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
    \qmlproperty SpriteSequence3D SpriteParticle3D::spriteSequence

    This property defines the sprite sequence properties for the particle.
    If the \l sprite texture contains a frame sequence, set this property
    to define the frame count, animation direction etc. features.
*/

QQuick3DParticleSpriteSequence *QQuick3DParticleSpriteParticle::spriteSequence() const
{
    return m_spriteSequence;
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
    markNodesDirty();
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
    markNodesDirty();
    Q_EMIT spriteChanged();
}

void QQuick3DParticleSpriteParticle::setSpriteSequence(QQuick3DParticleSpriteSequence *spriteSequence)
{
    if (m_spriteSequence == spriteSequence)
        return;

    m_spriteSequence = spriteSequence;
    updateFeatureLevel();
    markNodesDirty();
    Q_EMIT spriteSequenceChanged();
}

void QQuick3DParticleSpriteParticle::setBillboard(bool billboard)
{
    if (m_billboard == billboard)
        return;
    m_billboard = billboard;
    markNodesDirty();
    Q_EMIT billboardChanged();
}

void QQuick3DParticleSpriteParticle::setParticleScale(float scale)
{
    if (qFuzzyCompare(scale, m_particleScale))
        return;
    m_particleScale = scale;
    markNodesDirty();
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
    updateFeatureLevel();
    markNodesDirty();
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

static QSSGRenderParticles::FeatureLevel mapFeatureLevel(QQuick3DParticleSpriteParticle::FeatureLevel level)
{
    switch (level) {
    case QQuick3DParticleSpriteParticle::Simple:
        return QSSGRenderParticles::FeatureLevel::Simple;
    case QQuick3DParticleSpriteParticle::Mapped:
        return QSSGRenderParticles::FeatureLevel::Mapped;
    case QQuick3DParticleSpriteParticle::Animated:
        return QSSGRenderParticles::FeatureLevel::Animated;
    default:
        Q_ASSERT(false);
        return QSSGRenderParticles::FeatureLevel::Simple;
    }
}

QSSGRenderGraphObject *QQuick3DParticleSpriteParticle::ParticleUpdateNode::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (m_spriteParticle) {
        node = m_spriteParticle->updateParticleNode(this, node);
        QQuick3DNode::updateSpatialNode(node);
        m_nodeDirty = false;
    }
    return node;
}

QQuick3DParticleSpriteParticle::PerEmitterData &QQuick3DParticleSpriteParticle::perEmitterData(const ParticleUpdateNode *updateNode)
{
    for (auto &perEmitter : m_perEmitterData) {
        if (perEmitter.particleUpdateNode == updateNode)
            return perEmitter;
    }
    return n_noPerEmitterData;
}

QQuick3DParticleSpriteParticle::PerEmitterData &QQuick3DParticleSpriteParticle::perEmitterData(int emitterIndex)
{
    for (auto &perEmitter : m_perEmitterData) {
        if (perEmitter.emitterIndex == emitterIndex)
            return perEmitter;
    }
    return n_noPerEmitterData;
}

QSSGRenderGraphObject *QQuick3DParticleSpriteParticle::updateParticleNode(const ParticleUpdateNode *updateNode,
                                                                          QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderParticles();
    }

    auto particles = static_cast<QSSGRenderParticles *>(node);

    const auto &perEmitter = perEmitterData(updateNode);
    if (m_featureLevel == QQuick3DParticleSpriteParticle::Animated)
        updateAnimatedParticleBuffer(perEmitter, particles);
    else
        updateParticleBuffer(perEmitter, particles);

    if (!updateNode->m_nodeDirty)
        return particles;

    if (perEmitter.particleCount == 0)
        return particles;

    if (m_sprite)
        particles->m_sprite = m_sprite->getRenderImage();
    else
        particles->m_sprite = nullptr;

    if (m_spriteSequence) {
        particles->m_spriteImageCount = m_spriteSequence->m_frameCount;
        particles->m_blendImages = m_spriteSequence->m_interpolate;
    } else {
        particles->m_spriteImageCount = 1;
        particles->m_blendImages = true;
    }

    particles->m_hasTransparency = hasTransparency();

    if (m_colorTable)
        particles->m_colorTable = m_colorTable->getRenderImage();
    else
        particles->m_colorTable = nullptr;

    particles->m_blendMode = mapBlendMode(m_blendMode);
    particles->m_diffuseColor = color::sRGBToLinear(color());
    particles->m_billboard = m_billboard;
    particles->m_depthBias = perEmitter.emitter->depthBias();
    particles->m_featureLevel = mapFeatureLevel(m_featureLevel);
    particles->m_depthSorting = sortMode() == QQuick3DParticle::SortDistance;

    return particles;
}

void QQuick3DParticleSpriteParticle::handleMaxAmountChanged(int amount)
{
    if (m_particleData.size() == amount)
        return;

    reset();
    m_particleData.resize(amount);
    m_spriteParticleData.resize(amount);
}

void QQuick3DParticleSpriteParticle::handleSystemChanged(QQuick3DParticleSystem *system)
{
    for (PerEmitterData &value : m_perEmitterData) {
        delete value.particleUpdateNode;
        value.particleUpdateNode = new ParticleUpdateNode(system);
        value.particleUpdateNode->m_spriteParticle = this;
    }
}

void QQuick3DParticleSpriteParticle::updateNodes()
{
    for (const PerEmitterData &value : qAsConst(m_perEmitterData))
        value.particleUpdateNode->update();
}

void QQuick3DParticleSpriteParticle::markNodesDirty()
{
    for (const PerEmitterData &value : qAsConst(m_perEmitterData))
        value.particleUpdateNode->m_nodeDirty = true;
}

void QQuick3DParticleSpriteParticle::updateFeatureLevel()
{
    FeatureLevel featureLevel = FeatureLevel::Simple;
    if (m_colorTable)
        featureLevel = FeatureLevel::Mapped;
    if (m_spriteSequence)
        featureLevel = FeatureLevel::Animated;

    if (featureLevel != m_featureLevel)
        m_featureLevel = featureLevel;
}

void QQuick3DParticleSpriteParticle::componentComplete()
{
    if (!system() && qobject_cast<QQuick3DParticleSystem *>(parentItem()))
        setSystem(qobject_cast<QQuick3DParticleSystem *>(parentItem()));

    QQuick3DParticle::componentComplete();
}

void QQuick3DParticleSpriteParticle::reset()
{
    QQuick3DParticle::reset();
    deleteNodes();
    m_nextEmitterIndex = 0;
    m_perEmitterData.clear();
    m_spriteParticleData.fill({});
}

void QQuick3DParticleSpriteParticle::commitParticles()
{
    markAllDirty();
    update();
    updateNodes();
}

int QQuick3DParticleSpriteParticle::nextCurrentIndex(const QQuick3DParticleEmitter *emitter)
{
    if (!m_perEmitterData.contains(emitter)) {
        m_perEmitterData.insert(emitter, PerEmitterData());
        auto &perEmitter = m_perEmitterData[emitter];
        perEmitter.particleUpdateNode = new ParticleUpdateNode(system());
        perEmitter.emitter = emitter;
        perEmitter.particleUpdateNode->m_spriteParticle = this;
        perEmitter.emitterIndex = m_nextEmitterIndex++;
    }
    auto &perEmitter = m_perEmitterData[emitter];
    int index = QQuick3DParticle::nextCurrentIndex(emitter);
    if (m_spriteParticleData[index].emitterIndex != perEmitter.emitterIndex) {
        if (m_spriteParticleData[index].emitterIndex >= 0)
            perEmitterData(m_spriteParticleData[index].emitterIndex).particleCount--;
        perEmitter.particleCount++;
    }
    m_spriteParticleData[index].emitterIndex = perEmitter.emitterIndex;
    return index;
}

void QQuick3DParticleSpriteParticle::setParticleData(int particleIndex,
                     const QVector3D &position,
                     const QVector3D &rotation,
                     const QVector4D &color,
                     float size, float age,
                     float animationFrame)
{
    auto &dst = m_spriteParticleData[particleIndex];
    dst = {position, rotation, color, size, age, animationFrame, dst.emitterIndex};
}

void QQuick3DParticleSpriteParticle::updateParticleBuffer(const PerEmitterData &perEmitter, QSSGRenderGraphObject *spatialNode)
{
    const auto &particles = m_spriteParticleData;
    QSSGRenderParticles *node = static_cast<QSSGRenderParticles *>(spatialNode);
    if (!node)
        return;
    const int particleCount = perEmitter.particleCount;
    if (node->m_particleBuffer.particleCount() != particleCount || m_useAnimatedParticle)
        node->m_particleBuffer.resize(particleCount, sizeof(QSSGParticleSimple));

    m_useAnimatedParticle = false;
    char *dest = node->m_particleBuffer.pointer();
    const SpriteParticleData *src = particles.data();
    const int pps = node->m_particleBuffer.particlesPerSlice();
    const int ss = node->m_particleBuffer.sliceStride();
    const int slices = node->m_particleBuffer.sliceCount();
    const int emitterIndex = perEmitter.emitterIndex;
    int i = 0;
    QSSGBounds3 bounds;
    const auto smode = sortMode();
    if (smode == QQuick3DParticle::SortNewest || smode == QQuick3DParticle::SortOldest) {
        int offset = m_currentIndex;
        int step = (smode == QQuick3DParticle::SortNewest) ? -1 : 1;
        int li = 0;
        const auto sourceIndex = [&](int linearIndex, int offset, int wrap) -> int {
            return (linearIndex + offset + wrap) % wrap;
        };
        for (int s = 0; s < slices; s++) {
            QSSGParticleSimple *dp = reinterpret_cast<QSSGParticleSimple *>(dest);
            for (int p = 0; p < pps && i < particleCount; ) {
                const SpriteParticleData *data = src + sourceIndex(li * step, offset, m_maxAmount);
                if (data->emitterIndex == emitterIndex) {
                    if (data->size > 0.0f)
                        bounds.include(data->position);
                    dp->position = data->position;
                    dp->rotation = data->rotation * float(M_PI / 180.0f);
                    dp->color = data->color;
                    dp->size = data->size * m_particleScale;
                    dp->age = data->age;
                    dp++;
                    p++;
                    i++;
                }
                li++;
            }
            dest += ss;
        }
    } else {
        for (int s = 0; s < slices; s++) {
            QSSGParticleSimple *dp = reinterpret_cast<QSSGParticleSimple *>(dest);
            for (int p = 0; p < pps && i < particleCount; ) {
                if (src->emitterIndex == emitterIndex) {
                    if (src->size > 0.0f)
                        bounds.include(src->position);
                    dp->position = src->position;
                    dp->rotation = src->rotation * float(M_PI / 180.0f);
                    dp->color = src->color;
                    dp->size = src->size * m_particleScale;
                    dp->age = src->age;
                    dp++;
                    p++;
                    i++;
                }
                src++;
            }
            dest += ss;
        }
    }
    node->m_particleBuffer.setBounds(bounds);
}

void QQuick3DParticleSpriteParticle::updateAnimatedParticleBuffer(const PerEmitterData &perEmitter, QSSGRenderGraphObject *spatialNode)
{
    const auto &particles = m_spriteParticleData;
    QSSGRenderParticles *node = static_cast<QSSGRenderParticles *>(spatialNode);
    if (!node)
        return;
    const int particleCount = perEmitter.particleCount;
    if (node->m_particleBuffer.particleCount() != particleCount || !m_useAnimatedParticle)
        node->m_particleBuffer.resize(particleCount, sizeof(QSSGParticleAnimated));

    m_useAnimatedParticle = true;
    char *dest = node->m_particleBuffer.pointer();
    const SpriteParticleData *src = particles.data();
    const int pps = node->m_particleBuffer.particlesPerSlice();
    const int ss = node->m_particleBuffer.sliceStride();
    const int slices = node->m_particleBuffer.sliceCount();
    const int emitterIndex = perEmitter.emitterIndex;
    int i = 0;
    QSSGBounds3 bounds;
    const auto smode = sortMode();
    if (smode == QQuick3DParticle::SortNewest || smode == QQuick3DParticle::SortOldest) {
        int offset = m_currentIndex;
        int step = (smode == QQuick3DParticle::SortNewest) ? -1 : 1;
        int li = 0;
        const auto sourceIndex = [&](int linearIndex, int offset, int wrap) -> int {
            return (linearIndex + offset + wrap) % wrap;
        };
        for (int s = 0; s < slices; s++) {
            QSSGParticleAnimated *dp = reinterpret_cast<QSSGParticleAnimated *>(dest);
            for (int p = 0; p < pps && i < particleCount; ) {
                const SpriteParticleData *data = src + sourceIndex(li * step, offset, m_maxAmount);
                if (data->emitterIndex == emitterIndex) {
                    if (data->size > 0.0f)
                        bounds.include(data->position);
                    dp->position = data->position;
                    dp->rotation = data->rotation * float(M_PI / 180.0f);
                    dp->color = data->color;
                    dp->size = data->size * m_particleScale;
                    dp->age = data->age;
                    dp->animationFrame = data->animationFrame;
                    dp++;
                    p++;
                    i++;
                }
                li++;
            }
            dest += ss;
        }
    } else {
        for (int s = 0; s < slices; s++) {
            QSSGParticleAnimated *dp = reinterpret_cast<QSSGParticleAnimated *>(dest);
            for (int p = 0; p < pps && i < particleCount; ) {
                if (src->emitterIndex == emitterIndex) {
                    if (src->size > 0.0f)
                        bounds.include(src->position);
                    dp->position = src->position;
                    dp->rotation = src->rotation * float(M_PI / 180.0f);
                    dp->color = src->color;
                    dp->size = src->size * m_particleScale;
                    dp->age = src->age;
                    dp->animationFrame = src->animationFrame;
                    dp++;
                    p++;
                    i++;
                }
                src++;
            }
            dest += ss;
        }
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
