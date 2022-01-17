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

#include <QtQuick3DUtils/private/qquick3dprofiler_p.h>

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
struct QSSGRenderResourceLoader;
struct QSSGRenderLayer;

// There is one QSSGBufferManager per QSSGRenderContextInterface, and so per
// QQuickWindow, and by extension, per scenegraph render thread. This is
// essential here because graphics resources (vertex/index buffers, textures)
// are always specific to one render thread, they cannot be used and shared
// between different threads (and so windows). This is ensured by design, by
// having a dedicated BufferManager for each render thread (window).

class QSSGRenderContextInterface;
class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGBufferManager
{
public:
    QAtomicInt ref;

    struct ImageCacheKey {
        QSSGRenderPath path;
        int mipMode;
    };

    struct ImageData {
        QSSGRenderImageTexture renderImageTexture;
        QHash<QSSGRenderLayer*, uint32_t> usageCounts;
        uint32_t generationId = 0;
    };

    struct MeshData {
        QSSGRenderMesh *mesh = nullptr;
        QHash<QSSGRenderLayer*, uint32_t> usageCounts;
        uint32_t generationId = 0;
    };

    struct MemoryStats {
        uint64_t meshDataSize = 0;
        uint64_t imageDataSize = 0;
    };

    enum MipMode {
        MipModeNone = 0,
        MipModeBsdf,
        MipModeGenerated,
    };

    enum LoadRenderImageFlag {
        LoadWithFlippedY = 0x01
    };
    Q_DECLARE_FLAGS(LoadRenderImageFlags, LoadRenderImageFlag)

    QSSGBufferManager();
    ~QSSGBufferManager();

    void setRenderContextInterface(QSSGRenderContextInterface *ctx);

    QSSGRenderImageTexture loadRenderImage(const QSSGRenderImage *image,
                                           MipMode inMipMode = MipModeNone,
                                           LoadRenderImageFlags flags = LoadWithFlippedY);

    QSSGRenderMesh *getMeshForPicking(const QSSGRenderModel &model) const;
    QSSGBounds3 getModelBounds(const QSSGRenderModel *model) const;

    QSSGRenderMesh *loadMesh(const QSSGRenderModel *model);

    // Called at the end of the frame to release unreferenced geometry and textures
    void cleanupUnreferencedBuffers(quint32 frameId, QSSGRenderLayer *layer);
    void resetUsageCounters(quint32 frameId, QSSGRenderLayer *layer);

    void releaseGeometry(QSSGRenderGeometry *geometry);
    void releaseTextureData(QSSGRenderTextureData *textureData);

    void commitBufferResourceUpdates();

    void processResourceLoader(const QSSGRenderResourceLoader *loader);

    static QSSGMeshBVH *loadMeshBVH(const QSSGRenderPath &inSourcePath);
    static QSSGMeshBVH *loadMeshBVH(QSSGRenderGeometry *geometry);

    static QRhiTexture::Format toRhiFormat(const QSSGRenderTextureFormat format);

    static void registerMeshData(const QString &assetId, const QVector<QSSGMesh::Mesh> &meshData);
    static void unregisterMeshData(const QString &assetId);
    static QString runtimeMeshSourceName(const QString &assetId, qsizetype meshId);
    static QString primitivePath(const QString &primitive);

    QMutex *meshUpdateMutex();
#if QT_CONFIG(qml_debug)
    MemoryStats memoryStats() const;
    void increaseMemoryStat(QRhiTexture *texture);
    void decreaseMemoryStat(QRhiTexture *texture);
    void increaseMemoryStat(QSSGRenderMesh *mesh);
    void decreaseMemoryStat(QSSGRenderMesh *mesh);
#endif
    // Views for testing
    const QHash<ImageCacheKey, ImageData> &getImageMap() const { return imageMap; }
    const QHash<QSGTexture *, ImageData> &getSGImageMap() const { return qsgImageMap; }
    const QHash<QSSGRenderPath, MeshData> &getMeshMap() const { return meshMap; }
    const QHash<QSSGRenderGeometry *, MeshData> &getCustomMeshMap() const { return customMeshMap; }
    const QHash<QSSGRenderTextureData *, ImageData> &getCustomTextureMap() const { return customTextureMap; }

private:
    void clear();
    QRhiResourceUpdateBatch *meshBufferUpdateBatch();

    static QSSGMesh::Mesh loadPrimitive(const QString &inRelativePath);
    bool createRhiTexture(QSSGRenderImageTexture &texture,
                          const QSSGLoadedTexture *inTexture,
                          bool inForceScanForTransparency = false,
                          MipMode inMipMode = MipModeNone);
    QSSGRenderMesh *loadMesh(const QSSGRenderPath &inSourcePath);
    QSSGRenderMesh *loadCustomMesh(QSSGRenderGeometry *geometry);
    static QSSGMesh::Mesh loadMeshData(const QSSGRenderPath &inSourcePath);
    QSSGRenderMesh *createRenderMesh(const QSSGMesh::Mesh &mesh);
    QSSGRenderImageTexture loadTextureData(QSSGRenderTextureData *data, MipMode inMipMode);
    bool createEnvironmentMap(const QSSGLoadedTexture *inImage, QSSGRenderImageTexture *outTexture);

    void releaseMesh(const QSSGRenderPath &inSourcePath);
    void releaseImage(const ImageCacheKey &key);

    QSSGRenderContextInterface *m_contextInterface = nullptr; // ContextInterfaces owns BufferManager

    // These store the actual buffer handles
    QHash<ImageCacheKey, ImageData> imageMap;                   // Textures (specificed by path)
    QHash<QSGTexture *, ImageData> qsgImageMap;                 // Textures (from Qt Quick)
    QHash<QSSGRenderPath, MeshData> meshMap;                    // Meshes (specififed by path)
    QHash<QSSGRenderGeometry *, MeshData> customMeshMap;        // Meshes (QQuick3DGeometry)
    QHash<QSSGRenderTextureData *, ImageData> customTextureMap; // Textures (QQuick3DTextureData)

    QRhiResourceUpdateBatch *meshBufferUpdates = nullptr;
    QMutex meshBufferMutex;

    quint32 frameCleanupIndex = 0;
    quint32 frameResetIndex = 0;
    QSSGRenderLayer *currentLayer = nullptr;
#if QT_CONFIG(qml_debug)
    MemoryStats stats;
#endif
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
