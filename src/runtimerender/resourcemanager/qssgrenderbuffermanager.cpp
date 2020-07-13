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

#include "qssgrenderbuffermanager_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrenderprefiltertexture_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>
#include <QtQuick3DAssetImport/private/qssgmeshbvhbuilder_p.h>

#include <QtQuick/QSGTexture>

#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

QT_BEGIN_NAMESPACE

namespace {

struct PrimitiveEntry
{
    // Name of the primitive as it will be in the UIP file
    const char *primitive;
    // Name of the primitive file on the filesystem
    const char *file;
};

const int nPrimitives = 5;
const PrimitiveEntry primitives[nPrimitives] = {
        {"#Rectangle", "/Rectangle.mesh"},
        {"#Sphere","/Sphere.mesh"},
        {"#Cube","/Cube.mesh"},
        {"#Cone","/Cone.mesh"},
        {"#Cylinder","/Cylinder.mesh"},
};

const char *primitivesDirectory = "res//primitives";

}


QSSGBufferManager::QSSGBufferManager(const QSSGRef<QSSGRenderContext> &ctx,
                                         const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory,
                                         QSSGPerfTimer *inTimer)
{
    context = ctx;
    inputStreamFactory = inInputStreamFactory;
    perfTimer = inTimer;
    gpuSupportsDXT = ctx->supportsDXTImages();
}

QSSGBufferManager::~QSSGBufferManager()
{ clear(); }

void QSSGBufferManager::setImageHasTransparency(const QString &inImagePath, bool inHasTransparency)
{
    ImageMap::iterator theImage = imageMap.insert(inImagePath, QSSGRenderImageTextureData());
    theImage.value().m_textureFlags.setHasTransparency(inHasTransparency);
}

bool QSSGBufferManager::getImageHasTransparency(const QString &inSourcePath) const
{
    ImageMap::const_iterator theIter = imageMap.find(inSourcePath);
    if (theIter != imageMap.end())
        return theIter.value().m_textureFlags.hasTransparency();
    return false;
}

void QSSGBufferManager::setImageTransparencyToFalseIfNotSet(const QString &inSourcePath)
{
    ImageMap::iterator theImage = imageMap.find(inSourcePath);

    // If we did actually insert something
    if (theImage != imageMap.end())
        theImage.value().m_textureFlags.setHasTransparency(false);
}

void QSSGBufferManager::setInvertImageUVCoords(const QString &inImagePath, bool inShouldInvertCoords)
{
    ImageMap::iterator theImage = imageMap.find(inImagePath);
    if (theImage != imageMap.end())
        theImage.value().m_textureFlags.setInvertUVCoords(inShouldInvertCoords);
}

bool QSSGBufferManager::isImageLoaded(const QString &inSourcePath)
{
    QMutexLocker locker(&loadedImageSetMutex);
    return loadedImageSet.find(inSourcePath) != loadedImageSet.end();
}

bool QSSGBufferManager::aliasImagePath(const QString &inSourcePath,
                                       const QString &inAliasPath,
                                       bool inIgnoreIfLoaded)
{
    if (inSourcePath.isEmpty() || inAliasPath.isEmpty())
        return false;
    // If the image is loaded then we ignore this call in some cases.
    if (inIgnoreIfLoaded && isImageLoaded(inSourcePath))
        return false;
    aliasImageMap.insert(inSourcePath, inAliasPath);
    return true;
}

void QSSGBufferManager::unaliasImagePath(const QString &inSourcePath)
{
    aliasImageMap.remove(inSourcePath);
}

QString QSSGBufferManager::getImagePath(const QString &inSourcePath) const
{
    const auto foundIt = aliasImageMap.constFind(inSourcePath);
    return (foundIt != aliasImageMap.cend()) ? foundIt.value() : inSourcePath;
}

namespace {
QSize sizeForMipLevel(int mipLevel, const QSize &baseLevelSize)
{
    const int w = qMax(1, baseLevelSize.width() >> mipLevel);
    const int h = qMax(1, baseLevelSize.height() >> mipLevel);
    return QSize(w, h);
}
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(const QString &inImagePath, const QSSGRef<QSSGLoadedTexture> &inLoadedImage, bool inForceScanForTransparency, bool inBsdfMipmaps)
{
    //        SStackPerfTimer __perfTimer(perfTimer, "Image Upload");
    {
        QMutexLocker mapLocker(&loadedImageSetMutex);
        loadedImageSet.insert(inImagePath);
    }
    ImageMap::iterator theImage = imageMap.find(inImagePath);
    bool wasInserted = theImage == imageMap.end();
    if (wasInserted)
        theImage = imageMap.insert(inImagePath, QSSGRenderImageTextureData());

    // inLoadedImage.EnsureMultiplerOfFour( context->GetFoundation(), inImagePath.c_str() );

    QSSGRef<QSSGRenderTexture2D> theTexture = new QSSGRenderTexture2D(context);
    if (inLoadedImage->data) {
        QSSGRenderTextureFormat destFormat = inLoadedImage->format;
        if (inBsdfMipmaps) {
            if (inLoadedImage->format != QSSGRenderTextureFormat::RGBE8) {
                if (context->renderContextType() == QSSGRenderContextType::GLES2)
                    destFormat = QSSGRenderTextureFormat::RGBA8;
                else
                    destFormat = QSSGRenderTextureFormat::RGBA16F;
            }
        } else {
            theTexture->setTextureData(QSSGByteView((quint8 *)inLoadedImage->data, inLoadedImage->dataSizeInBytes),
                                       0,
                                       inLoadedImage->width,
                                       inLoadedImage->height,
                                       inLoadedImage->format,
                                       destFormat);
        }

        if (inBsdfMipmaps && inLoadedImage->format.isUncompressedTextureFormat()) {
            theTexture->setMinFilter(QSSGRenderTextureMinifyingOp::LinearMipmapLinear);
            QSSGRef<QSSGRenderPrefilterTexture> theBSDFMipMap = theImage.value().m_bsdfMipMap;
            if (theBSDFMipMap == nullptr) {
                theBSDFMipMap = QSSGRenderPrefilterTexture::create(context, inLoadedImage->width, inLoadedImage->height, theTexture, destFormat);
                theImage.value().m_bsdfMipMap = theBSDFMipMap;
            }

            if (theBSDFMipMap) {
                theBSDFMipMap->build(inLoadedImage->data, inLoadedImage->dataSizeInBytes, inLoadedImage->format);
            }
        }
    } else if (inLoadedImage->compressedData.isValid()) {
        // Compressed Texture Image handling using QTextureFileData
        for (int i = 0; i < inLoadedImage->compressedData.numLevels(); i++) {
            QSize imageSize = sizeForMipLevel(i, inLoadedImage->compressedData.size());
            auto format = GLConversion::fromGLtoTextureFormat(inLoadedImage->compressedData.glInternalFormat());
            theTexture->setTextureData(QSSGByteView(reinterpret_cast<quint8 *>(inLoadedImage->compressedData.data().data() + inLoadedImage->compressedData.dataOffset(i)), inLoadedImage->compressedData.dataLength(i)),
                                       i, imageSize.width(), imageSize.height(), format);
        }
    }



    /*else if (inLoadedImage->dds) {
            theImage.first->second.m_Texture = theTexture;
            bool supportsDXT = GPUSupportsDXT;
            bool isDXT = QSSGRenderTextureFormat::isCompressedTextureFormat(inLoadedImage.format);
            bool requiresDecompression = (supportsDXT == false && isDXT) || false;
            // test code for DXT decompression
            // if ( isDXT ) requiresDecompression = true;
            if (requiresDecompression) {
                qCWarning(WARNING, PERF_INFO,
                          "Image %s is DXT format which is unsupported by "
                          "the graphics subsystem, decompressing in CPU",
                          inImagePath.c_str());
            }
            STextureData theDecompressedImage;
            for (int idx = 0; idx < inLoadedImage.dds->numMipmaps; ++idx) {
                if (inLoadedImage.dds->mipwidth[idx] && inLoadedImage.dds->mipheight[idx]) {
                    if (requiresDecompression == false) {
                        theTexture->SetTextureData(
                                    toU8DataRef((char *)inLoadedImage.dds->data[idx],
                                                (quint32)inLoadedImage.dds->size[idx]),
                                    (quint8)idx, (quint32)inLoadedImage.dds->mipwidth[idx],
                                    (quint32)inLoadedImage.dds->mipheight[idx], inLoadedImage.format);
                    } else {
                        theDecompressedImage =
                                inLoadedImage.DecompressDXTImage(idx, &theDecompressedImage);

                        if (theDecompressedImage.data) {
                            theTexture->SetTextureData(
                                        toU8DataRef((char *)theDecompressedImage.data,
                                                    (quint32)theDecompressedImage.dataSizeInBytes),
                                        (quint8)idx, (quint32)inLoadedImage.dds->mipwidth[idx],
                                        (quint32)inLoadedImage.dds->mipheight[idx],
                                        theDecompressedImage.format);
                        }
                    }
                }
            }
            if (theDecompressedImage.data)
                inLoadedImage.ReleaseDecompressedTexture(theDecompressedImage);
        }*/
    if (wasInserted == true || inForceScanForTransparency)
        theImage.value().m_textureFlags.setHasTransparency(inLoadedImage->scanForTransparency());
    theImage.value().m_texture = theTexture;
    return theImage.value();
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(const QString &inImagePath, const QSSGRenderTextureFormat &inFormat, bool inForceScanForTransparency, bool inBsdfMipmaps)
{
    const QString realImagePath = getImagePath(inImagePath);

    if (Q_UNLIKELY(realImagePath.isNull()))
        return QSSGRenderImageTextureData();

    const auto foundIt = imageMap.constFind(realImagePath);
    if (foundIt != imageMap.cend())
        return foundIt.value();

    if (Q_LIKELY(!realImagePath.isNull())) {
        QSSGRef<QSSGLoadedTexture> theLoadedImage;
        {
            //                SStackPerfTimer __perfTimer(perfTimer, "Image Decompression");
            theLoadedImage = QSSGLoadedTexture::load(realImagePath, inFormat, *inputStreamFactory, true, context->renderContextType());
            // Hackish solution to custom materials not finding their textures if they are used
            // in sub-presentations. Note: Runtime 1 is going to be removed in Qt 3D Studio 2.x,
            // so this should be ok.
            if (!theLoadedImage) {
                if (QDir(realImagePath).isRelative()) {
                    QString searchPath = realImagePath;
                    if (searchPath.startsWith(QLatin1String("./")))
                        searchPath.prepend(QLatin1String("."));
                    int loops = 0;
                    while (!theLoadedImage && ++loops <= 3) {
                        theLoadedImage = QSSGLoadedTexture::load(searchPath,
                                                                   inFormat,
                                                                   *inputStreamFactory,
                                                                   true,
                                                                   context->renderContextType());
                        searchPath.prepend(QLatin1String("../"));
                    }
                } else {
                    // Some textures, for example environment maps for custom materials,
                    // have absolute path at this point. It points to the wrong place with
                    // the new project structure, so we need to split it up and construct
                    // the new absolute path here.
                    const QString &wholePath = realImagePath;
                    QStringList splitPath = wholePath.split(QLatin1String("../"));
                    if (splitPath.size() > 1) {
                        QString searchPath = splitPath.at(0) + splitPath.at(1);
                        int loops = 0;
                        while (!theLoadedImage && ++loops <= 3) {
                            theLoadedImage = QSSGLoadedTexture::load(searchPath,
                                                                       inFormat,
                                                                       *inputStreamFactory,
                                                                       true,
                                                                       context->renderContextType());
                            searchPath = splitPath.at(0);
                            for (int i = 0; i < loops; i++)
                                searchPath.append(QLatin1String("../"));
                            searchPath.append(splitPath.at(1));
                        }
                    }
                }
            }
        }

        if (Q_LIKELY(theLoadedImage))
            return loadRenderImage(realImagePath, theLoadedImage, inForceScanForTransparency, inBsdfMipmaps);

        // We want to make sure that bad path fails once and doesn't fail over and over
        // again
        // which could slow down the system quite a bit.
        imageMap.insert(realImagePath, QSSGRenderImageTextureData());
        qCWarning(WARNING, "Failed to load image: %s", qPrintable(realImagePath));
    }

    return QSSGRenderImageTextureData();
}

QSSGRenderImageTextureData QSSGBufferManager::loadRenderImage(QSGTexture *qsgTexture)
{
    if (Q_UNLIKELY(!qsgTexture))
        return QSSGRenderImageTextureData();

    auto theImage = qsgImageMap.find(qsgTexture);

    if (theImage == qsgImageMap.end()) {
        theImage = qsgImageMap.insert(qsgTexture, QSSGRenderImageTextureData());
        QSSGRef<QSSGRenderTexture2D> theTexture = new QSSGRenderTexture2D(context, qsgTexture);
        theImage.value().m_texture = theTexture;
        QObject::connect(qsgTexture, &QObject::destroyed, [this, qsgTexture]() {
            qsgImageMap.remove(qsgTexture);
        });
    } else {
        //TODO: make QSSGRenderTexture2D support updating handles instead of this hack
        auto textureId = reinterpret_cast<QSSGRenderBackend::QSSGRenderBackendTextureObject>(quintptr(qsgTexture->textureId()));
        if (theImage.value().m_texture->handle() != textureId) {
            QSSGRef<QSSGRenderTexture2D> theTexture = new QSSGRenderTexture2D(context, qsgTexture);
            theImage.value().m_texture = theTexture;
        }
    }

    return theImage.value();
}

QSSGMeshUtilities::MultiLoadResult QSSGBufferManager::loadPrimitive(const QString &inRelativePath) const
{
    QByteArray theName = inRelativePath.toUtf8();
    for (size_t idx = 0; idx < nPrimitives; ++idx) {
        if (primitives[idx].primitive == theName) {
            QString pathBuilder = QString::fromLatin1(primitivesDirectory);
            pathBuilder += QLatin1String(primitives[idx].file);
            quint32 id = 1;
            QSharedPointer<QIODevice> theInStream(inputStreamFactory->getStreamForFile(pathBuilder));
            if (theInStream)
                return QSSGMeshUtilities::Mesh::loadMulti(*theInStream, id);

            qCCritical(INTERNAL_ERROR, "Unable to find mesh primitive %s", qPrintable(pathBuilder));
            return QSSGMeshUtilities::MultiLoadResult();
        }
    }
    return QSSGMeshUtilities::MultiLoadResult();
}

QVector<QVector3D> QSSGBufferManager::createPackedPositionDataArray(
        const QSSGMeshUtilities::MultiLoadResult &inResult) const
{
    // we assume a position consists of 3 floats
    const auto mesh = inResult.m_mesh;
    qint32 vertexCount = mesh->m_vertexBuffer.m_data.size() / mesh->m_vertexBuffer.m_stride;
    QVector<QVector3D> positions(vertexCount);
    quint8 *baseOffset = reinterpret_cast<quint8 *>(mesh);

    // copy position data
    float *srcData = reinterpret_cast<float *>(mesh->m_vertexBuffer.m_data.begin(baseOffset));
    quint32 srcStride = mesh->m_vertexBuffer.m_stride / sizeof(float);
    QVector3D *p = positions.data();

    for (qint32 i = 0; i < vertexCount; ++i) {
        p[i] = QVector3D(srcData[0], srcData[1], srcData[2]);
        srcData += srcStride;
    }

    return positions;
}

QSSGRenderMesh *QSSGBufferManager::getMesh(const QSSGRenderMeshPath &inSourcePath) const
{
    if (inSourcePath.isNull())
        return nullptr;

    const auto foundIt = meshMap.constFind(inSourcePath);
    return (foundIt != meshMap.constEnd()) ? *foundIt : nullptr;
}

QSSGRenderMesh *QSSGBufferManager::createRenderMesh(
        const QSSGMeshUtilities::MultiLoadResult &result, const QSSGRenderMeshPath &inSourcePath)
{
    QSSGRenderMesh *newMesh = new QSSGRenderMesh(result.m_mesh->m_drawMode,
                                                 result.m_mesh->m_winding,
                                                 result.m_id);
    quint8 *baseAddress = reinterpret_cast<quint8 *>(result.m_mesh);
    meshMap.insert(QSSGRenderMeshPath::create(inSourcePath.path), newMesh);
    QSSGByteView vertexBufferData(result.m_mesh->m_vertexBuffer.m_data.begin(baseAddress),
                                                result.m_mesh->m_vertexBuffer.m_data.size());

    QSSGRef<QSSGRenderVertexBuffer>
            vertexBuffer = new QSSGRenderVertexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                         result.m_mesh->m_vertexBuffer.m_stride,
                                                         vertexBufferData);

    // create a tight packed position data VBO
    // this should improve our depth pre pass rendering
    QSSGRef<QSSGRenderVertexBuffer> posVertexBuffer;
    QVector<QVector3D> posData = createPackedPositionDataArray(result);
    if (posData.size())
        posVertexBuffer = new QSSGRenderVertexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                        3 * sizeof(float),
                                                        toByteView(posData));

    QSSGRef<QSSGRenderIndexBuffer> indexBuffer;
    if (result.m_mesh->m_indexBuffer.m_data.size()) {
        QSSGRenderComponentType bufComponentType = result.m_mesh->m_indexBuffer.m_componentType;
        quint32 sizeofType = getSizeOfType(bufComponentType);

        if (sizeofType == 2 || sizeofType == 4) {
            // Ensure type is unsigned; else things will fail in rendering pipeline.
            if (bufComponentType == QSSGRenderComponentType::Integer16)
                bufComponentType = QSSGRenderComponentType::UnsignedInteger16;
            if (bufComponentType == QSSGRenderComponentType::Integer32)
                bufComponentType = QSSGRenderComponentType::UnsignedInteger32;

            QSSGByteView indexBufferData(result.m_mesh->m_indexBuffer.m_data.begin(baseAddress),
                                                       result.m_mesh->m_indexBuffer.m_data.size());
            indexBuffer = new QSSGRenderIndexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                       bufComponentType,
                                                       indexBufferData);
        } else {
            Q_ASSERT(false);
        }
    }
    const auto &entries = result.m_mesh->m_vertexBuffer.m_entries;
    entryBuffer.resize(entries.size());
    for (quint32 entryIdx = 0, entryEnd = entries.size(); entryIdx < entryEnd; ++entryIdx) {
        entryBuffer[entryIdx] = entries.index(baseAddress, entryIdx).toVertexBufferEntry(baseAddress);

        newMesh->inputLayoutInputNames.append(QByteArray(entryBuffer[entryIdx].m_name));
    }

    // create our attribute layout
    auto attribLayout = context->createAttributeLayout(toDataView(entryBuffer.constData(), entryBuffer.count()));
    // create our attribute layout for depth pass
    QSSGRenderVertexBufferEntry vertBufferEntries[] = {
        QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
    };
    auto attribLayoutDepth = context->createAttributeLayout(toDataView(vertBufferEntries, 1));

    // create input assembler object
    quint32 strides = result.m_mesh->m_vertexBuffer.m_stride;
    quint32 offsets = 0;
    auto inputAssembler = context->createInputAssembler(attribLayout,
                                                        toDataView(&vertexBuffer, 1),
                                                        indexBuffer,
                                                        toDataView(&strides, 1),
                                                        toDataView(&offsets, 1),
                                                        result.m_mesh->m_drawMode);

    // create depth input assembler object
    quint32 posStrides = (posVertexBuffer) ? 3 * sizeof(float) : strides;
    auto inputAssemblerDepth = context->createInputAssembler(
                attribLayoutDepth,
                toDataView((posVertexBuffer) ? &posVertexBuffer : &vertexBuffer, 1),
                indexBuffer, toDataView(&posStrides, 1), toDataView(&offsets, 1),
                result.m_mesh->m_drawMode);

    auto inputAssemblerPoints = context->createInputAssembler(
                attribLayoutDepth,
                toDataView((posVertexBuffer) ? &posVertexBuffer : &vertexBuffer, 1),
                nullptr, toDataView(&posStrides, 1), toDataView(&offsets, 1),
                QSSGRenderDrawMode::Points);

    if (!inputAssembler || !inputAssemblerDepth || !inputAssemblerPoints) {
        Q_ASSERT(false);
        return nullptr;
    }
    newMesh->joints.resize(result.m_mesh->m_joints.size());
    for (quint32 jointIdx = 0, jointEnd = result.m_mesh->m_joints.size(); jointIdx < jointEnd; ++jointIdx) {
        const QSSGMeshUtilities::Joint &importJoint(result.m_mesh->m_joints.index(baseAddress, jointIdx));
        QSSGRenderJoint &newJoint(newMesh->joints[jointIdx]);
        newJoint.jointID = importJoint.m_jointID;
        newJoint.parentID = importJoint.m_parentID;
        ::memcpy(newJoint.invBindPose, importJoint.m_invBindPose, 16 * sizeof(float));
        ::memcpy(newJoint.localToGlobalBoneSpace, importJoint.m_localToGlobalBoneSpace, 16 * sizeof(float));
    }

    for (quint32 subsetIdx = 0, subsetEnd = result.m_mesh->m_subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
        QSSGRenderSubset subset;
        const QSSGMeshUtilities::MeshSubset &source(result.m_mesh->m_subsets.index(baseAddress, subsetIdx));
        subset.bounds = source.m_bounds;
        subset.bvhRoot = nullptr;
        subset.count = source.m_count;
        subset.offset = source.m_offset;
        subset.joints = newMesh->joints;
        subset.name = QString::fromUtf16(reinterpret_cast<const char16_t *>(source.m_name.begin(baseAddress)));
        subset.vertexBuffer = vertexBuffer;
        if (posVertexBuffer)
            subset.posVertexBuffer = posVertexBuffer;
        if (indexBuffer)
            subset.indexBuffer = indexBuffer;
        subset.inputAssembler = inputAssembler;
        subset.inputAssemblerDepth = inputAssemblerDepth;
        subset.inputAssemblerPoints = inputAssemblerPoints;
        subset.primitiveType = result.m_mesh->m_drawMode;
        newMesh->subsets.push_back(subset);
    }
    // If we want to, we can an in a quite stupid way break up modes into sub-subsets.
    // These are assumed to use the same material as the outer subset but have fewer tris
    // and should have a more exact bounding box.  This sort of thing helps with using the frustum
    // culling system but it is really done incorrectly.  It should be done via some sort of
    // oct-tree mechanism and it so that the sub-subsets spatially sorted and it should only be done
    // upon save-to-binary with the results saved out to disk.  As you can see, doing it properly
    // requires some real engineering effort so it is somewhat unlikely it will ever happen.
    // Or it could be done on import if someone really wants to change the mesh buffer format.
    // Either way it isn't going to happen here and it isn't going to happen this way but this
    // is a working example of using the technique.
#ifdef QSSG_RENDER_GENERATE_SUB_SUBSETS
    QSSGOption<QSSGRenderVertexBufferEntry> thePosAttrOpt = theVertexBuffer->getEntryByName("attr_pos");
    bool hasPosAttr = thePosAttrOpt.hasValue() && thePosAttrOpt->m_componentType == QSSGRenderComponentTypes::Float32
            && thePosAttrOpt->m_numComponents == 3;

    for (size_t subsetIdx = 0, subsetEnd = theNewMesh->subsets.size(); subsetIdx < subsetEnd; ++subsetIdx) {
        QSSGRenderSubset &theOuterSubset = theNewMesh->subsets[subsetIdx];
        if (theOuterSubset.count && theIndexBuffer
                && theIndexBuffer->getComponentType() == QSSGRenderComponentTypes::UnsignedInteger16
                && theNewMesh->drawMode == QSSGRenderDrawMode::Triangles && hasPosAttr) {
            // Num tris in a sub subset.
            quint32 theSubsetSize = 3334 * 3; // divisible by three.
            size_t theNumSubSubsets = ((theOuterSubset.count - 1) / theSubsetSize) + 1;
            quint32 thePosAttrOffset = thePosAttrOpt->m_firstItemOffset;
            const quint8 *theVertData = theResult.m_mesh->m_vertexBuffer.m_data.begin();
            const quint8 *theIdxData = theResult.m_mesh->m_indexBuffer.m_data.begin();
            quint32 theVertStride = theResult.m_mesh->m_vertexBuffer.m_stride;
            quint32 theOffset = theOuterSubset.offset;
            quint32 theCount = theOuterSubset.count;
            for (size_t subSubsetIdx = 0, subSubsetEnd = theNumSubSubsets; subSubsetIdx < subSubsetEnd; ++subSubsetIdx) {
                QSSGRenderSubsetBase theBase;
                theBase.offset = theOffset;
                theBase.count = NVMin(theSubsetSize, theCount);
                theBase.bounds.setEmpty();
                theCount -= theBase.count;
                theOffset += theBase.count;
                // Create new bounds.
                // Offset is in item size, not bytes.
                const quint16 *theSubsetIdxData = reinterpret_cast<const quint16 *>(theIdxData + theBase.m_Offset * 2);
                for (size_t theIdxIdx = 0, theIdxEnd = theBase.m_Count; theIdxIdx < theIdxEnd; ++theIdxIdx) {
                    quint32 theVertOffset = theSubsetIdxData[theIdxIdx] * theVertStride;
                    theVertOffset += thePosAttrOffset;
                    QVector3D thePos = *(reinterpret_cast<const QVector3D *>(theVertData + theVertOffset));
                    theBase.bounds.include(thePos);
                }
                theOuterSubset.subSubsets.push_back(theBase);
            }
        } else {
            QSSGRenderSubsetBase theBase;
            theBase.bounds = theOuterSubset.bounds;
            theBase.count = theOuterSubset.count;
            theBase.offset = theOuterSubset.offset;
            theOuterSubset.subSubsets.push_back(theBase);
        }
    }
#endif
    return newMesh;
}

QSSGRenderMesh *QSSGBufferManager::loadMesh(const QSSGRenderMeshPath &inMeshPath)
{
    if (inMeshPath.isNull())
        return nullptr;

    // check if it is already loaded
    MeshMap::iterator meshItr = meshMap.find(inMeshPath);
    if (meshItr != meshMap.end())
        return meshItr.value();

    // loading new mesh
    QSSGMeshUtilities::MultiLoadResult result = loadMeshData(inMeshPath);

    if (result.m_mesh == nullptr) {
        qCWarning(WARNING, "Failed to load mesh: %s", qPrintable(inMeshPath.path));
        return nullptr;
    }

    auto ret = createRenderMesh(result, inMeshPath);
    ::free(result.m_mesh);
    return ret;
}

QSSGRenderMesh *QSSGBufferManager::loadCustomMesh(const QSSGRenderMeshPath &inSourcePath,
                                                  QSSGMeshUtilities::Mesh *mesh, bool update)
{
    if (!inSourcePath.isNull() && mesh) {
        MeshMap::iterator meshItr = meshMap.find(inSourcePath);
        // Only create the mesh if it doesn't yet exist or update is true
        if (meshItr == meshMap.end() || update) {
            if (meshItr != meshMap.end()) {
                releaseMesh(*meshItr.value());
                meshMap.erase(meshItr);
            }
            QSSGMeshUtilities::MultiLoadResult result;
            result.m_mesh = mesh;
            auto ret = createRenderMesh(result, inSourcePath);
            return ret;
        }
    }
    return nullptr;
}

QSSGMeshBVH *QSSGBufferManager::loadMeshBVH(const QSSGRenderMeshPath &inSourcePath)
{
    // loading new mesh
    QSSGMeshUtilities::MultiLoadResult result = loadMeshData(inSourcePath);

    if (result.m_mesh == nullptr) {
        qCWarning(WARNING, "Failed to load mesh: %s", qPrintable(inSourcePath.path));
        return nullptr;
    }

    // Build BVH for Mesh
    QSSGMeshBVHBuilder meshBVHBuilder(result.m_mesh);
    auto bvh = meshBVHBuilder.buildTree();

    ::free(result.m_mesh);
    return bvh;
}

QSSGMeshUtilities::MultiLoadResult QSSGBufferManager::loadMeshData(const QSSGRenderMeshPath &inMeshPath) const
{
    // loading new mesh
    QSSGMeshUtilities::MultiLoadResult result;

    // check to see if this is a primitive mesh
    if (inMeshPath.path.startsWith('#'))
        result = loadPrimitive(inMeshPath.path);

    // Attempt a load from the filesystem if this mesh isn't a primitive.
    if (result.m_mesh == nullptr) {
        QString pathBuilder = inMeshPath.path;
        int poundIndex = pathBuilder.lastIndexOf('#');
        int id = 0;
        if (poundIndex != -1) {
            id = pathBuilder.midRef(poundIndex + 1).toInt();
            pathBuilder = pathBuilder.left(poundIndex);
        }
        if (!pathBuilder.isEmpty()) {
            QSharedPointer<QIODevice> ioStream(inputStreamFactory->getStreamForFile(pathBuilder));
            if (ioStream)
                result = QSSGMeshUtilities::Mesh::loadMulti(*ioStream, id);
        }
    }
    return result;
}

QSSGRenderMesh *QSSGBufferManager::createMesh(const QString &inSourcePath, quint8 *inVertData, quint32 inNumVerts, quint32 inVertStride, quint32 *inIndexData, quint32 inIndexCount, QSSGBounds3 inBounds)
{
    QString sourcePath = inSourcePath;

    // QPair<QString, SRenderMesh*> thePair(sourcePath, (SRenderMesh*)nullptr);
    // Make sure there isn't already a buffer entry for this mesh.
    const auto meshPath = QSSGRenderMeshPath::create(sourcePath);
    auto it = meshMap.find(meshPath);
    const auto end = meshMap.end();

    QPair<MeshMap::iterator, bool> theMesh;
    if (it != end)
        theMesh = { it, true };
    else
        theMesh = { meshMap.insert(meshPath, nullptr), false };

    if (theMesh.second == true) {
        QSSGRenderMesh *theNewMesh = new QSSGRenderMesh(QSSGRenderDrawMode::Triangles, QSSGRenderWinding::CounterClockwise, 0);

        // If we failed to create the RenderMesh, return a failure.
        if (!theNewMesh) {
            Q_ASSERT(false);
            return nullptr;
        }

        // Get rid of any old mesh that was sitting here and fill it with a new one.
        // NOTE : This is assuming that the source of our mesh data doesn't do its own memory
        // management and always returns new buffer pointers every time.
        // Don't know for sure if that's what we'll get from our intended sources, but that's
        // easily
        // adjustable by looking for matching pointers in the Subsets.
        if (theNewMesh && theMesh.first.value() != nullptr) {
            delete theMesh.first.value();
        }

        theMesh.first.value() = theNewMesh;
        quint32 vertDataSize = inNumVerts * inVertStride;
        Q_ASSERT(vertDataSize <= INT32_MAX); // TODO:
        QSSGByteView theVBufData(inVertData, qint32(vertDataSize));
        // QSSGConstDataRef<quint8> theVBufData( theResult.Mesh->VertexBuffer.Data.begin(
        // baseAddress )
        //		, theResult.Mesh->VertexBuffer.Data.size() );

        QSSGRef<QSSGRenderVertexBuffer> theVertexBuffer = new QSSGRenderVertexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                                                            inVertStride,
                                                                                            theVBufData);
        QSSGRef<QSSGRenderIndexBuffer> theIndexBuffer = nullptr;
        if (inIndexData != nullptr && inIndexCount > 3) {
            const quint32 inSize = inIndexCount * sizeof(quint32);
            Q_ASSERT(inSize <= INT32_MAX);
            Q_ASSERT(*inIndexData <= INT8_MAX);
            QSSGByteView theIBufData(reinterpret_cast<quint8 *>(inIndexData), qint32(inSize));
            theIndexBuffer = new QSSGRenderIndexBuffer(context, QSSGRenderBufferUsageType::Static,
                                                          QSSGRenderComponentType::UnsignedInteger32,
                                                          theIBufData);
        }

        // WARNING
        // Making an assumption here about the contents of the stream
        // PKC TODO : We may have to consider some other format.
        QSSGRenderVertexBufferEntry theEntries[] = {
            QSSGRenderVertexBufferEntry("attr_pos", QSSGRenderComponentType::Float32, 3),
            QSSGRenderVertexBufferEntry("attr_uv", QSSGRenderComponentType::Float32, 2, 12),
            QSSGRenderVertexBufferEntry("attr_norm", QSSGRenderComponentType::Float32, 3, 18),
        };

        // create our attribute layout
        QSSGRef<QSSGRenderAttribLayout> theAttribLayout = context->createAttributeLayout(toDataView(theEntries, 3));
        /*
            // create our attribute layout for depth pass
            QSSGRenderVertexBufferEntry theEntriesDepth[] = {
                    QSSGRenderVertexBufferEntry( "attr_pos",
            QSSGRenderComponentTypes::float, 3 ),
            };
            QSSGRenderAttribLayout* theAttribLayoutDepth = context->CreateAttributeLayout(
            toConstDataRef( theEntriesDepth, 1 ) );
            */
        // create input assembler object
        quint32 strides = inVertStride;
        quint32 offsets = 0;
        QSSGRef<QSSGRenderInputAssembler> theInputAssembler = context->createInputAssembler(theAttribLayout,
                                                                                                  toDataView(&theVertexBuffer, 1),
                                                                                                  theIndexBuffer,
                                                                                                  toDataView(&strides, 1),
                                                                                                  toDataView(&offsets, 1),
                                                                                                  QSSGRenderDrawMode::Triangles);

        if (!theInputAssembler) {
            Q_ASSERT(false);
            return nullptr;
        }

        // Pull out just the mesh object name from the total path
        const QString &fullName = inSourcePath;
        QString subName(inSourcePath);

        int indexOfSub = fullName.lastIndexOf('#');
        if (indexOfSub != -1) {
            subName = fullName.right(indexOfSub + 1);
        }

        theNewMesh->joints.clear();
        QSSGRenderSubset theSubset;
        theSubset.bounds = inBounds;
        theSubset.count = inIndexCount;
        theSubset.offset = 0;
        theSubset.joints = theNewMesh->joints;
        theSubset.name = subName;
        theSubset.vertexBuffer = theVertexBuffer;
        theSubset.posVertexBuffer = nullptr;
        theSubset.indexBuffer = theIndexBuffer;
        theSubset.inputAssembler = theInputAssembler;
        theSubset.inputAssemblerDepth = theInputAssembler;
        theSubset.inputAssemblerPoints = theInputAssembler;
        theSubset.primitiveType = QSSGRenderDrawMode::Triangles;
        theNewMesh->subsets.push_back(theSubset);
    }

    return theMesh.first.value();
}

void QSSGBufferManager::releaseMesh(QSSGRenderMesh &inMesh)
{
    delete &inMesh;
}

void QSSGBufferManager::releaseTexture(QSSGRenderImageTextureData &inEntry)
{
    // TODO:
    Q_UNUSED(inEntry);
    // if (inEntry.Texture)
    //     inEntry.Texture->release();
}

void QSSGBufferManager::clear()
{
    for (auto iter = meshMap.begin(), end = meshMap.end(); iter != end; ++iter) {
        QSSGRenderMesh *theMesh = iter.value();
        if (theMesh)
            QSSGBufferManager::releaseMesh(*theMesh);
    }
    meshMap.clear();
    for (auto iter = imageMap.begin(), end = imageMap.end(); iter != end; ++iter) {
        QSSGRenderImageTextureData &theEntry = iter.value();
        QSSGBufferManager::releaseTexture(theEntry);
    }
    imageMap.clear();
    aliasImageMap.clear();
    {
        QMutexLocker locker(&loadedImageSetMutex);
        loadedImageSet.clear();
    }
}

void QSSGBufferManager::invalidateBuffer(const QString &inSourcePath)
{
    {
        // TODO:
        const auto meshPath = QSSGRenderMeshPath::create(inSourcePath);
        const auto iter = meshMap.constFind(meshPath);
        if (iter != meshMap.cend()) {
            if (iter.value())
                releaseMesh(*iter.value());
            meshMap.erase(iter);
            return;
        }
    }
    {
        ImageMap::iterator iter = imageMap.find(inSourcePath);
        if (iter != imageMap.end()) {
            QSSGRenderImageTextureData &theEntry = iter.value();
            releaseTexture(theEntry);
            imageMap.remove(inSourcePath);
            {
                QMutexLocker locker(&loadedImageSetMutex);
                loadedImageSet.remove(inSourcePath);
            }
        }
    }
}

QT_END_NAMESPACE
