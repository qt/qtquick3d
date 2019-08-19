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

#ifndef QSSG_RENDER_OCCLUSION_QUERY_H
#define QSSG_RENDER_OCCLUSION_QUERY_H

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

#include <QtQuick3DRender/private/qssgrenderquerybase_p.h>

QT_BEGIN_NAMESPACE

// forward declaration
class QSSGRenderContext;

class QSSGRenderOcclusionQuery : public QSSGRenderQueryBase
{
    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to context
     * @param[in] fnd			Pointer to foundation
     *
     * @return No return.
     */
    QSSGRenderOcclusionQuery(const QSSGRef<QSSGRenderContext> &context);

public:
    ~QSSGRenderOcclusionQuery() override;

    /**
     * @brief Get query type
     *
     * @return Return query type
     */
    QSSGRenderQueryType queryType() const override { return QSSGRenderQueryType::Samples; }

    /**
     * @brief begin a query
     *
     * @return no return.
     */
    void begin() override;

    /**
     * @brief end a query
     *
     * @return no return.
     */
    void end() override;

    /**
     * @brief Get the result of a query
     *
     * @param[out] params	Contains result of query regarding query type
     *
     * @return no return.
     */
    void result(quint32 *params) override;

    /**
     * @brief query if a result is available
     *
     *
     * @return true if available.
     */
    bool resultAvailable();

    /*
     * @brief static creation function
     *
     * * @return a occlusion query object on success
     */
    static QSSGRef<QSSGRenderOcclusionQuery> create(const QSSGRef<QSSGRenderContext> &context);
};

QT_END_NAMESPACE

#endif
