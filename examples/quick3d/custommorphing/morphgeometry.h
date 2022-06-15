// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MORPHGEOMETRY_H
#define MORPHGEOMETRY_H

#include <QtQuick3D/qquick3d.h>
#include <QtQuick3D/qquick3dgeometry.h>

#include <QVector3D>
#include <QtCore/QList>

//! [class definition]
class MorphGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MorphGeometry)
    Q_PROPERTY(int gridSize READ gridSize WRITE setGridSize NOTIFY gridSizeChanged)

public:
    MorphGeometry(QQuick3DObject *parent = nullptr);

    int gridSize() { return m_gridSize; }
    void setGridSize(int gridSize);

signals:
    void gridSizeChanged();

private:
    void calculateGeometry();
    void updateData();

    QList<QVector3D> m_positions;
    QList<QVector3D> m_normals;

    QList<QVector3D> m_targetPositions;
    QList<QVector3D> m_targetNormals;

    QList<quint32> m_indexes;

    QByteArray m_vertexBuffer;
    QByteArray m_indexBuffer;

    int m_gridSize = 50;
    QVector3D boundsMin;
    QVector3D boundsMax;
};
//! [class definition]

#endif // MORPHGEOMETRY_H
