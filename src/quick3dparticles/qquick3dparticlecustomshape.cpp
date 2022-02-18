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

#include "qquick3dparticlecustomshape_p.h"
#include "qquick3dparticlerandomizer_p.h"
#include "qquick3dparticlesystem_p.h"
#include "qquick3dparticleutils_p.h"
#include "qquick3dparticleshapedatautils_p.h"
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlfile.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleCustomShape3D
    \inherits ParticleAbtractShape3D
    \inqmlmodule QtQuick3D.Particles3D
    \brief Loads custom particle shapes for emitters and affectors.
    \since 6.3

    The ParticleCustomShape3D element can be used to load custom particle shapes.

    For example, to emit particles from positions defined in heart.cbor:

    \qml
    ParticleEmitter3D {
        shape: ParticleCustomShape3D {
            source: "heart.cbor"
        }
        ...
    }
    \endqml

    The format of CBOR shape files is following:
    \badcode
    [
      "QQ3D_SHAPE", // string
      version, // integer
      [
        posX, // float
        posY, // float
        posZ, // float
        posX, // float
        ...
      ]
    ]
    \endcode

    To assist in generating these shape files you can use the shapegen tool.
*/

QQuick3DParticleCustomShape::QQuick3DParticleCustomShape(QObject *parent)
    : QQuick3DParticleAbstractShape(parent)
{
}

/*!
    \qmlproperty url ParticleCustomShape3D::source

    This property holds the location of the shape file.
*/

QUrl QQuick3DParticleCustomShape::source() const
{
    return m_source;
}

/*!
    \qmlproperty bool ParticleCustomShape3D::randomizeData

    This property holds whether the particles are used in random order instead
    of in the order they are specified in the source.

    The default value is \c false.
*/
bool QQuick3DParticleCustomShape::randomizeData() const
{
    return m_random;
}

void QQuick3DParticleCustomShape::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    m_source = source;

    loadFromSource();
    Q_EMIT sourceChanged();
}

void QQuick3DParticleCustomShape::setRandomizeData(bool random)
{
    if (m_random == random)
        return;

    m_random = random;
    if (m_random)
        m_randomizeDirty = true;
    Q_EMIT randomizeDataChanged();
}

void QQuick3DParticleCustomShape::loadFromSource()
{
    m_positions.clear();

    // Get path to file
    const QQmlContext *context = qmlContext(this);
    QString dataFilePath = QQmlFile::urlToLocalFileOrQrc(context ? context->resolvedUrl(m_source) : m_source);

    QFile dataFile(dataFilePath);
    if (!dataFile.open(QIODevice::ReadOnly)) {
        // Invalid file
        qWarning() << "Unable to open file:" << dataFilePath;
        return;
    }
    QCborStreamReader reader(&dataFile);

    // Check that file is proper CBOR and get the version
    int version = QQuick3DParticleShapeDataUtils::readShapeHeader(reader);

    if (version == -1) {
        // Invalid file
        qWarning() << "Invalid shape data version:" << version;
        return;
    }

    // Start positions array
    reader.enterContainer();

    while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
        QVector3D pos = QQuick3DParticleShapeDataUtils::readValue(reader, QMetaType::QVector3D).value<QVector3D>();
        m_positions.append(pos);
    }

    // Leave positions array
    reader.leaveContainer();

    // Leave root array
    reader.leaveContainer();

    if (m_random)
        m_randomizeDirty = true;
}

void QQuick3DParticleCustomShape::doRandomizeData()
{
    if (!m_system || m_positions.isEmpty())
        return;

    auto rand = m_system->rand();
    int seed = rand->get(0, QPRand::Shape1) * float(INT_MAX);
    std::shuffle(m_positions.begin(), m_positions.end(), std::default_random_engine(seed));

    m_randomizeDirty = false;
}

QVector3D QQuick3DParticleCustomShape::getPosition(int particleIndex)
{
    if (!m_parentNode || m_positions.isEmpty())
        return QVector3D();

    if (m_randomizeDirty)
        doRandomizeData();

    int index = particleIndex % m_positions.size();
    return m_positions.at(index) * m_parentNode->scale();
}

QT_END_NAMESPACE
