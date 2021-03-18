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

#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderFrameBuffer::QSSGRenderFrameBuffer(const QSSGRef<QSSGRenderContext> &context)
    : m_context(context), m_backend(context->backend()), m_bufferHandle(nullptr), m_attachmentBits(0)
{
    m_bufferHandle = m_backend->createRenderTarget();
    Q_ASSERT(m_bufferHandle);
}

QSSGRenderFrameBuffer::~QSSGRenderFrameBuffer()
{
    m_backend->releaseRenderTarget(m_bufferHandle);
    m_bufferHandle = nullptr;
    m_attachmentBits = 0;

    // release attachments
    for (int idx = 0; idx != int(QSSGRenderFrameBufferAttachment::LastAttachment); ++idx) {
        if ((QSSGRenderFrameBufferAttachment)idx != QSSGRenderFrameBufferAttachment::DepthStencil
            || m_context->supportsDepthStencil())
            releaseAttachment((QSSGRenderFrameBufferAttachment)idx);
    }
}

inline void CheckAttachment(const QSSGRef<QSSGRenderContext> &ctx, QSSGRenderFrameBufferAttachment attachment)
{
    (void)ctx;
    (void)attachment;
}

QSSGRenderTextureTargetType QSSGRenderFrameBuffer::releaseAttachment(QSSGRenderFrameBufferAttachment idx)
{
    QSSGRenderTextureTargetType target = QSSGRenderTextureTargetType::Unknown;
    int index = static_cast<int>(idx);

    QSSGRenderTextureOrRenderBuffer attachment = m_attachments[index];
    if (attachment.hasTexture2D()) {
        target = (attachment.texture2D()->isMultisampleTexture()) ? QSSGRenderTextureTargetType::Texture2D_MS
                                                                 : QSSGRenderTextureTargetType::Texture2D;
        // Attach.GetTexture2D()->release();
    } else if (attachment.hasTextureCube()) {
        target = (attachment.textureCube()->isMultisampleTexture()) ? QSSGRenderTextureTargetType::Texture2D_MS
                                                                   : QSSGRenderTextureTargetType::TextureCube;
        // Attach.GetTextureCube()->release();
    } else if (attachment.hasRenderBuffer())
        // Attach.GetRenderBuffer()->release();

        CheckAttachment(m_context, idx);
    m_attachments[index] = QSSGRenderTextureOrRenderBuffer();

    m_attachmentBits &= ~(1 << index);

    return target;
}

QSSGRenderTextureOrRenderBuffer QSSGRenderFrameBuffer::attachment(QSSGRenderFrameBufferAttachment attachment)
{
    if (attachment == QSSGRenderFrameBufferAttachment::Unknown || attachment > QSSGRenderFrameBufferAttachment::LastAttachment) {
        qCCritical(RENDER_INVALID_PARAMETER, "Attachment out of range");
        return QSSGRenderTextureOrRenderBuffer();
    }
    CheckAttachment(m_context, attachment);
    return m_attachments[static_cast<int>(attachment)];
}

void QSSGRenderFrameBuffer::attach(QSSGRenderFrameBufferAttachment attachment,
                                     const QSSGRenderTextureOrRenderBuffer &buffer,
                                     QSSGRenderTextureTargetType target)
{
    if (attachment == QSSGRenderFrameBufferAttachment::Unknown || attachment > QSSGRenderFrameBufferAttachment::LastAttachment) {
        qCCritical(RENDER_INVALID_PARAMETER, "Attachment out of range");
        return;
    }

    const quint32 attachmentBit = (1 << static_cast<int>(attachment));

    // early out
    // if there is nothing to detach
    if (!buffer.hasTexture2D() && !buffer.hasRenderBuffer() && !(m_attachmentBits & attachmentBit))
        return;

    CheckAttachment(m_context, attachment);
    // Ensure we are the bound framebuffer
    m_context->setRenderTarget(this);

    // release previous attachments
    QSSGRenderTextureTargetType theRelTarget = releaseAttachment(attachment);

    if (buffer.hasTexture2D()) {
        // On the same attachment point there could be a something attached with a different
        // target MSAA <--> NoMSAA
        if (theRelTarget != QSSGRenderTextureTargetType::Unknown && theRelTarget != target)
            m_backend->renderTargetAttach(m_bufferHandle, attachment, QSSGRenderBackend::QSSGRenderBackendTextureObject(nullptr), theRelTarget);

        m_backend->renderTargetAttach(m_bufferHandle, attachment, buffer.texture2D()->handle(), target);
        // buffer.GetTexture2D()->addRef();
        m_attachmentBits |= attachmentBit;
    } else if (buffer.hasRenderBuffer()) {
        m_backend->renderTargetAttach(m_bufferHandle, attachment, buffer.renderBuffer()->handle());
        // buffer.GetRenderBuffer()->addRef();
        m_attachmentBits |= attachmentBit;
    } else if (theRelTarget == QSSGRenderTextureTargetType::Unknown) {
        // detach renderbuffer
        m_backend->renderTargetAttach(m_bufferHandle, attachment, QSSGRenderBackend::QSSGRenderBackendRenderbufferObject(nullptr));
    } else {
        // detach texture
        m_backend->renderTargetAttach(m_bufferHandle, attachment, QSSGRenderBackend::QSSGRenderBackendTextureObject(nullptr), theRelTarget);
    }
    m_attachments[static_cast<int>(attachment)] = buffer;
}

void QSSGRenderFrameBuffer::attachFace(QSSGRenderFrameBufferAttachment attachment,
                                         const QSSGRenderTextureOrRenderBuffer &buffer,
                                         QSSGRenderTextureCubeFace face)
{
    if (attachment == QSSGRenderFrameBufferAttachment::Unknown || attachment > QSSGRenderFrameBufferAttachment::LastAttachment) {
        qCCritical(RENDER_INVALID_PARAMETER, "Attachment out of range");
        return;
    }

    if (face == QSSGRenderTextureCubeFace::InvalidFace) {
        Q_ASSERT(false);
        return;
    }

    CheckAttachment(m_context, attachment);
    // Ensure we are the bound framebuffer
    m_context->setRenderTarget(this);

    // release previous attachments
    QSSGRenderTextureTargetType attachTarget = static_cast<QSSGRenderTextureTargetType>(
            (int)QSSGRenderTextureTargetType::TextureCube + (int)face);
    QSSGRenderTextureTargetType theRelTarget = releaseAttachment(attachment);

    // If buffer has no texture cube, this call is used to detach faces.
    // If release target is not cube, there is something else attached to that
    // attachment point, so we want to release that first. E.g (MSAA <--> NoMSAA)
    if (theRelTarget == QSSGRenderTextureTargetType::TextureCube && !buffer.hasTextureCube()) {
        theRelTarget = attachTarget;
        attachTarget = QSSGRenderTextureTargetType::Unknown;
    } else if (theRelTarget == QSSGRenderTextureTargetType::TextureCube) {
        theRelTarget = QSSGRenderTextureTargetType::Unknown;
    }
    if (theRelTarget != QSSGRenderTextureTargetType::Unknown) {
        m_backend->renderTargetAttach(m_bufferHandle, attachment, QSSGRenderBackend::QSSGRenderBackendTextureObject(nullptr), theRelTarget);
    }

    if (attachTarget != QSSGRenderTextureTargetType::Unknown) {
        m_backend->renderTargetAttach(m_bufferHandle, attachment, buffer.textureCube()->handle(), attachTarget);
        // buffer.GetTextureCube()->addRef();
        m_attachmentBits |= (1 << static_cast<int>(attachment));
    }

    m_attachments[static_cast<int>(attachment)] = buffer;
}

bool QSSGRenderFrameBuffer::isComplete()
{
    // Ensure we are the bound framebuffer
    m_context->setRenderTarget(this);

    return m_backend->renderTargetIsValid(m_bufferHandle);
}

QSSGRenderTextureOrRenderBuffer::QSSGRenderTextureOrRenderBuffer(const QSSGRef<QSSGRenderTexture2D> &texture)
    : m_texture2D(texture)
{
}

QSSGRenderTextureOrRenderBuffer::QSSGRenderTextureOrRenderBuffer(const QSSGRef<QSSGRenderRenderBuffer> &render)
    : m_renderBuffer(render)
{
}

QSSGRenderTextureOrRenderBuffer::QSSGRenderTextureOrRenderBuffer(const QSSGRef<QSSGRenderTextureCube> &textureCube)
    : m_textureCube(textureCube)
{
}

QSSGRenderTextureOrRenderBuffer::QSSGRenderTextureOrRenderBuffer() = default;

QSSGRenderTextureOrRenderBuffer::QSSGRenderTextureOrRenderBuffer(const QSSGRenderTextureOrRenderBuffer &other) = default;

QSSGRenderTextureOrRenderBuffer::~QSSGRenderTextureOrRenderBuffer() = default;


QSSGRenderTextureOrRenderBuffer &QSSGRenderTextureOrRenderBuffer::operator=(const QSSGRenderTextureOrRenderBuffer &other)
{
    if (this != &other) {
        m_texture2D = other.m_texture2D;
        m_renderBuffer = other.m_renderBuffer;
        m_textureCube = other.m_textureCube;
    }
    return *this;
}

QSSGRef<QSSGRenderTexture2D> QSSGRenderTextureOrRenderBuffer::texture2D() const
{
    Q_ASSERT(hasTexture2D());
    return m_texture2D;
}

QSSGRef<QSSGRenderTextureCube> QSSGRenderTextureOrRenderBuffer::textureCube() const
{
    Q_ASSERT(hasTextureCube());
    return m_textureCube;
}

QSSGRef<QSSGRenderRenderBuffer> QSSGRenderTextureOrRenderBuffer::renderBuffer() const
{
    Q_ASSERT(hasRenderBuffer());
    return m_renderBuffer;
}

QT_END_NAMESPACE
