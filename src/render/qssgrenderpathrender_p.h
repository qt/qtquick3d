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

#ifndef QSSG_RENDER_PATH_RENDER_H
#define QSSG_RENDER_PATH_RENDER_H

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

#include <QtQuick3DRender/private/qssgrenderbackend_p.h>
#include <QtQuick3DUtils/private/qssgbounds3_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderContext;
class QSSGRenderPathSpecification;
class QSSGRenderPathFontSpecification;

///< A program pipeline is a collection of a multiple programs (vertex, fragment, geometry,....)
class Q_QUICK3DRENDER_EXPORT QSSGRenderPathRender
{
    Q_DISABLE_COPY(QSSGRenderPathRender)
public:
    QAtomicInt ref;

protected:
    QSSGRef<QSSGRenderContext> m_context; ///< pointer to context
    QSSGRef<QSSGRenderBackend> m_backend; ///< pointer to backend

public:
    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to render context
     * @param[in] fnd			Pointer to foundation
     * @param[in] range		Number of internal objects
     *
     * @return No return.
     */
    QSSGRenderPathRender(const QSSGRef<QSSGRenderContext> &context, size_t range);

    /// @brief destructor
    ~QSSGRenderPathRender();

    /**
     * @brief get the backend object handle
     *
     * @return the backend object handle.
     */
    QSSGRenderBackend::QSSGRenderBackendPathObject getPathHandle() { return m_pathRenderHandle; }

    // The render context can create a path specification object.
    void setPathSpecification(const QSSGRef<QSSGRenderPathSpecification> &inCommandBuffer);

    QSSGBounds3 getPathObjectBoundingBox();
    QSSGBounds3 getPathObjectFillBox();
    QSSGBounds3 getPathObjectStrokeBox();

    void setStrokeWidth(float inStrokeWidth);
    float getStrokeWidth() const;

    void stencilStroke();
    void stencilFill();

    /**
     * @brief static create function
     *
     * @param[in] context		Pointer to render context
     * @param[in] range			Number of internal objects
     *
     * @return the backend object handle.
     */
    static QSSGRef<QSSGRenderPathRender> create(const QSSGRef<QSSGRenderContext> &context, size_t range);

private:
    QSSGRenderBackend::QSSGRenderBackendPathObject m_pathRenderHandle; ///< opaque backend handle
    size_t m_range; ///< range of internal objects
    float m_strokeWidth;
};

QT_END_NAMESPACE

#endif
