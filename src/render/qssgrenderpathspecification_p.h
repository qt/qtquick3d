/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSG_RENDER_PATH_SPECIFICATION_H
#define QSSG_RENDER_PATH_SPECIFICATION_H

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

#include <QtGui/QVector2D>
#include <QtQuick3DRender/private/qssgrenderbackend_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderContext;

class QSSGRenderPathSpecification
{
public:
    QAtomicInt ref;

private:
    QSSGRef<QSSGRenderContext> m_context; ///< pointer to context
    QSSGRef<QSSGRenderBackend> m_backend; ///< pointer to backend

public:
    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to render context
     * @param[in] fnd			Pointer to foundation
     *
     * @return No return.
     */
    QSSGRenderPathSpecification(const QSSGRef<QSSGRenderContext> &context);

    /// @QSSGRenderPathSpecification destructor
    virtual ~QSSGRenderPathSpecification();

    /**
     * @brief reset commands and coordiantes
     *
     * @return No return.
     */
    virtual void reset();

    /**
     * @brief add new move to command
     *
     * @param[in] inPoint		Coordinate
     *
     * @return No return.
     */
    virtual void moveTo(QVector2D inPoint);

    /**
     * @brief add new cubic curve command
     *
     * @param[in] inC1		control point 1
     * @param[in] inC2		control point 2
     * @param[in] inDest	final point
     *
     * @return No return.
     */
    virtual void cubicCurveTo(QVector2D inC1, QVector2D inC2, QVector2D inDest);

    /**
     * @brief add new close command
     *
     *
     * @return No return.
     */
    virtual void closePath();

    /**
     * @brief Get path command list
     *
     *
     * @return path commands
     */
    virtual QVector<quint8> getPathCommands() { return m_pathCommands; }

    /**
     * @brief Get path coordinates list
     *
     *
     * @return path coordinates
     */
    virtual QVector<float> getPathCoords() { return m_pathCoords; }

private:
    QVector<quint8> m_pathCommands;
    QVector<float> m_pathCoords;

    /**
     * @brief add a new point to the coordinates
     *
     * @param[in] inPoint		Coordinate
     *
     * @return No return.
     */
    void addPoint(QVector2D inData);

public:
    static QSSGRef<QSSGRenderPathSpecification> createPathSpecification(const QSSGRef<QSSGRenderContext> &context);
};

QT_END_NAMESPACE

#endif
