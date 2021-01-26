/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSG_RENDER_VERTEX_BUFFER_H
#define QSSG_RENDER_VERTEX_BUFFER_H

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

#include <QtQuick3DRender/private/qssgrenderdatabuffer_p.h>

QT_BEGIN_NAMESPACE

// forward declaration
class QSSGRenderContext;

///< Vertex buffer representation
class Q_QUICK3DRENDER_EXPORT QSSGRenderVertexBuffer : public QSSGRenderDataBuffer
{
public:
    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to context
     * @param[in] entries		Vertex buffer attribute layout entries
     * @param[in] size			Size of the buffer
     * @param[in] bindFlags		Where to binf this buffer (e.g. vertex, index, ...)
     *							For OpenGL this should be a single
     *value
     * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
     * @param[in] data			A pointer to the buffer data that is allocated by the
     *application.
     *
     * @return No return.
     */
    QSSGRenderVertexBuffer(const QSSGRef<QSSGRenderContext> &context,
                             QSSGRenderBufferUsageType usageType,
                             quint32 stride,
                             QSSGByteView data);

    ///< destructor
    virtual ~QSSGRenderVertexBuffer() override;

    /**
     * @brief return vertex data stride
     *
     * @return data stride.
     */
    quint32 stride() const { return m_stride; }

    /**
     * @brief get vertex count
     *
     * @return vertex count
     */
    quint32 numVertexes() const
    {
        Q_ASSERT((m_bufferCapacity % m_stride) == 0);
        return m_bufferCapacity / m_stride;
    }

    /**
     * @brief bind the buffer bypasses the context state
     *
     * @return no return.
     */
    void bind() override;

private:
    quint32 m_stride;
};

QT_END_NAMESPACE

#endif
