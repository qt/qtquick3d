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

#ifndef QSSG_RENDER_BUFFER_MANAGER_H
#define QSSG_RENDER_BUFFER_MANAGER_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimagetexturedata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DAssetImport/private/qssgmeshutilities_p.h>
#include <QtQuick3DUtils/private/qssgperftimer_p.h>
#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>


QT_BEGIN_NAMESPACE

struct QSSGRenderMesh;
struct QSSGLoadedTexture;
class QSSGRhiContext;
struct QSSGMeshBVH;
class QSGTexture;
class QSSGRenderGeometry;
namespace QSSGMeshUtilities {
    struct MultiLoadResult;
}

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGBufferManager
{
public:
    QAtomicInt ref;
private:
    typedef QHash<QSSGRenderPath, QSSGRenderImageTextureData> ImageMap;
    typedef QHash<QSGTexture *, QSSGRenderImageTextureData> QSGImageMap;
    typedef QHash<QSSGRenderPath, QSSGRenderMesh *> MeshMap;
    typedef QHash<QSSGRenderGeometry *, QSSGRenderMesh *> CustomMeshMap;

    QSSGRef<QSSGRhiContext> context;
    QSSGRef<QSSGInputStreamFactory> inputStreamFactory;
    ImageMap imageMap;
    QSGImageMap qsgImageMap;
    MeshMap meshMap;
    CustomMeshMap customMeshMap;
    QVector<QSSGRenderVertexBufferEntry> entryBuffer;

    void clear();

    QSSGMeshUtilities::MultiLoadResult loadPrimitive(const QString &inRelativePath) const;
    QVector<QVector3D> createPackedPositionDataArray(
            const QSSGMeshUtilities::MultiLoadResult &inResult) const;
    static void releaseMesh(QSSGRenderMesh &inMesh);
    static void releaseTexture(QSSGRenderImageTextureData &inEntry);

public:
    QSSGBufferManager(const QSSGRef<QSSGRhiContext> &inRenderContext,
                        const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory);
    ~QSSGBufferManager();

    // Returns a texture and a boolean indicating if this texture has transparency in it or not.
    // Can't name this LoadImage because that gets mangled by windows to LoadImageA (uggh)
    // In some cases we need to only scan particular images for transparency.
    QSSGRenderImageTextureData loadRenderImage(const QSSGRenderPath &inImagePath,
                                                 const QSSGRef<QSSGLoadedTexture> &inTexture,
                                                 bool inForceScanForTransparency = false,
                                                 bool inBsdfMipmaps = false);
    QSSGRenderImageTextureData loadRenderImage(const QSSGRenderPath &inSourcePath,
                                                 const QSSGRenderTextureFormat &inFormat = QSSGRenderTextureFormat::Unknown,
                                                 bool inForceScanForTransparency = false,
                                                 bool inBsdfMipmaps = false);
    QSSGRenderImageTextureData loadRenderImage(QSGTexture *qsgTexture);
    QSSGRenderMesh *getMesh(const QSSGRenderPath &inSourcePath) const;
    QSSGRenderMesh *getMesh(QSSGRenderGeometry *geometry) const;
    QSSGRenderMesh *loadMesh(const QSSGRenderPath &inSourcePath);
    QSSGRenderMesh *loadCustomMesh(QSSGRenderGeometry *geometry,
                                   QSSGMeshUtilities::Mesh *mesh,
                                   bool update = false);
    QSSGMeshBVH *loadMeshBVH(const QSSGRenderPath &inSourcePath);
    QSSGMeshUtilities::MultiLoadResult loadMeshData(const QSSGRenderPath &inSourcePath) const;

    QSSGRenderMesh *createRenderMesh(const QSSGMeshUtilities::MultiLoadResult &result);

    static QRhiTexture::Format toRhiFormat(const QSSGRenderTextureFormat format);

private:
    bool loadRenderImageComputeMipmap(const QSSGLoadedTexture *inImage, QSSGRenderImageTextureData *outImageData);
};
QT_END_NAMESPACE

#endif
