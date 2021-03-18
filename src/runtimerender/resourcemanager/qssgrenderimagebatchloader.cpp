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

#include "qssgrenderimagebatchloader_p.h"

#include <QtQuick3DUtils/private/qssginvasivelinkedlist_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderthreadpool_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderloadedtexture_p.h>

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

QT_BEGIN_NAMESPACE

namespace {

struct QSSGImageLoaderBatch;

struct QSSGLoadingImage
{
    QSSGImageLoaderBatch *batch = nullptr;
    QString sourcePath;
    quint64 taskId = 0;
    QSSGLoadingImage *tail = nullptr;

    // Called from main thread
    QSSGLoadingImage(const QString &inSourcePath) : batch(nullptr), sourcePath(inSourcePath), taskId(0), tail(nullptr) {}

    QSSGLoadingImage() = default;

    // Called from main thread
    void setup(QSSGImageLoaderBatch &inBatch);

    // Called from loader thread
    static void loadImage(void *inImg);

    // Potentially called from loader thread
    static void taskCancelled(void *inImg);
};

using TLoadingImageList = QSSGInvasiveSingleLinkedList<QSSGLoadingImage, &QSSGLoadingImage::tail>;

struct QSSGBatchLoader;

struct QSSGImageLoaderBatch
{
    // All variables setup in main thread and constant from then on except
    // loaded image count.
    QSSGBatchLoader &loader;
    QSSGRef<IImageLoadListener> loadListener;
    QWaitCondition loadEvent;
    QMutex loadMutex;
    TLoadingImageList images;

    TImageBatchId batchId;
    // Incremented in main thread
    quint32 loadedOrCanceledImageCount;
    quint32 finalizedImageCount;
    quint32 numImages;
    QSSGRenderContextType contextType;

    // Called from main thread
    static QSSGImageLoaderBatch *createLoaderBatch(QSSGBatchLoader &inLoader,
                                                   TImageBatchId inBatchId,
                                                   QSSGDataView<QString> inSourcePaths,
                                                   const QString &inImageTillLoaded,
                                                   IImageLoadListener *inListener,
                                                   QSSGRenderContextType contextType);

    // Called from main thread
    QSSGImageLoaderBatch(QSSGBatchLoader &inLoader,
                           IImageLoadListener *inLoadListener,
                           const TLoadingImageList &inImageList,
                           TImageBatchId inBatchId,
                           quint32 inImageCount,
                           QSSGRenderContextType contextType);

    // Called from main thread
    ~QSSGImageLoaderBatch();

    // Called from main thread
    bool isLoadingFinished()
    {
        QMutexLocker locker(&loadMutex);
        return loadedOrCanceledImageCount >= numImages;
    }

    bool isFinalizedFinished()
    {
        QMutexLocker locker(&loadMutex);
        return finalizedImageCount >= numImages;
    }

    void incrementLoadedImageCount()
    {
        QMutexLocker locker(&loadMutex);
        ++loadedOrCanceledImageCount;
    }
    void incrementFinalizedImageCount()
    {
        QMutexLocker locker(&loadMutex);
        ++finalizedImageCount;
    }
    // Called from main thread
    void cancel();
    void cancel(const QString &inSourcePath);
};

struct QSSGBatchLoadedImage
{
    QString sourcePath;
    QSSGRef<QSSGLoadedTexture> texture;
    QSSGImageLoaderBatch *batch = nullptr;
    QSSGBatchLoadedImage() = default;

    // Called from loading thread
    QSSGBatchLoadedImage(const QString &inSourcePath, QSSGLoadedTexture *inTexture, QSSGImageLoaderBatch &inBatch)
        : sourcePath(inSourcePath), texture(inTexture), batch(&inBatch)
    {
    }

    // Called from main thread
    bool finalize(const QSSGRef<QSSGBufferManager> &inMgr);
};

struct QSSGBatchLoader : public IImageBatchLoader
{
    typedef QHash<TImageBatchId, QSSGImageLoaderBatch *> TImageLoaderBatchMap;
    typedef QHash<QString, TImageBatchId> TSourcePathToBatchMap;

    // Accessed from loader thread
    QSSGRef<QSSGInputStreamFactory> inputStreamFactory;
    //!!Not threadsafe!  accessed only from main thread
    QSSGRef<QSSGBufferManager> bufferManager;
    // Accessed from main thread
    QSSGRef<QSSGAbstractThreadPool> threadPool;
    // Accessed from both threads
    QSSGPerfTimer *perfTimer;
    // main thread
    TImageBatchId nextBatchId;
    // main thread
    TImageLoaderBatchMap batches;
    // main thread
    QMutex loaderMutex;

    // Both loader and main threads
    QVector<QSSGBatchLoadedImage> loadedImages;
    // main thread
    QVector<TImageBatchId> finishedBatches;
    // main thread
    TSourcePathToBatchMap sourcePathToBatches;
    // main thread
    QVector<QSSGLoadingImage> loaderBuilderWorkspace;

    QSSGBatchLoader(const QSSGRef<QSSGInputStreamFactory> &inFactory,
                    const QSSGRef<QSSGBufferManager> &inBufferManager,
                    const QSSGRef<QSSGAbstractThreadPool> &inThreadPool,
                    QSSGPerfTimer *inTimer)
        : inputStreamFactory(inFactory), bufferManager(inBufferManager), threadPool(inThreadPool), perfTimer(inTimer), nextBatchId(1)
    {
    }

    virtual ~QSSGBatchLoader() override;

    // Returns an ID to the load request.  Request a block of images to be loaded.
    // Also takes an image that the buffer system will return when requested for the given source
    // paths
    // until said path is loaded.
    // An optional listener can be passed in to get callbacks about the batch.
    TImageBatchId loadImageBatch(QSSGDataView<QString> inSourcePaths,
                                 QString inImageTillLoaded,
                                 IImageLoadListener *inListener,
                                 QSSGRenderContextType contextType) override
    {
        if (inSourcePaths.size() == 0)
            return 0;

        QMutexLocker loaderLock(&loaderMutex);

        TImageBatchId theBatchId = 0;

        // Empty loop intentional to find an unused batch id.
        for (theBatchId = nextBatchId; batches.find(theBatchId) != batches.end(); ++nextBatchId, theBatchId = nextBatchId) {
        }

        QSSGImageLoaderBatch *theBatch(
                QSSGImageLoaderBatch::createLoaderBatch(*this, theBatchId, inSourcePaths, inImageTillLoaded, inListener, contextType));
        if (theBatch) {
            batches.insert(theBatchId, theBatch);
            return theBatchId;
        }
        return 0;
    }

    void cancelImageBatchLoading(TImageBatchId inBatchId) override
    {
        QSSGImageLoaderBatch *theBatch(getBatch(inBatchId));
        if (theBatch)
            theBatch->cancel();
    }

    // Blocks if the image is currently in-flight
    void cancelImageLoading(QString inSourcePath) override
    {
        QMutexLocker loaderLock(&loaderMutex);
        TSourcePathToBatchMap::iterator theIter = sourcePathToBatches.find(inSourcePath);
        if (theIter != sourcePathToBatches.end()) {
            TImageBatchId theBatchId = theIter.value();
            TImageLoaderBatchMap::iterator theBatchIter = batches.find(theBatchId);
            if (theBatchIter != batches.end())
                theBatchIter.value()->cancel(inSourcePath);
        }
    }

    QSSGImageLoaderBatch *getBatch(TImageBatchId inId)
    {
        QMutexLocker loaderLock(&loaderMutex);
        TImageLoaderBatchMap::iterator theIter = batches.find(inId);
        if (theIter != batches.end())
            return theIter.value();
        return nullptr;
    }

    void blockUntilLoaded(TImageBatchId inId) override
    {
        // TODO: This is not sane
        QMutexLocker locker(&loaderMutex);
        for (QSSGImageLoaderBatch *theBatch = getBatch(inId); theBatch; theBatch = getBatch(inId)) {
            // Only need to block if images aren't loaded.  Don't need to block if they aren't
            // finalized.
            if (!theBatch->isLoadingFinished()) {
                theBatch->loadEvent.wait(&loaderMutex, 200);
                //                theBatch->m_LoadEvent.reset(); ???
            }
            beginFrame();
        }
    }
    void imageLoaded(QSSGLoadingImage &inImage, QSSGLoadedTexture *inTexture)
    {
        QMutexLocker loaderLock(&loaderMutex);
        loadedImages.push_back(QSSGBatchLoadedImage(inImage.sourcePath, inTexture, *inImage.batch));
        inImage.batch->incrementLoadedImageCount();
        inImage.batch->loadEvent.wakeAll();
    }
    // These are called by the render context, users don't need to call this.
    void beginFrame() override
    {
        QMutexLocker loaderLock(&loaderMutex);
        // Pass 1 - send out all image loaded signals
        for (int idx = 0, end = loadedImages.size(); idx < end; ++idx) {

            sourcePathToBatches.remove(loadedImages[idx].sourcePath);
            loadedImages[idx].finalize(bufferManager);
            loadedImages[idx].batch->incrementFinalizedImageCount();
            if (loadedImages[idx].batch->isFinalizedFinished())
                finishedBatches.push_back(loadedImages[idx].batch->batchId);
        }
        loadedImages.clear();
        // pass 2 - clean up any existing batches.
        for (int idx = 0, end = finishedBatches.size(); idx < end; ++idx) {
            TImageLoaderBatchMap::iterator theIter = batches.find(finishedBatches[idx]);
            if (theIter != batches.end()) {
                QSSGImageLoaderBatch *theBatch = theIter.value();
                if (theBatch->loadListener)
                    theBatch->loadListener->OnImageBatchComplete(theBatch->batchId);
                batches.remove(finishedBatches[idx]);
                // TODO:
                theBatch->~QSSGImageLoaderBatch();
            }
        }
        finishedBatches.clear();
    }

    void endFrame() override {}
};

void QSSGLoadingImage::setup(QSSGImageLoaderBatch &inBatch)
{
    batch = &inBatch;
    taskId = inBatch.loader.threadPool->addTask(this, loadImage, taskCancelled);
}

void QSSGLoadingImage::loadImage(void *inImg)
{
    QSSGLoadingImage *theThis = reinterpret_cast<QSSGLoadingImage *>(inImg);
    //    SStackPerfTimer theTimer(theThis->m_Batch->m_Loader.m_PerfTimer, "Image Decompression");
    if (theThis->batch->loader.bufferManager->isImageLoaded(theThis->sourcePath) == false) {
        QSSGRef<QSSGLoadedTexture> theTexture = QSSGLoadedTexture::load(theThis->sourcePath,
                                                                              QSSGRenderTextureFormat::Unknown,
                                                                              *theThis->batch->loader.inputStreamFactory,
                                                                              true,
                                                                              theThis->batch->contextType);
        // if ( theTexture )
        //	theTexture->EnsureMultiplerOfFour( theThis->m_Batch->m_Loader.m_Foundation,
        // theThis->m_SourcePath.c_str() );

        theThis->batch->loader.imageLoaded(*theThis, theTexture.data());
    } else {
        theThis->batch->loader.imageLoaded(*theThis, nullptr);
    }
}

void QSSGLoadingImage::taskCancelled(void *inImg)
{
    QSSGLoadingImage *theThis = reinterpret_cast<QSSGLoadingImage *>(inImg);
    theThis->batch->loader.imageLoaded(*theThis, nullptr);
}

bool QSSGBatchLoadedImage::finalize(const QSSGRef<QSSGBufferManager> &inMgr)
{
    if (texture) {
        // PKC : We'll look at the path location to see if the image is in the standard
        // location for IBL light probes or a standard hdr format and decide to generate BSDF
        // miplevels (if the image doesn't have
        // mipmaps of its own that is).
        QString thepath(sourcePath);
        bool isIBL = (thepath.contains(QLatin1String(".hdr"))) || (thepath.contains(QLatin1String("\\IBL\\"))) ||
                     (thepath.contains(QLatin1String("/IBL/")));
        inMgr->loadRenderImage(sourcePath, texture, false, isIBL);
        inMgr->unaliasImagePath(sourcePath);
    }
    if (batch->loadListener)
        batch->loadListener->OnImageLoadComplete(sourcePath, texture ? ImageLoadResult::Succeeded : ImageLoadResult::Failed);

    if (texture) {
        // m_Texture->release();
        return true;
    }

    return false;
}

QSSGImageLoaderBatch *QSSGImageLoaderBatch::createLoaderBatch(QSSGBatchLoader &inLoader,
                                                              TImageBatchId inBatchId,
                                                              QSSGDataView<QString> inSourcePaths,
                                                              const QString &inImageTillLoaded,
                                                              IImageLoadListener *inListener,
                                                              QSSGRenderContextType contextType)
{
    TLoadingImageList theImages;
    quint32 theLoadingImageCount = 0;
    for (int idx = 0, end = inSourcePaths.size(); idx < end; ++idx) {
        QString theSourcePath(inSourcePaths[idx]);

        // TODO: What was the meaning of isValid() (now isEmpty())??
        if (theSourcePath.isEmpty() == false)
            continue;

        if (inLoader.bufferManager->isImageLoaded(theSourcePath))
            continue;

        const auto foundIt = inLoader.sourcePathToBatches.find(inSourcePaths[idx]);

        // TODO: This is a bit funky, check if we really need to update the inBatchId...
        inLoader.sourcePathToBatches.insert(inSourcePaths[idx], inBatchId);

        // If the loader has already seen this image.
        if (foundIt != inLoader.sourcePathToBatches.constEnd())
            continue;

        if (inImageTillLoaded.isEmpty()) {
            // Alias the image so any further requests for this source path will result in
            // the default images (image till loaded).
            bool aliasSuccess = inLoader.bufferManager->aliasImagePath(theSourcePath, inImageTillLoaded, true);
            (void)aliasSuccess;
            Q_ASSERT(aliasSuccess);
        }

        // TODO: Yeah... make sure this is cleaned up correctly.
        QSSGLoadingImage *sli = new QSSGLoadingImage(theSourcePath);
        theImages.push_front(*sli);
        ++theLoadingImageCount;
    }
    if (theImages.empty() == false) {
        QSSGImageLoaderBatch *theBatch = new QSSGImageLoaderBatch(inLoader, inListener, theImages, inBatchId, theLoadingImageCount, contextType);
        return theBatch;
    }
    return nullptr;
}

QSSGImageLoaderBatch::QSSGImageLoaderBatch(QSSGBatchLoader &inLoader,
                                               IImageLoadListener *inLoadListener,
                                               const TLoadingImageList &inImageList,
                                               TImageBatchId inBatchId,
                                               quint32 inImageCount,
                                               QSSGRenderContextType contextType)
    : loader(inLoader)
    , loadListener(inLoadListener)
    , images(inImageList)
    , batchId(inBatchId)
    , loadedOrCanceledImageCount(0)
    , finalizedImageCount(0)
    , numImages(inImageCount)
    , contextType(contextType)
{
    for (TLoadingImageList::iterator iter = images.begin(), end = images.end(); iter != end; ++iter) {
        iter->setup(*this);
    }
}

QSSGImageLoaderBatch::~QSSGImageLoaderBatch()
{
    auto iter = images.begin();
    const auto end = images.end();
    while (iter != end) {
        auto temp(iter);
        ++iter;
        delete temp.m_obj;
    }
}

void QSSGImageLoaderBatch::cancel()
{
    for (TLoadingImageList::iterator iter = images.begin(), end = images.end(); iter != end; ++iter)
        loader.threadPool->cancelTask(iter->taskId);
}

void QSSGImageLoaderBatch::cancel(const QString &inSourcePath)
{
    for (TLoadingImageList::iterator iter = images.begin(), end = images.end(); iter != end; ++iter) {
        if (iter->sourcePath == inSourcePath) {
            loader.threadPool->cancelTask(iter->taskId);
            break;
        }
    }
}
}

QSSGRef<IImageBatchLoader> IImageBatchLoader::createBatchLoader(const QSSGRef<QSSGInputStreamFactory> &inFactory,
                                                                const QSSGRef<QSSGBufferManager> &inBufferManager,
                                                                const QSSGRef<QSSGAbstractThreadPool> &inThreadPool,
                                                                QSSGPerfTimer *inTimer)
{
    return QSSGRef<IImageBatchLoader>(new QSSGBatchLoader(inFactory, inBufferManager, inThreadPool, inTimer));
}

QSSGBatchLoader::~QSSGBatchLoader()
{
    QVector<TImageBatchId> theCancelledBatches;
    auto theIter = batches.begin();
    const auto theEnd = batches.end();
    while (theIter != theEnd) {
        theIter.value()->cancel();
        theCancelledBatches.push_back(theIter.value()->batchId);
        ++theIter;
    }
    for (int idx = 0, end = theCancelledBatches.size(); idx < end; ++idx)
        blockUntilLoaded(theCancelledBatches[idx]);

    Q_ASSERT(batches.size() == 0);
}

QT_END_NAMESPACE
