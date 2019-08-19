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

#include "qssgrenderpixelgraphicsrenderer_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderpixelgraphicstypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

QT_BEGIN_NAMESPACE

namespace {

struct QSSGPGRectShader
{
    QSSGRef<QSSGRenderShaderProgram> rectShader;
    QSSGRef<QSSGRenderShaderConstantBase> mvp;
    QSSGRef<QSSGRenderShaderConstantBase> rectColor;
    QSSGRef<QSSGRenderShaderConstantBase> leftright;
    QSSGRef<QSSGRenderShaderConstantBase> bottomtop;

    QSSGPGRectShader() = default;
    void setShader(const QSSGRef<QSSGRenderShaderProgram> &program)
    {
        rectShader = program;
        if (program) {
            mvp = program->shaderConstant("model_view_projection");
            rectColor = program->shaderConstant("rect_color");
            leftright = program->shaderConstant("leftright[0]");
            bottomtop = program->shaderConstant("bottomtop[0]");
        }
    }

    void apply(QMatrix4x4 &inVP, const QSSGPGRect &inObject)
    {
        if (mvp)
            rectShader->setConstantValue(mvp.data(), toDataView(inVP), 1);
        if (rectColor)
            rectShader->setConstantValue(rectColor.data(), inObject.fillColor, 1);
        if (leftright) {
            float theData[] = { inObject.left, inObject.right };
            rectShader->setConstantValue(leftright.data(), *theData, 2);
        }
        if (bottomtop) {
            float theData[] = { inObject.bottom, inObject.top };
            rectShader->setConstantValue(bottomtop.data(), *theData, 2);
        }
    }

    operator bool() { return rectShader != nullptr; }
};

struct QSSGPixelGraphicsRenderer : public QSSGPixelGraphicsRendererInterface
{
    QSSGRenderContextInterface *m_renderContext;
    QSSGRef<QSSGRenderVertexBuffer> m_quadVertexBuffer;
    QSSGRef<QSSGRenderIndexBuffer> m_quadIndexBuffer;
    QSSGRef<QSSGRenderInputAssembler> m_quadInputAssembler;
    QSSGRef<QSSGRenderAttribLayout> m_quadAttribLayout;
    QSSGShaderVertexCodeGenerator m_vertexGenerator;
    QSSGShaderFragmentCodeGenerator m_fragmentGenerator;
    QSSGPGRectShader m_rectShader;

    QSSGPixelGraphicsRenderer(QSSGRenderContextInterface *ctx)
        : m_renderContext(ctx)
        , m_vertexGenerator(m_renderContext->renderContext()->renderContextType())
        , m_fragmentGenerator(m_vertexGenerator, m_renderContext->renderContext()->renderContextType())
    {
    }

    void getRectShaderProgram()
    {
        if (!m_rectShader) {
            m_vertexGenerator.begin();
            m_fragmentGenerator.begin();
            m_vertexGenerator.addAttribute("attr_pos", "vec2");
            m_vertexGenerator.addUniform("model_view_projection", "mat4");
            m_vertexGenerator.addUniform("leftright[2]", "float");
            m_vertexGenerator.addUniform("bottomtop[2]", "float");
            m_fragmentGenerator.addVarying("rect_uvs", "vec2");
            m_fragmentGenerator.addUniform("rect_color", "vec4");
            m_vertexGenerator << "void main() {\n"
                              << "    gl_Position = model_view_projection * vec4(\n"
                                 "        leftright[int(attr_pos.x)], bottomtop[int(attr_pos.y)], 0.0, 1.0);\n"
                                 "    rect_uvs = attr_pos;\n"
                                 "}\n";

            m_fragmentGenerator << "void main() {\n"
                                   "    fragOutput = rect_color;"
                                   "}\n";

            m_vertexGenerator.buildShaderSource();
            m_fragmentGenerator.buildShaderSource();

            m_rectShader.setShader(
                    m_renderContext->shaderCache()->compileProgram("PixelRectShader",
                                                                      m_vertexGenerator.m_finalShaderBuilder.constData(),
                                                                      m_fragmentGenerator.m_finalShaderBuilder.constData(),
                                                                      nullptr, // no tess control shader
                                                                      nullptr, // no tess eval shader
                                                                      nullptr, // no geometry shader
                                                                      QSSGShaderCacheProgramFlags(),
                                                                      shaderCacheNoFeatures()));
        }
    }
    void generateXYQuad()
    {
        const QSSGRef<QSSGRenderContext> &theRenderContext(m_renderContext->renderContext());

        QSSGRenderVertexBufferEntry theEntries[] = {
            QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 2),
        };

        QVector2D pos[] = { QVector2D(0, 0), QVector2D(0, 1), QVector2D(1, 1), QVector2D(1, 0) };

        if (m_quadVertexBuffer == nullptr) {
            m_quadVertexBuffer = new QSSGRenderVertexBuffer(theRenderContext, QSSGRenderBufferUsageType::Static,
                                                              2 * sizeof(float),
                                                              toByteView(pos, 4));
        }

        if (m_quadIndexBuffer == nullptr) {
            quint8 indexData[] = {
                0, 1, 2, 0, 2, 3,
            };
            m_quadIndexBuffer = new QSSGRenderIndexBuffer(theRenderContext, QSSGRenderBufferUsageType::Static,
                                                            QSSGRenderComponentType::UnsignedInteger8,
                                                            toByteView(indexData, sizeof(indexData)));
        }

        if (m_quadAttribLayout == nullptr) {
            // create our attribute layout
            m_quadAttribLayout = theRenderContext->createAttributeLayout(toDataView(theEntries, 1));
        }

        if (m_quadInputAssembler == nullptr) {

            // create input assembler object
            quint32 strides = m_quadVertexBuffer->stride();
            quint32 offsets = 0;
            m_quadInputAssembler = theRenderContext->createInputAssembler(m_quadAttribLayout,
                                                                          toDataView(&m_quadVertexBuffer, 1),
                                                                          m_quadIndexBuffer,
                                                                          toDataView(&strides, 1),
                                                                          toDataView(&offsets, 1));
        }
    }

    void renderPixelObject(QMatrix4x4 &inProjection, const QSSGPGRect &inObject)
    {
        generateXYQuad();
        getRectShaderProgram();
        if (m_rectShader) {
            m_renderContext->renderContext()->setActiveShader(m_rectShader.rectShader);
            m_rectShader.apply(inProjection, inObject);

            m_renderContext->renderContext()->setInputAssembler(m_quadInputAssembler);
            m_renderContext->renderContext()->draw(QSSGRenderDrawMode::Triangles, m_quadInputAssembler->indexCount(), 0);
        }
    }

    void renderPixelObject(QMatrix4x4 &inProjection, const QSSGPGVertLine &inObject)
    {
        // lines are really just rects, but they grow in width in a sort of odd way.
        // specifically, they grow the increasing coordinate on even boundaries and centered on odd
        // boundaries.
        QSSGPGRect theRect;
        theRect.top = inObject.top;
        theRect.bottom = inObject.bottom;
        theRect.fillColor = inObject.lineColor;
        theRect.left = inObject.x;
        theRect.right = theRect.left + 1.0f;
        renderPixelObject(inProjection, theRect);
    }

    void renderPixelObject(QMatrix4x4 &inProjection, const QSSGPGHorzLine &inObject)
    {
        QSSGPGRect theRect;
        theRect.right = inObject.right;
        theRect.left = inObject.left;
        theRect.fillColor = inObject.lineColor;
        theRect.bottom = inObject.y;
        theRect.top = theRect.bottom + 1.0f;
        renderPixelObject(inProjection, theRect);
    }

    void render(const QVector<QSSGPGGraphObject *> &inObjects) override
    {
        const QSSGRef<QSSGRenderContext> &theRenderContext(m_renderContext->renderContext());
        theRenderContext->pushPropertySet();
        // Setup an orthographic camera that places the center at the
        // lower left of the viewport.
        QRectF theViewport = theRenderContext->viewport();
        // With no projection at all, we are going to get a square view box
        // with boundaries from -1,1 in all dimensions.  This is close to what we want.
        theRenderContext->setDepthTestEnabled(false);
        theRenderContext->setDepthWriteEnabled(false);
        theRenderContext->setScissorTestEnabled(false);
        theRenderContext->setBlendingEnabled(true);
        theRenderContext->setCullingEnabled(false);
        // Colors are expected to be non-premultiplied, so we premultiply alpha into them at this
        // point.
        theRenderContext->setBlendFunction(QSSGRenderBlendFunctionArgument(QSSGRenderSrcBlendFunc::SrcAlpha,
                                                                             QSSGRenderDstBlendFunc::OneMinusSrcAlpha,
                                                                             QSSGRenderSrcBlendFunc::One,
                                                                             QSSGRenderDstBlendFunc::OneMinusSrcAlpha));
        theRenderContext->setBlendEquation(
                QSSGRenderBlendEquationArgument(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add));

        QSSGRenderCamera theCamera;
        theCamera.position.setZ(-5.f);
        theCamera.clipNear = 1.0f;
        theCamera.clipFar = 10.0f;
        theCamera.flags.setFlag(QSSGRenderCamera::Flag::Orthographic);
        // Setup camera projection
        theCamera.computeFrustumOrtho(theViewport);
        // Translate such that 0, 0 is lower left of screen.
        QRectF theIdealViewport = theViewport;
        theIdealViewport.setX(theIdealViewport.x() - theViewport.width() / 2.0f);
        theIdealViewport.setY(theIdealViewport.y() - theViewport.height() / 2.0f);
        QMatrix4x4 theProjectionMatrix = QSSGRenderContext::applyVirtualViewportToProjectionMatrix(theCamera.projection,
                                                                                                     theViewport,
                                                                                                     theIdealViewport);
        theCamera.projection = theProjectionMatrix;
        // Explicitly call the node's calculate global variables so that the camera doesn't attempt
        // to change the projection we setup.
        static_cast<QSSGRenderNode &>(theCamera).calculateGlobalVariables();
        QMatrix4x4 theVPMatrix;
        theCamera.calculateViewProjectionMatrix(theVPMatrix);

        QVector4D theTest(60, 200, 0, 1);
        QVector4D theResult = mat44::transform(theVPMatrix, theTest);

        (void)theTest;
        (void)theResult;

        for (quint32 idx = 0, end = inObjects.size(); idx < end; ++idx) {
            const QSSGPGGraphObject &theObject(*inObjects[idx]);

            switch (theObject.type) {
            case QSSGGTypes::VertLine:
                renderPixelObject(theVPMatrix, static_cast<const QSSGPGVertLine &>(theObject));
                break;
            case QSSGGTypes::HorzLine:
                renderPixelObject(theVPMatrix, static_cast<const QSSGPGHorzLine &>(theObject));
                break;
            case QSSGGTypes::Rect:
                renderPixelObject(theVPMatrix, static_cast<const QSSGPGRect &>(theObject));
                break;
            default:
                Q_ASSERT(false);
                break;
            }
        }

        theRenderContext->popPropertySet(false);
    }
};
}

QSSGPixelGraphicsRendererInterface::~QSSGPixelGraphicsRendererInterface() = default;

QSSGRef<QSSGPixelGraphicsRendererInterface> QSSGPixelGraphicsRendererInterface::createRenderer(QSSGRenderContextInterface *ctx)
{
    return QSSGRef<QSSGPixelGraphicsRenderer>(new QSSGPixelGraphicsRenderer(ctx));
}

QT_END_NAMESPACE
