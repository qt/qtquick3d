/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef GRID_GEOMETRY_H
#define GRID_GEOMETRY_H

#include <QtQuick3D/private/qquick3dgeometry_p.h>

QT_BEGIN_NAMESPACE

class GridGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(int horizontalLines READ horizontalLines WRITE setHorizontalLines NOTIFY horizontalLinesChanged)
    Q_PROPERTY(int verticalLines READ verticalLines WRITE setVerticalLines NOTIFY verticalLinesChanged)
    Q_PROPERTY(float horizontalStep READ horizontalStep WRITE setHorizontalStep NOTIFY horizontalStepChanged)
    Q_PROPERTY(float verticalStep READ verticalStep WRITE setVerticalStep NOTIFY verticalStepChanged)

public:
    GridGeometry();
    ~GridGeometry() override;

    int horizontalLines() const;
    int verticalLines() const;
    float horizontalStep() const;
    float verticalStep() const;

public Q_SLOTS:
    void setHorizontalLines(int count);
    void setVerticalLines(int count);
    void setHorizontalStep(float step);
    void setVerticalStep(float step);

Q_SIGNALS:
    void horizontalLinesChanged();
    void verticalLinesChanged();
    void horizontalStepChanged();
    void verticalStepChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    int m_horLines = 1000;
    int m_vertLines = 1000;
    float m_horStep = .1f;
    float m_vertStep = .1f;
    bool m_dirty = true;
};

QT_END_NAMESPACE

#endif
