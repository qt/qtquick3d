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

#ifndef QSSG_RENDER_INDEX_BUFFER_H
#define QSSG_RENDER_INDEX_BUFFER_H

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

class Q_QUICK3DRENDER_EXPORT QSSGRenderIndexBuffer : public QSSGRenderDataBuffer
{
public:
    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to context
     * @param[in] size			Size of the buffer
     * @param[in] componentType	Size of the buffer
     * @param[in] usage			Usage of the buffer (e.g. static, dynamic...)
     * @param[in] data			A pointer to the buffer data that is allocated by the
     * application.
     *
     * @return No return.
     */
    QSSGRenderIndexBuffer(const QSSGRef<QSSGRenderContext> &context,
                            QSSGRenderBufferUsageType usageType,
                            QSSGRenderComponentType componentType,
                            QSSGByteView data);

    ///< destruvtor
    ~QSSGRenderIndexBuffer() override;

    /**
     * @brief get the component type (quint8, quint16)
     *
     * @return the component type
     */
    QSSGRenderComponentType componentType() const { return m_componentType; }

    /**
     * @brief get the index count
     *
     * @return actual index count
     */
    quint32 numIndices() const;

    /**
     * @brief bind the buffer bypasses the context state
     *
     * @return no return.
     */
    void bind() override;

    /**
     * @brief draw the buffer
     *
     * @param[in] drawMode		draw mode (e.g Triangles...)
     * @param[in] count			vertex count
     * @param[in] offset		start offset in byte
     *
     * @return no return.
     */
    void draw(QSSGRenderDrawMode drawMode, quint32 count, quint32 offset);

private:
    QSSGRenderComponentType m_componentType; ///< component type (quint8, quint16)
};

QT_END_NAMESPACE

#endif
