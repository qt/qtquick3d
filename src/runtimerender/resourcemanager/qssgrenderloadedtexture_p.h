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

#ifndef QSSG_RENDER_LOADED_TEXTURE_H
#define QSSG_RENDER_LOADED_TEXTURE_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>

#include <QtGui/QImage>

#include <private/qtexturefiledata_p.h>

QT_BEGIN_NAMESPACE
class QSSGInputStreamFactory;
class QSSGRenderTextureData;

struct QSSGTextureData
{
    void *data = nullptr;
    quint32 dataSizeInBytes = 0;
    QSSGRenderTextureFormat format = QSSGRenderTextureFormat::Unknown;
};

// Utility class used for loading image data from disk.
// Supports jpg, png, and dds.
struct QSSGLoadedTexture
{
public:
    qint32 width = 0;
    qint32 height = 0;
    qint32 components = 0;
    void *data = nullptr;
    bool ownsData = true;
    QTextureFileData compressedData;
    QImage image;
    quint32 dataSizeInBytes = 0;
    QSSGRenderTextureFormat format = QSSGRenderTextureFormat::RGBA8;

    ~QSSGLoadedTexture();
    void setFormatFromComponents()
    {
        switch (components) {
        case 1: // undefined, but in this context probably luminance
            format = QSSGRenderTextureFormat::R8;
            break;
        case 2:
            format = QSSGRenderTextureFormat::RG8;
            break;
        case 3:
            format = QSSGRenderTextureFormat::RGB8;
            break;

        default:
            // fallthrough intentional
        case 4:
            format = QSSGRenderTextureFormat::RGBA8;
            break;
        }
    }

    // Returns true if this image has a pixel less than 255.
    bool scanForTransparency() const;

    static QSSGLoadedTexture *load(const QString &inPath,
                                   const QSSGRenderTextureFormat &inFormat,
                                   QSSGInputStreamFactory &inFactory,
                                   bool inFlipY = true);
    static QSSGLoadedTexture *loadQImage(const QString &inPath, qint32 flipVertical);
    static QSSGLoadedTexture *loadCompressedImage(const QString &inPath, bool inFlipY = true);
    static QSSGLoadedTexture *loadHdrImage(const QSharedPointer<QIODevice> &source, const QSSGRenderTextureFormat &inFormat);
    static QSSGLoadedTexture *loadTextureData(QSSGRenderTextureData *textureData);
};
QT_END_NAMESPACE

#endif
