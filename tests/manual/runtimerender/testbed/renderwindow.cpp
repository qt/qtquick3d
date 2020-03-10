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

#include "renderwindow.h"
#include <QtQuick3DRender/private/qssgrendercontext_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderlayer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermodel_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterial_p.h>
#include <QtQuick3D/qquick3d.h>

RenderWindow::RenderWindow(QWindow *parent)
    : QWindow(parent)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setWidth(1280);
    setHeight(720);
    m_frameTimer.start();
}

RenderWindow::~RenderWindow()
{
    delete m_glContext;
}

void RenderWindow::initialize()
{
    m_renderContext = QSSGRenderContext::createGl(QQuick3D::idealSurfaceFormat());

    m_context = QSSGRenderContextInterface::getRenderContextInterface(m_renderContext, "./", quintptr(this));
    m_context->setSceneColor(QColor(1.0, 0.0, 0.0, 0.0));

    buildTestScene();
}

void RenderWindow::drawFrame(qint64 delta)
{
    updateAnimations(delta);

    QSize renderTargetSize = size() * devicePixelRatio();

    m_context->setViewport(QRect(0,0,renderTargetSize.width(),renderTargetSize.height()));
    m_context->beginFrame();
    m_context->prepareLayerForRender(*m_layer);
    m_context->renderLayer(*m_layer, false);
    m_context->endFrame();
}

void RenderWindow::renderLater()
{
    requestUpdate();
}

void RenderWindow::renderNow()
{
    if (!m_isIntialized) {
        preInit();
        initialize();
        m_isIntialized = true;
    }
    m_glContext->makeCurrent(this);
    drawFrame(m_frameTimer.elapsed());
    m_frameTimer.restart();
    m_glContext->swapBuffers(this);
    m_glContext->doneCurrent();
    if (m_autoUpdate)
        renderLater();
}

void RenderWindow::updateAnimations(qint64 delta)
{
    QVector3D angles = m_cube->rotation.toEulerAngles();
    angles = QVector3D(60, 45, angles.z() + (delta * 0.1f));
    m_cube->rotation = QQuaternion::fromEulerAngles(angles);
    m_cube->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformIsDirty);

    QVector3D planetsAngles = m_planetsRoot->rotation.toEulerAngles();
    planetsAngles = QVector3D(-60, -145, planetsAngles.z() + (delta * 0.01f));
    m_planetsRoot->rotation = QQuaternion::fromEulerAngles(planetsAngles);
    m_planetsRoot->markDirty(QSSGRenderNode::TransformDirtyFlag::TransformIsDirty);
}

bool RenderWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void RenderWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event)

    if (isExposed())
        renderNow();
}

void RenderWindow::preInit()
{
    m_glContext = new QOpenGLContext();
    m_glContext->setFormat(requestedFormat());
    m_glContext->create();

    if (!m_glContext->makeCurrent(this))
        qDebug("fail");
}

void RenderWindow::buildTestScene()
{
    m_layer = new QSSGRenderLayer();
    m_layer->clearColor = QVector3D(0.4f, 0.4f, 0.4f);
    m_layer->background = QSSGRenderLayer::Background::Color;

    // Camera
    auto camera = new QSSGRenderCamera();
    m_layer->addChild(*camera);
    camera->lookAt(QVector3D(0.0, 0.0, 600.0),
                   QVector3D(0.0, 0.0, 0.0),
                   QVector3D(0.0, 0.0, 0.0));

    // Light
    auto light = new QSSGRenderLight();
    m_layer->addChild(*light);

    // Mesh (#Cube)
    m_cube = new QSSGRenderModel();
    m_cube->scale = QVector3D(3.0f, 2.0f, 1.0f);
    m_cube->meshPath = QSSGRenderMeshPath::create(QStringLiteral("#Cube"));
    m_layer->addChild(*m_cube);

    // Cube Material
    auto material = new QSSGRenderDefaultMaterial(QSSGRenderGraphObject::Type::DefaultMaterial);
    material->color = QVector4D(0.4f, 0.6f, 0.3f, 1.0f);
    m_cube->materials.append(material);

    // Planets
    m_planetsRoot = new QSSGRenderNode();
    m_layer->addChild(*m_planetsRoot);
    auto planetMaterial = new QSSGRenderDefaultMaterial(QSSGRenderGraphObject::Type::DefaultMaterial);
    planetMaterial->color = QVector4D(0.8f, 0.6f, 0.3f, 1.0f);
    int area = std::max(width(), height());
    int planets = 100;
    for (int i = 0; i < planets; i++) {
        auto planet = new QSSGRenderModel();
        planet->scale = QVector3D(.1f, .1f, .1f);
        planet->meshPath = QSSGRenderMeshPath::create(QStringLiteral("#Sphere"));
        planet->materials.append(planetMaterial);
        planet->position = QVector3D(area / 2 - rand() % area,
                                     area / 2 - rand() % area,
                                     area / 2 - rand() % area);
        m_planetsRoot->addChild(*planet);
    }
}
