/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "../shared/renderexample.h"
#include "../shared/renderexampletools.h"
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrendertexture2d_p.h>
#include <QtQuick3DRender/private/qssgrenderindexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendervertexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderrenderbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtGui/QVector4D>
#include <QtGui/QGuiApplication>
#include <QtQuick3D/qquick3d.h>

struct ShaderArgs
{
    QMatrix4x4 mvp;
    QSSGRef<QSSGRenderTexture2D> texture;
    QSSGRef<QSSGRenderShaderProgram> shader;
    ShaderArgs() {}
};
class RenderToTexture : public QSSGRenderExample
{
    QSSGRef<QSSGRenderContext> m_Context;
    QSSGRef<QSSGRenderVertexBuffer> mVertexBuffer;
    QSSGRef<QSSGRenderIndexBuffer> mIndexBuffer;
    QSSGRef<QSSGRenderInputAssembler> mInputAssembler;
    // Simple shader
    QSSGRef<QSSGRenderShaderProgram> mSimpleShader;
    // Simple shader with texture lookup.
    QSSGRef<QSSGRenderShaderProgram> mSimpleShaderTex;

    QSSGRef<QSSGRenderFrameBuffer> mFrameBuffer;
    QSSGRef<QSSGRenderTexture2D> mColorBuffer;
    QSSGRef<QSSGRenderTexture2D> mDepthBuffer;

    quint32 mFBWidth{400};
    quint32 mFBHeight{400};

    bool m_viewportDirty = true;

    ShaderArgs mShaderArgs;
    QMatrix4x4 frus;
    QMatrix4x4 model;
    QMatrix4x4 rot;
    qint64 m_elapsedTime = 0;

public:
    RenderToTexture() = default;
    void setupMVP(const QVector3D &translation)
    {
        mShaderArgs.mvp = frus;
        mShaderArgs.mvp *= model;
        mShaderArgs.mvp.translate(translation);
        mShaderArgs.mvp *= rot;
    }
    void DrawIndexedArrays(const QVector3D &translation)
    {
        setupMVP(translation);
        m_Context->setActiveShader(mShaderArgs.shader);
        mShaderArgs.shader->setPropertyValue("mat_mvp", mShaderArgs.mvp);
        mShaderArgs.shader->setPropertyValue("image0", mShaderArgs.texture.data());
        m_Context->draw(QSSGRenderDrawMode::Triangles, mIndexBuffer->numIndices(), 0);
    }

    // QSSGRenderExample interface
public:
    void initialize() override
    {
        m_Context = QSSGRenderContext::createGl(format());
        mInputAssembler = QSSGRenderExampleTools::createBox(m_Context, mVertexBuffer, mIndexBuffer);
        mSimpleShader = QSSGRenderExampleTools::createSimpleShader(m_Context);
        mSimpleShaderTex = QSSGRenderExampleTools::createSimpleShaderTex(m_Context);
        // If you don't want the depth buffer information back out of the system, then you can
        // do this.
        // mDepthBuffer = m_Context.CreateRenderBuffer( QSSGRenderRenderBufferFormats::Depth16,
        // mFBWidth, mFBHeight );

        m_Context->setInputAssembler(mInputAssembler);

        mDepthBuffer = new QSSGRenderTexture2D(m_Context);
        mDepthBuffer->setTextureData(QSSGByteView(), 0, mFBWidth, mFBHeight,
                                     QSSGRenderTextureFormat::Depth16);
        mColorBuffer = new QSSGRenderTexture2D(m_Context);
        mColorBuffer->setTextureData(QSSGByteView(), 0, mFBWidth, mFBHeight,
                                     QSSGRenderTextureFormat::RGBA8);
        if (mDepthBuffer && mColorBuffer) {
            // Creating objects tends to Bind them to their active state hooks.
            // So to protect the rest of the system against what they are doing (if we care), we
            // need
            // to push the current state
            // Auto-binds the framebuffer.
            mFrameBuffer = new QSSGRenderFrameBuffer(m_Context);
            mFrameBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, mColorBuffer);
            mFrameBuffer->attach(QSSGRenderFrameBufferAttachment::Depth, mDepthBuffer);
            Q_ASSERT(mFrameBuffer->isComplete());

            m_Context->setRenderTarget(nullptr);
        }
        mColorBuffer->setMinFilter(QSSGRenderTextureMinifyingOp::Linear);
        mColorBuffer->setMagFilter(QSSGRenderTextureMagnifyingOp::Linear);
        m_Context->setDepthTestEnabled(true);
        m_Context->setDepthWriteEnabled(true);
        m_Context->setClearColor(QVector4D(.3f, .3f, .3f, 1.f));
        // Setup various matrici
        frus.frustum(-1, 1, -1, 1, 1, 10);
        model.translate(0, 0, -4);
        mShaderArgs.texture = mColorBuffer;
    }
    void drawFrame(qint64 delta) override {
        m_elapsedTime += delta;
        rot = QMatrix4x4();
        rot.rotate((float)m_elapsedTime * 0.1f, .707f, .707f, 0);
        QSSGRenderClearFlags clearFlags(QSSGRenderClearValues::Color | QSSGRenderClearValues::Depth);
        // render to frame buffer
        {
            QSSGRenderContextScopedProperty<const QSSGRef<QSSGRenderFrameBuffer> &> framebuffer(*m_Context.data(),
                                                                                                &QSSGRenderContext::renderTarget,
                                                                                                &QSSGRenderContext::setRenderTarget,
                                                                                                mFrameBuffer);

            QSSGRenderContextScopedProperty<QRect> viewport(
                *m_Context.data(), &QSSGRenderContext::viewport, &QSSGRenderContext::setViewport,
                QRect(0, 0, mFBWidth, mFBHeight));
            QSSGRenderContextScopedProperty<QVector4D> clearColor(
                *m_Context.data(), &QSSGRenderContext::clearColor, &QSSGRenderContext::setClearColor,
                QVector4D(1.0f, .6f, .6f, 1.6f));
            m_Context->clear(clearFlags);
            mShaderArgs.shader = mSimpleShader;
            DrawIndexedArrays(QVector3D());
        }
        if (m_viewportDirty) {
            m_Context->setViewport(QRect(0, 0, this->width(), this->height()));
            m_viewportDirty = false;
        }

        m_Context->clear(clearFlags);
        mShaderArgs.texture = mColorBuffer;
        mShaderArgs.shader = mSimpleShaderTex;

        DrawIndexedArrays(QVector3D(-2.f, 0.f, 0.f));

        mShaderArgs.texture = mDepthBuffer;
        DrawIndexedArrays(QVector3D(2.f, 0.f, 0.f));
    }

protected:
    void resizeEvent(QResizeEvent *) override
    {
        m_viewportDirty = true;
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QSurfaceFormat::setDefaultFormat(QQuick3D::idealSurfaceFormat());

    RenderToTexture renderToTexture;
    renderToTexture.show();

    return app.exec();
}
