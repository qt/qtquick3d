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

#include "renderexample.h"
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <cmath>

#include <QtGui/QSurfaceFormat>

QSSGRenderExample::QSSGRenderExample(QWindow *parent)
    : QWindow(parent)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setWidth(1280);
    setHeight(720);
    m_frameTimer.start();
}

QSSGRenderExample::~QSSGRenderExample()
{
    delete m_context;
}

void QSSGRenderExample::renderLater()
{
    requestUpdate();
}

void QSSGRenderExample::renderNow()
{
    if (!m_isIntialized) {
        preInit();
        initialize();
        m_isIntialized = true;
    }
    m_context->makeCurrent(this);
    drawFrame(m_frameTimer.elapsed());
    m_frameTimer.restart();
    m_context->swapBuffers(this);
    m_context->doneCurrent();
    if (m_autoUpdate)
        renderLater();
}

bool QSSGRenderExample::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void QSSGRenderExample::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void QSSGRenderExample::preInit()
{
    m_context = new QOpenGLContext();
    m_context->setFormat(requestedFormat());
    m_context->create();

    if (!m_context->makeCurrent(this))
        qDebug("fail");
}


// Math stuff

int eq(float a, float b)
{
    float diff = a - b;
    if (diff < 0) {
        diff = -diff;
    }
    return diff <= eps;
}
