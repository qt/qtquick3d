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
    Q_ASSERT(format.majorVersion() >= 2);

    /*
     * Currently we are using 3 backends and you can force to use
     * which backend as following values
     * ES2 backend for QT_QUICK3D_FORCE_OPENGL_BACKEND=1
     * GL3 backend for QT_QUICK3D_FORCE_OPENGL_BACKEND=2
     * GL4 backend for QT_QUICK3D_FORCE_OPENGL_BACKEND=3
     */
    // create backend
    QSSGRenderBackend *theBackend = nullptr;
    static int envBE = qEnvironmentVariableIntValue("QT_QUICK3D_FORCE_OPENGL_BACKEND");
    if (envBE == 1) {
        theBackend = new QSSGRenderBackendGLES2Impl(format);
    } else if (envBE == 2) {
        theBackend = new QSSGRenderBackendGL3Impl(format);
    } else if (envBE == 3) {
        theBackend = new QSSGRenderBackendGL4Impl(format);
    } else {
        const bool isES = format.renderableType() == QSurfaceFormat::OpenGLES;
        const int majorVersion = format.majorVersion();
        const int minorVersion = format.minorVersion();

        if (isES && (majorVersion == 2 || (majorVersion == 3 && minorVersion == 0))) {
            theBackend = new QSSGRenderBackendGLES2Impl(format);
        } else if (majorVersion == 3 && minorVersion >= 1 && !isES) {
            theBackend = new QSSGRenderBackendGL3Impl(format);
        } else if (majorVersion == 4 || (isES && majorVersion == 3 && minorVersion >= 1)) {
#ifdef Q_OS_MACOS
            // TODO: macOS crashes with glTextStorage2DMultisample, so fall back to OpenGL3
            // for now (QSSG-590)
            theBackend = new QSSGRenderBackendGL3Impl(format);
#else
            theBackend = new QSSGRenderBackendGL4Impl(format);
#endif
        } else {
            Q_ASSERT(false);
            qCCritical(RENDER_INTERNAL_ERROR) << "Can't find a suitable OpenGL version for"
                                       << format;
        }
    }

    Q_ASSERT(theBackend != nullptr);

    return QSSGRef<QSSGRenderContext>(new QSSGRenderContext(theBackend));
}

QT_END_NAMESPACE
