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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrenderimagetexture_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <limits>

QT_BEGIN_NAMESPACE

QSSGRenderImage2D::QSSGRenderImage2D(const QSSGRef<QSSGRenderContext> &context,
                                         const QSSGRef<QSSGRenderTexture2D> &inTexture,
                                         QSSGRenderImageAccessType inAccess)
    : m_context(context)
    , m_texture2D(inTexture)
    , m_textureUnit(std::numeric_limits<qint32>::max())
    , m_accessType(inAccess)
    , m_textureLevel(0)
{
}

QSSGRenderImage2D::~QSSGRenderImage2D()
{
}

void QSSGRenderImage2D::setTextureLevel(qint32 inLevel)
{
    if (m_texture2D && m_texture2D->numMipmaps() >= (quint32)inLevel) {
        m_textureLevel = inLevel;
    }
}

void QSSGRenderImage2D::bind(qint32 unit)
{
    if (unit == -1)
        m_textureUnit = m_context->nextTextureUnit();
    else
        m_textureUnit = unit;

    QSSGTextureDetails theDetails(m_texture2D->textureDetails());

    // note it is the callers responsibility that the texture format is supported by the compute
    // shader
    m_context->backend()->bindImageTexture(m_texture2D->handle(),
                                            m_textureUnit,
                                            m_textureLevel,
                                            false,
                                            0,
                                            m_accessType,
                                            theDetails.format);
}

QSSGRenderBackend::QSSGRenderBackendTextureObject QSSGRenderImage2D::handle()
{
    return m_texture2D->handle();
}

QT_END_NAMESPACE
