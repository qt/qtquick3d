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
#include <QtQuick3DUtils/private/qqsbcollection_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>

#include <QtGui/QVector2D>

#include <QtCore/QString>
#include <QtCore/QReadWriteLock>

QT_BEGIN_NAMESPACE

class QSSGRenderContextInterface;

struct QSSGCustomShaderMetaData
{
    enum Flag {
        UsesScreenTexture = 1 << 0,
        UsesDepthTexture = 1 << 1,
        UsesAoTexture = 1 << 2,
        OverridesPosition = 1 << 3,
        UsesProjectionMatrix = 1 << 4,
        UsesInverseProjectionMatrix = 1 << 5,
        UsesScreenMipTexture = 1 << 6,
        UsesVarColor = 1 << 7
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    Flags flags;
    QSet<QByteArray> customFunctions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGCustomShaderMetaData::Flags)

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderLibraryManager
{
    typedef QHash<QByteArray, QByteArray> TPathDataMap;
    typedef QSet<QString> TPathSet;

    QSSGRef<QSSGInputStreamFactory> m_inputStreamFactory;
    TPathDataMap m_expandedFiles;
    QHash<QByteArray, QSSGCustomShaderMetaData> m_metadata;
    QByteArray m_vertShader;
    QByteArray m_fragShader;

    QQsbCollection::EntryMap m_shaderEntries;

    QAtomicInt ref;
    QReadWriteLock m_lock;

    static QString getShaderCodeLibraryDirectory();

    explicit QSSGShaderLibraryManager(const QSSGRef<QSSGInputStreamFactory> &inputStreamFactory);

    ~QSSGShaderLibraryManager();

    void setShaderSource(const QByteArray &inShaderPathKey, QSSGShaderCache::ShaderType type,
                         const QByteArray &inSource, const QSSGCustomShaderMetaData &meta);

    // Does not load any shaders, only information about the content of the pregenerated shaders
    void loadPregeneratedShaderInfo();

    void resolveIncludeFiles(QByteArray &theReadBuffer, const QByteArray &inMaterialInfoString);
    QByteArray getIncludeContents(const QByteArray &inShaderPathKey);

    QByteArray getShaderSource(const QByteArray &inShaderPathKey, QSSGShaderCache::ShaderType type);
    QSSGCustomShaderMetaData getShaderMetaData(const QByteArray &inShaderPathKey, QSSGShaderCache::ShaderType type);

    void setShaderCodeLibraryVersion(const QByteArray &version);

    static bool compare(const QSSGShaderDefaultMaterialKey &key1, const QSSGShaderDefaultMaterialKey &key2);
};

QT_END_NAMESPACE

#endif
