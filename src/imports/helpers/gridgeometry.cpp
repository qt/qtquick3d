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

#include "gridgeometry_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>

/*!
    \qmltype GridGeometry
    \inqmlmodule QtQuick3D.Helpers
    \inherits Geometry
    \brief Visual helper for showing grid in a scene.

    This helper implements grid geometry, which allows showing a grid in a scene.
*/
/*! \qmlproperty int GridGeometry::horizontalLines
    Specifies the number of horizontal lines in a grid.
*/
/*! \qmlproperty int GridGeometry::verticalLines
    Specifies the number of vertical lines in a grid.
*/
/*! \qmlproperty real GridGeometry::horizontalStep
    Specifies the spacing between horizontal lines.
*/
/*! \qmlproperty real GridGeometry::verticalStep
    Specifies the spacing between vertical lines.
*/

GridGeometry::GridGeometry()
    : QQuick3DGeometry()
{

}

GridGeometry::~GridGeometry()
{

}

int GridGeometry::horizontalLines() const
{
    return m_horLines;
}

int GridGeometry::verticalLines() const
{
    return m_vertLines;
}

float GridGeometry::horizontalStep() const
{
    return m_horStep;
}

float GridGeometry::verticalStep() const
{
    return m_vertStep;
}

void GridGeometry::setHorizontalLines(int count)
{
    count = qMax(count, 1);
    if (m_horLines == count)
        return;
    m_horLines = qMax(count, 1);
    emit horizontalLinesChanged();
    update();
    m_dirty = true;
}

void GridGeometry::setVerticalLines(int count)
{
    count = qMax(count, 1);
    if (m_vertLines == count)
        return;
    m_vertLines = qMax(count, 1);
    emit verticalLinesChanged();
    update();
    m_dirty = true;
}

void GridGeometry::setHorizontalStep(float step)
{
    step = qMax(step, 0.0f);
    if (qFuzzyCompare(m_horStep, step))
        return;
    m_horStep = step;
    emit horizontalStepChanged();
    update();
    m_dirty = true;
}

void GridGeometry::setVerticalStep(float step)
{
    step = qMax(step, 0.0f);
    if (qFuzzyCompare(m_vertStep, step))
        return;
    m_vertStep = step;
    emit verticalStepChanged();
    update();
    m_dirty = true;
}

static void fillVertexData(QByteArray &vertexData, int horLines, float horStep,
                           int vertLines, float vertStep)
{
    const int size = (horLines + vertLines) * sizeof(float) * 8 * 2;
    vertexData.resize(size);
    float *dataPtr = reinterpret_cast<float *>(vertexData.data());

    float y0 = -float(horLines - 1) * horStep * .5f;
    float x0 = -float(vertLines - 1) * vertStep * .5f;
    float y1 = -y0;
    float x1 = -x0;

    for (int i = 0; i < horLines; ++i) {
        // start position
        dataPtr[0] = x0;
        dataPtr[1] = y0 + i * horStep;
        dataPtr[2] = .0f;
        dataPtr[3] = 1.f;

        dataPtr[4] = 0;
        dataPtr[5] = 0;
        dataPtr[6] = 1.f;
        dataPtr[7] = 0;

        dataPtr += 8;

        // end position
        dataPtr[0] = x1;
        dataPtr[1] = y0 + i * horStep;
        dataPtr[2] = .0f;
        dataPtr[3] = 1.f;

        dataPtr[4] = 0;
        dataPtr[5] = 0;
        dataPtr[6] = 1.f;
        dataPtr[7] = 0;

        dataPtr += 8;
    }

    for (int i = 0; i < vertLines; ++i) {
        // start position
        dataPtr[0] = x0 + i * vertStep;
        dataPtr[1] = y0;
        dataPtr[2] = .0f;
        dataPtr[3] = 1.f;

        dataPtr[4] = 0;
        dataPtr[5] = 0;
        dataPtr[6] = 1.f;
        dataPtr[7] = 0;

        dataPtr += 8;

        // end position
        dataPtr[0] = x0 + i * vertStep;
        dataPtr[1] = y1;
        dataPtr[2] = .0f;
        dataPtr[3] = 1.f;

        dataPtr[4] = 0;
        dataPtr[5] = 0;
        dataPtr[6] = 1.f;
        dataPtr[7] = 0;

        dataPtr += 8;
    }
}

QSSGRenderGraphObject *GridGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (m_dirty) {
        QByteArray vertexData;
        fillVertexData(vertexData, m_horLines, m_horStep, m_vertLines, m_vertStep);
        clear();
        addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                     QQuick3DGeometry::Attribute::ComponentType::F32Type);
        addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 16,
                     QQuick3DGeometry::Attribute::ComponentType::F32Type);
        setStride(32);
        setVertexData(vertexData);
        setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);
        setBounds(QVector3D(-m_vertLines/2, -m_horLines/2, 0.0) * m_horStep,
                            QVector3D(m_vertLines/2, m_horLines/2, 0.0) * m_vertStep);
    }
    node = QQuick3DGeometry::updateSpatialNode(node);
    return node;
}

