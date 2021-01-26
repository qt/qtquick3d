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

#ifndef QSSG_RENDER_TEXTURE_CUBE_H
#define QSSG_RENDER_TEXTURE_CUBE_H

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

#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrenderbackend_p.h>
#include <QtQuick3DRender/private/qssgrendertexturebase_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderContext;

class Q_QUICK3DRENDER_EXPORT QSSGRenderTextureCube : public QSSGRenderTextureBase
{
    Q_DISABLE_COPY(QSSGRenderTextureCube)
private:
    quint32 m_width; ///< texture width (per face)
    quint32 m_height; ///< texture height (per face)

public:
    /**
     * @brief constructor
     *
     * @param[in] context		Pointer to context
     * @param[in] fnd			Pointer to foundation
     * @param[in] texTarget		Texture target
     *
     * @return No return.
     */
    QSSGRenderTextureCube(const QSSGRef<QSSGRenderContext> &context);

    virtual ~QSSGRenderTextureCube() override;

    /**
     * @brief constructor
     *
     * @param[in] newBuffer		Pointer to pixel buffer
     * @param[in] inMipLevel	Pointer to foundation
     * @param[in] width			Texture target
     * @param[in] height		Texture target
     * @param[in] slices		Texture target
     * @param[in] format		Texture target
     *
     * @return No return.
     */
    void setTextureData(QSSGByteView newBuffer,
                        quint8 inMipLevel,
                        QSSGRenderTextureCubeFace inFace,
                        quint32 width,
                        quint32 height,
                        QSSGRenderTextureFormat format);

    // Get the texture details for mipmap level 0 if it was set.
    QSSGTextureDetails textureDetails() const override;

    /**
     * @brief Bind a texture for shader access
     *
     *
     * @return No return.
     */
    void bind() override;
};

QT_END_NAMESPACE

#endif
