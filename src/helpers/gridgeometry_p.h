// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

// Workaround for QTBUG-94099, ensures qml_register_types...() is exported
#include "qtquick3dhelpersglobal_p.h"

QT_BEGIN_NAMESPACE

class GridGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(int horizontalLines READ horizontalLines WRITE setHorizontalLines NOTIFY horizontalLinesChanged)
    Q_PROPERTY(int verticalLines READ verticalLines WRITE setVerticalLines NOTIFY verticalLinesChanged)
    Q_PROPERTY(float horizontalStep READ horizontalStep WRITE setHorizontalStep NOTIFY horizontalStepChanged)
    Q_PROPERTY(float verticalStep READ verticalStep WRITE setVerticalStep NOTIFY verticalStepChanged)
    QML_NAMED_ELEMENT(GridGeometry)

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

private:
    void updateData();

    int m_horLines = 1000;
    int m_vertLines = 1000;
    float m_horStep = .1f;
    float m_vertStep = .1f;
};

QT_END_NAMESPACE

#endif
