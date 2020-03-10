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

#include "renderexampletools.h"

#include <QtQuick3DRender/private/qssgrenderindexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrendervertexbuffer_p.h>
#include <QtQuick3DRender/private/qssgrenderattriblayout_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <QtGui/QVector3D>


struct BoxFace
{
    QVector3D positions[4];
    QVector3D normal;
};

static const BoxFace g_BoxFaces[] = {
    { // Z+
      QVector3D(-1, -1, 1), QVector3D(-1, 1, 1), QVector3D(1, 1, 1), QVector3D(1, -1, 1), QVector3D(0, 0, 1) },
    { // X+
      QVector3D(1, -1, 1), QVector3D(1, 1, 1), QVector3D(1, 1, -1), QVector3D(1, -1, -1), QVector3D(1, 0, 0) },
    { // Z-
      QVector3D(1, -1, -1), QVector3D(1, 1, -1), QVector3D(-1, 1, -1), QVector3D(-1, -1, -1),
      QVector3D(0, 0, -1) },
    { // X-
      QVector3D(-1, -1, -1), QVector3D(-1, 1, -1), QVector3D(-1, 1, 1), QVector3D(-1, -1, 1),
      QVector3D(-1, 0, 0) },
    { // Y+
      QVector3D(-1, 1, 1), QVector3D(-1, 1, -1), QVector3D(1, 1, -1), QVector3D(1, 1, 1), QVector3D(0, 1, 0) },
    { // Y-
      QVector3D(-1, -1, -1), QVector3D(-1, -1, 1), QVector3D(1, -1, 1), QVector3D(1, -1, -1), QVector3D(0, -1, 0) }
};

static const QVector2D g_BoxUVs[] = {
    QVector2D(0, 1), QVector2D(0, 0), QVector2D(1, 0), QVector2D(1, 1),
};

QSSGRef<QSSGRenderInputAssembler> QSSGRenderExampleTools::createBox(QSSGRef<QSSGRenderContext> context,
                                                                               QSSGRef<QSSGRenderVertexBuffer> &outVertexBuffer,
                                                                               QSSGRef<QSSGRenderIndexBuffer> &outIndexBuffer)
{
    const quint32 numVerts = 24;
    const quint32 numIndices = 36;
    QVector3D extents = QVector3D(1, 1, 1);

    // Attribute Layouts
    QSSGRenderVertexBufferEntry entries[] = {
        QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3, 0),
        QSSGRenderVertexBufferEntry("attr_norm", QSSGRenderComponentType::Float32, 3, 3 * sizeof(float)),
        QSSGRenderVertexBufferEntry("attr_uv", QSSGRenderComponentType::Float32, 2, 6 * sizeof(float)),
    };

    QSSGRef<QSSGRenderAttribLayout> attribLayout = context->createAttributeLayout(toDataView(entries, 3));

    // Vertex Buffer
    struct Vertex {
        QVector3D position;
        QVector3D normal;
        QVector2D uv;
    };
    Q_STATIC_ASSERT(sizeof(Vertex) == 8 * sizeof(float));

    quint32 bufStride = sizeof(Vertex);
    QVector<Vertex> vertices(numVerts);

    Vertex *v =vertices.begin();
    for (quint32 i = 0; i < 6; i++) {
        const BoxFace &bf = g_BoxFaces[i];
        for (quint32 j = 0; j < 4; j++) {
            v->position = bf.positions[j] * extents;
            v->normal = bf.normal;
            v->uv = g_BoxUVs[j];
            ++v;
        }
    }

    auto vertexDataRef = toByteView(vertices);
    outVertexBuffer= new QSSGRenderVertexBuffer(context, QSSGRenderBufferUsageType::Static, bufStride, vertexDataRef);
    Q_ASSERT(bufStride == outVertexBuffer->stride());

    // Index Buffer
    QVector<quint16> indexBuffer(numIndices);
    quint16 *indices = indexBuffer.begin();
    for (quint8 i = 0; i < 6; i++) {
        const quint16 base = i * 4;
        *(indices++) = base + 0;
        *(indices++) = base + 1;
        *(indices++) = base + 2;
        *(indices++) = base + 0;
        *(indices++) = base + 2;
        *(indices++) = base + 3;
    }
    auto indexDataRef = toByteView(indexBuffer);
    outIndexBuffer= new QSSGRenderIndexBuffer(context, QSSGRenderBufferUsageType::Static,
                                               QSSGRenderComponentType::UnsignedInteger16,
                                               indexDataRef);

    quint32 strides = outVertexBuffer->stride();
    quint32 offsets = 0;

    QSSGRef<QSSGRenderInputAssembler> inputAssembler = context->createInputAssembler(attribLayout,
                                                                                              toDataView(&outVertexBuffer, 1),
                                                                                              outIndexBuffer,
                                                                                              toDataView(&strides, 1),
                                                                                              toDataView(&offsets, 1));
    return inputAssembler;
}


namespace {

static void dumpShaderOutput(const QSSGRef<QSSGRenderContext> &ctx, const QSSGRenderVertFragCompilationResult &compResult)
{
    Q_UNUSED(ctx)
    Q_UNUSED(compResult)
    //    if (!isTrivial(compResult.mFragCompilationOutput)) {
    //        qWarning("Frag output:\n%s", compResult.mFragCompilationOutput);
    //    }
    //    if (!isTrivial(compResult.mVertCompilationOutput)) {
    //        qWarning("Vert output:\n%s", compResult.mVertCompilationOutput);
    //    }
    //    if (!isTrivial(compResult.mLinkOutput)) {
    //        qWarning("Link output:\n%s", compResult.mLinkOutput);
    //    }
}

QSSGRef<QSSGRenderShaderProgram> compileAndDump(const QSSGRef<QSSGRenderContext> &ctx, const char *name, const char *vertShader, const char *fragShader)
{
    QSSGRenderVertFragCompilationResult compResult =
            ctx->compileSource(name, toByteView(vertShader), toByteView(fragShader));
    dumpShaderOutput(ctx, compResult);
    return compResult.m_shader;
}
}

QSSGRef<QSSGRenderShaderProgram> QSSGRenderExampleTools::createSimpleShader(QSSGRef<QSSGRenderContext> ctx)
{
    return compileAndDump(ctx, "SimpleShader", getSimpleVertShader(), getSimpleFragShader());
}

QSSGRef<QSSGRenderShaderProgram> QSSGRenderExampleTools::createSimpleShaderTex(QSSGRef<QSSGRenderContext> ctx)
{
    return compileAndDump(ctx, "SimpleShader", getSimpleVertShader(), getSimpleFragShaderTex());
}
