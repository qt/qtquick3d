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

#ifndef QSSG_RENDER_DYNAMIC_OBJECT_SYSTEM_H
#define QSSG_RENDER_DYNAMIC_OBJECT_SYSTEM_H

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

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>

#include <QtGui/QVector2D>

#include <QtCore/QString>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

class QSSGRenderContextInterface;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderLibraryManager
{
    struct QSSGShaderInfo
    {
        QByteArray m_type; ///< shader type (GLSL or HLSL)
        QByteArray m_version; ///< shader version (e.g. 330 vor GLSL)

        QSSGShaderInfo() { }
        QSSGShaderInfo(const QByteArray &inType, const QByteArray &inVersion)
            : m_type(inType), m_version(inVersion)
        {
        }
    };

    typedef QHash<QByteArray, QByteArray> TPathDataMap;
    typedef QHash<QByteArray, QSSGShaderInfo> TShaderInfoMap;
    typedef QSet<QString> TPathSet;

    QSSGRenderContextInterface *m_context;
    TPathDataMap m_expandedFiles;
    TShaderInfoMap m_shaderInfoMap;
    QByteArray m_vertShader;
    QByteArray m_fragShader;
    mutable QMutex m_propertyLoadMutex;
    QAtomicInt ref;

    static QString getShaderCodeLibraryDirectory();

    QSSGShaderLibraryManager(QSSGRenderContextInterface *ctx);

    ~QSSGShaderLibraryManager();

    void setShaderData(const QByteArray &inPath,
                       const QByteArray &inData,
                       const QByteArray &inShaderType,
                       const QByteArray &inShaderVersion);


    void resolveIncludeFiles(QByteArray &theReadBuffer, const QByteArray &inPath);

    static QByteArrayList getParameters(const QByteArray &str, int begin, int end);

    static void insertSnapperDirectives(QByteArray &str);

    // This just returns the custom material shader source without compiling
    QByteArray getShaderSource(const QByteArray &inPath);

    void setShaderCodeLibraryVersion(const QByteArray &version);
};

QT_END_NAMESPACE

#endif
