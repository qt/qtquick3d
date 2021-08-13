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

#ifndef SHAPEMANAGER_H
#define SHAPEMANAGER_H

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

#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtGui/qimage.h>
#include <QtGui/qvector3d.h>

class ShapeManager : public QObject
{
    Q_OBJECT
public:
    explicit ShapeManager(QObject *parent = nullptr);

    enum class SortingMode : quint8
    {
        Random,
        DistanceClosestFirst,
        DistanceClosestLast
    };

    void setImage(const QString &filename);
    void setDepth(float depth);
    void setAmount(int amount);
    void setScale(float scale);
    void setSortingMode(SortingMode mode);
    void setSortingPosition(const QVector3D &position);

    bool loadImage();
    bool generateData();
    bool saveShapeData(const QString &filename);
    void dumpOutput();

private:
    QString m_imageFilename;
    QImage m_image;
    float m_depth = 0.0f;
    int m_amount = -1;
    QString m_cborFilename;
    QList<QVector3D> m_data;
    QList<QVector3D> m_outputData;
    float m_scale = 1.0f;
    SortingMode m_sortingMode = SortingMode::Random;
    QVector3D m_sortingPosition;
};

#endif // SHAPEMANAGER_H
