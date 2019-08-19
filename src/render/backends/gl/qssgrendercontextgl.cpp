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

#include <QtGui/QMatrix4x4>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRender/private/qssgrenderprogrampipeline_p.h>
#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE

QSSGRef<QSSGRenderContext> QSSGRenderContext::createGl(const QSurfaceFormat &format)
{
    QSSGRef<QSSGRenderContext> retval;

    Q_ASSERT(format.majorVersion() >= 2);

    // create backend
    QSSGRef<QSSGRenderBackend> theBackend;
    bool isES = format.renderableType() == QSurfaceFormat::OpenGLES;
    if (isES && (format.majorVersion() == 2 || (format.majorVersion() == 3 && format.minorVersion() == 0))) {
        theBackend = new QSSGRenderBackendGLES2Impl(format);
    } else if (format.majorVersion() == 3 && format.minorVersion() >= 1 && !isES) {
        theBackend = new QSSGRenderBackendGL3Impl(format);
    } else if (format.majorVersion() == 4 || (isES && format.majorVersion() == 3 && format.minorVersion() >= 1)) {
#ifdef Q_OS_MACOS
        // TODO: macOS crashes with glTextStorage2DMultisample, so fall back to OpenGL3
        // for now (QSSG-590)
        theBackend = new QSSGRenderBackendGL3Impl(format);
#else
        theBackend = new QSSGRenderBackendGL4Impl(format);
#endif
    } else {
        Q_ASSERT(false);
        qCCritical(INTERNAL_ERROR) << "Can't find a suitable OpenGL version for" << format;
    }

    QSSGRef<QSSGRenderContext> impl(new QSSGRenderContext(theBackend));
    retval = impl;

    return retval;
}

QT_END_NAMESPACE
