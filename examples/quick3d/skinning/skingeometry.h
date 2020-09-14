/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SKINGEOMETRY_H
#define SKINGEOMETRY_H

#include <QtQuick3D/qquick3d.h>
#include <QtQuick3D/qquick3dgeometry.h>

#include <QVector3D>
#include <QtCore/QList>

class SkinGeometry : public QQuick3DGeometry
{
    //! [geometry]
    Q_OBJECT
    QML_NAMED_ELEMENT(SkinGeometry)
    Q_PROPERTY(QList<QVector3D> positions READ positions WRITE setPositions NOTIFY positionsChanged)
    Q_PROPERTY(QList<qint32> joints READ joints WRITE setJoints NOTIFY jointsChanged)
    Q_PROPERTY(QList<float> weights READ weights WRITE setWeights NOTIFY weightsChanged)
    Q_PROPERTY(QList<quint32> indexes READ indexes WRITE setIndexes NOTIFY indexesChanged)
    //! [geometry]

public:
    SkinGeometry(QQuick3DObject *parent = nullptr);

    QList<QVector3D> positions() const;
    QList<qint32> joints() const;
    QList<float> weights() const;
    QList<quint32> indexes() const;

public Q_SLOTS:
    void setPositions(const QList<QVector3D> &positions);
    void setJoints(const QList<qint32> &joints);
    void setWeights(const QList<float> &weights);
    void setIndexes(const QList<quint32> &indexes);

Q_SIGNALS:
    void positionsChanged();
    void jointsChanged();
    void weightsChanged();
    void indexesChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    QList<QVector3D> m_positions;
    QList<qint32> m_joints;
    QList<float> m_weights;
    QList<quint32> m_indexes;

    bool m_vertexDirty = false;
    bool m_indexDirty = false;

    QByteArray m_vertexBuffer;
    QByteArray m_indexBuffer;
};

#endif // SKINGEOMETRY_H
