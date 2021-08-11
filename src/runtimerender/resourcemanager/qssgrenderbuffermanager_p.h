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
#include <QtQuick3DRuntimeRender/private/qssgrenderimagetexture_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendermesh_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DUtils/private/qssgmesh_p.h>

#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

struct QSSGRenderMesh;
struct QSSGLoadedTexture;
class QSSGRhiContext;
struct QSSGMeshBVH;
class QSGTexture;
class QSSGRenderGeometry;
class QSSGRenderTextureData;
struct QSSGRenderModel;
struct QSSGRenderImage;

// There is one QSSGBufferManager per QSSGRenderContextInterface, and so per
// QQuickWindow, and by extension, per scenegraph render thread. This is
// essential here because graphics resources (vertex/index buffers, textures)
// are always specific to one render thread, they cannot be used and shared
// between different threads (and so windows). This is ensured by design, by
// having a dedicated BufferManager for each render thread (window).

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGBufferManager
{
public:
    QAtomicInt ref;

    struct ImageCacheKey {
        QSSGRenderPath path;
        int mipMode;
    };

private:
    typedef QHash<ImageCacheKey, QSSGRenderImageTexture> ImageMap;
    typedef QHash<QSGTexture *, QSSGRenderImageTexture> QSGImageMap;
    typedef QHash<QSSGRenderPath, QSSGRenderMesh *> MeshMap;
    typedef QHash<QSSGRenderGeometry *, QSSGRenderMesh *> CustomMeshMap;
    typedef QHash<QSSGRenderTextureData *, QSSGRenderImageTexture> CustomTextureMap;

    typedef QHash<QSSGRenderPath, QSet<const QSSGRenderModel *>> ModelPathRefereneMap;
    typedef QHash<QSSGRenderPath, QSet<const QSSGRenderImage *>> ImagePathReferenceMap;
    typedef QHash<const QSSGRenderModel *, QSSGRenderPath> ModelPathMap;
    typedef QHash<const QSSGRenderImage *, QSSGRenderPath> ImagePathMap;


    QSSGRef<QSSGRhiContext> context;
    QSSGRef<QSSGShaderCache> shaderCache;
    ImageMap imageMap;
    QSGImageMap qsgImageMap;
    MeshMap meshMap;
    CustomMeshMap customMeshMap;
    CustomTextureMap customTextureMap;
    QVector<QSSGRenderVertexBufferEntry> entryBuffer;
    ModelPathRefereneMap modelRefMap;
    ImagePathReferenceMap imageRefMap;
    ModelPathMap cachedModelPathMap;
    ImagePathMap cachedImagePathMap;
    QRhiResourceUpdateBatch *meshBufferUpdates = nullptr;
    QMutex meshBufferMutex;

    void clear();

    QSSGMesh::Mesh loadPrimitive(const QString &inRelativePath) const;

public:
    enum MipMode {
        MipModeNone = 0,
        MipModeBsdf,
        MipModeGenerated,
    };

    enum LoadRenderImageFlag {
        LoadWithFlippedY = 0x01
    };
    Q_DECLARE_FLAGS(LoadRenderImageFlags, LoadRenderImageFlag)

    QSSGBufferManager(const QSSGRef<QSSGRhiContext> &inRenderContext,
                      const QSSGRef<QSSGShaderCache> &inShaderContext);
    ~QSSGBufferManager();

    QSSGRenderImageTexture loadRenderImage(const QSSGRenderImage *image,
                                           MipMode inMipMode = MipModeNone,
                                           LoadRenderImageFlags flags = LoadWithFlippedY);
    QSSGRenderImageTexture loadTextureData(QSSGRenderTextureData *data, MipMode inMipMode);
    QSSGRenderMesh *getMesh(const QSSGRenderPath &inSourcePath) const;
    QSSGRenderMesh *getMesh(QSSGRenderGeometry *geometry) const;
    QSSGRenderMesh *loadMesh(const QSSGRenderModel *model);
    QSSGRenderMesh *loadCustomMesh(QSSGRenderGeometry *geometry,
                                   const QSSGMesh::Mesh &mesh,
                                   bool update = false);
    QSSGMeshBVH *loadMeshBVH(const QSSGRenderPath &inSourcePath);
    QSSGMeshBVH *loadMeshBVH(QSSGRenderGeometry *geometry);
    QSSGMesh::Mesh loadMeshData(const QSSGRenderPath &inSourcePath) const;

    QSSGRenderMesh *createRenderMesh(const QSSGMesh::Mesh &mesh);

    void addMeshReference(const QSSGRenderPath &sourcePath, const QSSGRenderModel *model);
    void addImageReference(const QSSGRenderPath &sourcePath, const QSSGRenderImage *image);
    void removeMeshReference(const QSSGRenderPath &sourcePath, const QSSGRenderModel *model);
    void removeImageReference(const QSSGRenderPath &sourcePath, const QSSGRenderImage *image);
    void cleanupUnreferencedBuffers();
    void releaseGeometry(QSSGRenderGeometry *geometry);
    void releaseTextureData(QSSGRenderTextureData *textureData);

    static QRhiTexture::Format toRhiFormat(const QSSGRenderTextureFormat format);

    QRhiResourceUpdateBatch *meshBufferUpdateBatch();
    void commitBufferResourceUpdates();

    static void registerMeshData(const QString &assetId, const QVector<QSSGMesh::Mesh> &meshData);
    static void unregisterMeshData(const QString &assetId);
    static QString runtimeMeshSourceName(const QString &assetId, qsizetype meshId);
    static QString primitivePath(const QString &primitive);

    QMutex *meshUpdateMutex();

private:
    bool createRhiTexture(QSSGRenderImageTexture &texture,
                          const QSSGLoadedTexture *inTexture,
                          bool inForceScanForTransparency = false,
                          MipMode inMipMode = MipModeNone);
    QSSGRenderMesh *loadMesh(const QSSGRenderPath &inSourcePath);
    bool createEnvironmentMap(const QSSGLoadedTexture *inImage, QSSGRenderImageTexture *outTexture);
    void releaseMesh(const QSSGRenderPath &inSourcePath);
    void releaseImage(const ImageCacheKey &key);
    void releaseImage(const QSSGRenderPath &sourcePath);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGBufferManager::LoadRenderImageFlags)

inline size_t qHash(const QSSGBufferManager::ImageCacheKey &k, size_t seed) Q_DECL_NOTHROW
{
    return qHash(k.path, seed) ^ k.mipMode;
}

inline bool operator==(const QSSGBufferManager::ImageCacheKey &a, const QSSGBufferManager::ImageCacheKey &b) Q_DECL_NOTHROW
{
    return a.path == b.path && a.mipMode == b.mipMode;
}

QT_END_NAMESPACE

#endif
