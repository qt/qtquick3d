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

#include "qssgrendereffectsystem_p.h"
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include "qssgrenderinputstreamfactory_p.h"
//#include <QtQuick3DRuntimeRender/private/qssgrenderstring.h>
#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderresourcemanager_p.h>
#include "qssgrenderdynamicobjectsystemcommands_p.h"
#include <QtQuick3DRender/private/qssgrenderframebuffer_p.h>
#include <QtQuick3DRender/private/qssgrendershaderconstant_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgoffscreenrendermanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderprefiltertexture_p.h>
#include "qssgoffscreenrenderkey_p.h"
#include "qssgrenderdynamicobjectsystemutil_p.h"

QT_BEGIN_NAMESPACE

using namespace dynamic;

// None of this code will work if the size of void* changes because that would mean that
// the alignment of some of the objects isn't 4 bytes but would be 8 bytes.

/*
                ApplyBufferValue,
                //Apply the depth buffer as an input texture.
                ApplyDepthValue,
                Render, //Render to current FBO
                */

struct QSSGAllocatedBufferEntry
{
    QAtomicInt ref;
    QByteArray name;
    QSSGRef<QSSGRenderFrameBuffer> frameBuffer;
    QSSGRef<QSSGRenderTexture2D> texture;
    QSSGAllocateBufferFlags flags;
    bool needsClear;

    QSSGAllocatedBufferEntry(const QByteArray &inName, QSSGRenderFrameBuffer &inFb, QSSGRenderTexture2D &inTexture, QSSGAllocateBufferFlags inFlags)
        : name(inName), frameBuffer(&inFb), texture(&inTexture), flags(inFlags), needsClear(true)
    {
    }
    QSSGAllocatedBufferEntry() = default;
};

struct QSSGAllocatedImageEntry
{
    QAtomicInt ref;
    QByteArray name;
    QSSGRef<QSSGRenderImage2D> image;
    QSSGRef<QSSGRenderTexture2D> texture;
    QSSGAllocateBufferFlags flags;

    QSSGAllocatedImageEntry(const QByteArray &inName, QSSGRenderImage2D &inImage, QSSGRenderTexture2D &inTexture, QSSGAllocateBufferFlags inFlags)
        : name(inName), image(&inImage), texture(&inTexture), flags(inFlags)
    {
    }
    QSSGAllocatedImageEntry() = default;
};

struct QSSGImageEntry
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> shader;
    QSSGRenderCachedShaderProperty<QSSGRenderImage2D *> image;

    QSSGImageEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader, const char *inImageName)
        : shader(inShader), image(inImageName, inShader)
    {
    }

    void set(QSSGRenderImage2D *inImage) { image.set(inImage); }

    static QSSGImageEntry createImageEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader, const char *inStem)
    {
        return QSSGImageEntry(inShader, inStem);
    }
};

struct QSSGAllocatedDataBufferEntry
{
    QAtomicInt ref;
    QByteArray name;
    QSSGRef<QSSGRenderDataBuffer> dataBuffer;
    QSSGRenderBufferType bufferType;
    QSSGByteRef bufferData;
    QSSGAllocateBufferFlags flags;
    bool needsClear;

    QSSGAllocatedDataBufferEntry(const QByteArray &inName,
                                   QSSGRenderDataBuffer &inDataBuffer,
                                   QSSGRenderBufferType inType,
                                   const QSSGByteRef &data,
                                   QSSGAllocateBufferFlags inFlags)
        : name(inName), dataBuffer(&inDataBuffer), bufferType(inType), bufferData(data), flags(inFlags), needsClear(false)
    {
    }
    QSSGAllocatedDataBufferEntry() = default;
};

struct QSSGDataBufferEntry
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> shader;
    QSSGRenderCachedShaderBuffer<QSSGRenderShaderBufferBase> dataBuffer;

    QSSGDataBufferEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader, const char *inBufferName)
        : shader(inShader), dataBuffer(inBufferName, inShader)
    {
    }

    void set(QSSGRenderDataBuffer *inBuffer)
    {
        if (inBuffer)
            inBuffer->bind();

        dataBuffer.set();
    }

    static QSSGDataBufferEntry createDataBufferEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader, const char *inStem)
    {
        return QSSGDataBufferEntry(inShader, inStem);
    }
};

struct QSSGTextureEntry
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> shader;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> texture;
    QSSGRenderCachedShaderProperty<QVector4D> textureData;
    QSSGRenderCachedShaderProperty<qint32> textureFlags;

    QSSGTextureEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader, const char *inTexName, const char *inDataName, const char *inFlagName)
        : shader(inShader), texture(inTexName, inShader), textureData(inDataName, inShader), textureFlags(inFlagName, inShader)
    {
    }

    void set(const QSSGRef<QSSGRenderTexture2D> &inTexture, bool inNeedsAlphaMultiply, const QSSGRenderEffect::TextureProperty *inDefinition)
    {
        float theMixValue(inNeedsAlphaMultiply ? 0.0f : 1.0f);
        if (inTexture && inDefinition) {
            inTexture->setMagFilter(inDefinition->magFilterType);
            inTexture->setMinFilter(static_cast<QSSGRenderTextureMinifyingOp>(inDefinition->magFilterType));
            inTexture->setTextureWrapS(inDefinition->clampType);
            inTexture->setTextureWrapT(inDefinition->clampType);
        }
        texture.set(inTexture.data());
        if (inTexture) {
            QSSGTextureDetails theDetails(inTexture->textureDetails());
            textureData.set(QVector4D((float)theDetails.width, (float)theDetails.height, theMixValue, 0.0f));
            // I have no idea what these flags do.
            textureFlags.set(1);
        } else {
            textureFlags.set(0);
        }
    }

    static QSSGTextureEntry createTextureEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                                 const QByteArray &inStem,
                                                 QString &inBuilder,
                                                 QString &inBuilder2)
    {
        inBuilder = QString::fromLatin1(inStem);
        inBuilder.append(QString::fromLatin1("Info"));
        inBuilder2 = QString::fromLatin1("flag");
        inBuilder2.append(QString::fromLatin1(inStem));
        return QSSGTextureEntry(inShader, inStem, inBuilder.toLocal8Bit(), inBuilder2.toLocal8Bit());
    }
};

typedef QPair<QByteArray, QSSGRef<QSSGTextureEntry>> TNamedTextureEntry;
typedef QPair<QByteArray, QSSGRef<QSSGImageEntry>> TNamedImageEntry;
typedef QPair<QByteArray, QSSGRef<QSSGDataBufferEntry>> TNamedDataBufferEntry;

struct QSSGEffectClass
{
    QAtomicInt ref;
    QSSGDynamicObjectClass *dynamicClass;

    QSSGEffectClass(QSSGDynamicObjectClass &dynClass) : dynamicClass(&dynClass) {}
};

struct QSSGEffectContext
{
    QAtomicInt ref;
    QByteArray m_className;
    QSSGRenderContextInterface *m_context;
    QSSGRef<QSSGResourceManager> m_resourceManager;
    QVector<QSSGAllocatedBufferEntry> m_allocatedBuffers;
    QVector<QSSGAllocatedImageEntry> m_allocatedImages;
    QVector<QSSGAllocatedDataBufferEntry> m_allocatedDataBuffers;
    QVector<TNamedTextureEntry> m_textureEntries;
    QVector<TNamedImageEntry> m_imageEntries;
    QVector<TNamedDataBufferEntry> m_dataBufferEntries;

    QSSGEffectContext(const QByteArray &inName, QSSGRenderContextInterface *ctx, const QSSGRef<QSSGResourceManager> &inManager)
        : m_className(inName), m_context(ctx), m_resourceManager(inManager)
    {
    }

    ~QSSGEffectContext()
    {
        while (m_allocatedBuffers.size())
            releaseBuffer(0);

        while (m_allocatedImages.size())
            releaseImage(0);

        while (m_allocatedDataBuffers.size())
            releaseDataBuffer(0);
    }

    void releaseBuffer(qint32 inIdx)
    {
        QSSGAllocatedBufferEntry &theEntry(m_allocatedBuffers[inIdx]);
        theEntry.frameBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());
        m_resourceManager->release(theEntry.frameBuffer);
        m_resourceManager->release(theEntry.texture);
        { // replace_with_last
            m_allocatedBuffers[inIdx] = m_allocatedBuffers.back();
            m_allocatedBuffers.pop_back();
        }
    }

    void releaseImage(qint32 inIdx)
    {
        QSSGAllocatedImageEntry &theEntry(m_allocatedImages[inIdx]);
        m_resourceManager->release(theEntry.image);
        m_resourceManager->release(theEntry.texture);
        { // replace_with_last
            m_allocatedImages[inIdx] = m_allocatedImages.back();
            m_allocatedImages.pop_back();
        }
    }

    void releaseDataBuffer(qint32 inIdx)
    {
        QSSGAllocatedDataBufferEntry &theEntry(m_allocatedDataBuffers[inIdx]);
        ::free(theEntry.bufferData.begin());
        { // replace_with_last
            m_allocatedDataBuffers[inIdx] = m_allocatedDataBuffers.back();
            m_allocatedDataBuffers.pop_back();
        }
    }

    qint32 findBuffer(const QByteArray &inName)
    {
        for (qint32 idx = 0, end = m_allocatedBuffers.size(); idx < end; ++idx)
            if (m_allocatedBuffers[idx].name == inName)
                return idx;
        return m_allocatedBuffers.size();
    }

    qint32 findImage(const QByteArray &inName)
    {
        for (qint32 idx = 0, end = m_allocatedImages.size(); idx < end; ++idx)
            if (m_allocatedImages[idx].name == inName)
                return idx;

        return m_allocatedImages.size();
    }

    qint32 findDataBuffer(const QByteArray &inName)
    {
        for (qint32 idx = 0, end = m_allocatedDataBuffers.size(); idx < end; ++idx) {
            if (m_allocatedDataBuffers[idx].name == inName)
                return idx;
        }

        return m_allocatedDataBuffers.size();
    }

    void setTexture(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                    const QByteArray &inPropName,
                    const QSSGRef<QSSGRenderTexture2D> &inTexture,
                    bool inNeedsMultiply,
                    QString &inStringBuilder,
                    QString &inStringBuilder2,
                    const QSSGRenderEffect::TextureProperty *inPropDec = nullptr)
    {
        QSSGRef<QSSGTextureEntry> theTextureEntry;
        for (qint32 idx = 0, end = m_textureEntries.size(); idx < end && theTextureEntry == nullptr; ++idx) {
            if (m_textureEntries[idx].first == inPropName && m_textureEntries[idx].second->shader == inShader)
                theTextureEntry = m_textureEntries[idx].second;
        }
        if (theTextureEntry == nullptr) {
            QSSGRef<QSSGTextureEntry> theNewEntry(new QSSGTextureEntry(
                    QSSGTextureEntry::createTextureEntry(inShader, inPropName, inStringBuilder, inStringBuilder2)));
            m_textureEntries.push_back(QPair<QByteArray, QSSGRef<QSSGTextureEntry>>(inPropName, theNewEntry));
            theTextureEntry = theNewEntry;
        }
        theTextureEntry->set(inTexture, inNeedsMultiply, inPropDec);
    }

    void setImage(const QSSGRef<QSSGRenderShaderProgram> &inShader, const QByteArray &inPropName, const QSSGRef<QSSGRenderImage2D> &inImage)
    {
        QSSGRef<QSSGImageEntry> theImageEntry;
        for (qint32 idx = 0, end = m_imageEntries.size(); idx < end && theImageEntry == nullptr; ++idx) {
            if (m_imageEntries[idx].first == inPropName && m_imageEntries[idx].second->shader == inShader)
                theImageEntry = m_imageEntries[idx].second;
        }
        if (theImageEntry == nullptr) {
            QSSGRef<QSSGImageEntry> theNewEntry(
                    new QSSGImageEntry(QSSGImageEntry::createImageEntry(inShader, inPropName)));
            m_imageEntries.push_back(QPair<QByteArray, QSSGRef<QSSGImageEntry>>(inPropName, theNewEntry));
            theImageEntry = theNewEntry;
        }

        theImageEntry->set(inImage.data());
    }

    void setDataBuffer(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                       const QByteArray &inPropName,
                       const QSSGRef<QSSGRenderDataBuffer> &inBuffer)
    {
        QSSGRef<QSSGDataBufferEntry> theDataBufferEntry;
        for (qint32 idx = 0, end = m_dataBufferEntries.size(); idx < end && theDataBufferEntry == nullptr; ++idx) {
            if (m_dataBufferEntries[idx].first == inPropName && m_dataBufferEntries[idx].second->shader == inShader)
                theDataBufferEntry = m_dataBufferEntries[idx].second;
        }
        if (theDataBufferEntry == nullptr) {
            QSSGRef<QSSGDataBufferEntry> theNewEntry(new QSSGDataBufferEntry(
                    QSSGDataBufferEntry::createDataBufferEntry(inShader, inPropName)));
            m_dataBufferEntries.push_back(QPair<QByteArray, QSSGRef<QSSGDataBufferEntry>>(inPropName, theNewEntry));
            theDataBufferEntry = theNewEntry;
        }

        theDataBufferEntry->set(inBuffer.data());
    }
};

/* We setup some shared state on the effect shaders */
struct QSSGEffectShader
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> m_shader;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_mvp;
    QSSGRenderCachedShaderProperty<QVector2D> m_fragColorAlphaSettings;
    QSSGRenderCachedShaderProperty<QVector2D> m_destSize;
    QSSGRenderCachedShaderProperty<float> m_appFrame;
    QSSGRenderCachedShaderProperty<float> m_fps;
    QSSGRenderCachedShaderProperty<QVector2D> m_cameraClipRange;
    QSSGTextureEntry m_textureEntry;
    QSSGEffectShader(const QSSGRef<QSSGRenderShaderProgram> &inShader);
};

QSSGEffectShader::QSSGEffectShader(const QSSGRef<QSSGRenderShaderProgram> &inShader)
    : m_shader(inShader)
    , m_mvp("ModelViewProjectionMatrix", inShader)
    , m_fragColorAlphaSettings("FragColorAlphaSettings", inShader)
    , m_destSize("DestSize", inShader)
    , m_appFrame("AppFrame", inShader)
    , m_fps("FPS", inShader)
    , m_cameraClipRange("CameraClipRange", inShader)
    , m_textureEntry(inShader, "Texture0", "Texture0Info", "Texture0Flags")
{
}

QSSGEffectRenderArgument::QSSGEffectRenderArgument(QSSGRenderEffect *inEffect,
                                                       const QSSGRef<QSSGRenderTexture2D> &inColorBuffer,
                                                       const QVector2D &inCameraClipRange,
                                                       const QSSGRef<QSSGRenderTexture2D> &inDepthTexture,
                                                       const QSSGRef<QSSGRenderTexture2D> &inDepthBuffer)
    : m_effect(inEffect)
    , m_colorBuffer(inColorBuffer)
    , m_cameraClipRange(inCameraClipRange)
    , m_depthTexture(inDepthTexture)
    , m_depthStencilBuffer(inDepthBuffer)
{
}

QSSGRenderEffect::~QSSGRenderEffect()
{

}

QSSGEffectSystem::QSSGEffectSystem(QSSGRenderContextInterface *inContext) : m_context(inContext)
{
    init();
}

QSSGEffectSystem::~QSSGEffectSystem()
{
    for (qint32 idx = 0, end = m_contexts.size(); idx < end; ++idx)
        delete m_contexts[idx].data();
    m_contexts.clear();
}

QSSGEffectContext &QSSGEffectSystem::getEffectContext(QSSGRenderEffect &inEffect)
{
    if (inEffect.m_context == nullptr) {
        inEffect.m_context = new QSSGEffectContext(inEffect.className, m_context, m_resourceManager);
        m_contexts.push_back(inEffect.m_context);
    }
    return *inEffect.m_context;
}

const QSSGRef<QSSGEffectClass> QSSGEffectSystem::getEffectClass(const QByteArray &inStr) const
{
    const auto theIter = m_effectClasses.constFind(inStr);
    if (theIter != m_effectClasses.cend())
        return theIter.value();
    return nullptr;
}

bool QSSGEffectSystem::isEffectRegistered(const QByteArray &inStr) { return getEffectClass(inStr) != nullptr; }

QVector<QByteArray> QSSGEffectSystem::getRegisteredEffects()
{
    m_effectList.clear();
    auto theIter = m_effectClasses.cbegin();
    const auto theEnd = m_effectClasses.cend();
    for (; theIter != theEnd; ++theIter)
        m_effectList.push_back(theIter.key());
    return m_effectList;
}

void QSSGEffectSystem::setEffectPropertyDefaultValue(QString inName, QString inPropName, QSSGByteView inDefaultData)
{
    Q_UNUSED(inName)
    Q_UNUSED(inPropName)
    Q_UNUSED(inDefaultData)
    Q_ASSERT(0);
    //        m_context->dynamicObjectSystem()->setPropertyDefaultValue(inName, inPropName, inDefaultData);
}

void QSSGEffectSystem::setEffectPropertyEnumNames(QString inName, QString inPropName, QSSGDataView<QString> inNames)
{
    Q_UNUSED(inName)
    Q_UNUSED(inPropName)
    Q_UNUSED(inNames)
    Q_ASSERT(0);
    //        m_context->dynamicObjectSystem()->setPropertyEnumNames(inName, inPropName, inNames);
}

bool QSSGEffectSystem::registerEffect(const QByteArray &inName)
{
    Q_UNUSED(inName)
    Q_ASSERT(0);
//    if (isEffectRegistered(inName))
//        return false;
//    m_context->dynamicObjectSystem()->doRegister(inName, inProperties, sizeof(QSSGRenderEffect), QSSGRenderGraphObject::Type::Effect);
//    auto theClass = m_context->dynamicObjectSystem()->dynamicObjectClass(inName);
//    QSSGRef<QSSGEffectClass> theEffect(new QSSGEffectClass(*theClass));
//    m_effectClasses.insert(inName, theEffect);
//    return true;
return false;
}

bool QSSGEffectSystem::unregisterEffect(const QByteArray &inName)
{
    Q_UNUSED(inName)
    Q_ASSERT(0);
    //        if (!isEffectRegistered(inName))
    //            return false;

    //        m_context->dynamicObjectSystem()->unregister(inName);

    //        TEffectClassMap::iterator iter = m_effectClasses.find(inName);
    //        if (iter != m_effectClasses.end())
    //            m_effectClasses.erase(iter);

    //        for (quint32 idx = 0, end = m_contexts.size(); idx < end; ++idx) {
    //            if (m_contexts[idx]->m_className == inName)
    //                releaseEffectContext(m_contexts[idx].data());
    //        }
    //        return true;
    return false;
}

void QSSGEffectSystem::setEffectPropertyTextureSettings(QString inName,
                                                          QString inPropName,
                                                          QString inPropPath,
                                                          QSSGRenderTextureTypeValue inTexType,
                                                          QSSGRenderTextureCoordOp inCoordOp,
                                                          QSSGRenderTextureMagnifyingOp inMagFilterOp,
                                                          QSSGRenderTextureMinifyingOp inMinFilterOp)
{
    Q_UNUSED(inName)
    Q_UNUSED(inPropName)
    Q_UNUSED(inPropPath)
    Q_UNUSED(inTexType)
    Q_UNUSED(inCoordOp)
    Q_UNUSED(inMagFilterOp)
    Q_UNUSED(inMinFilterOp)
    Q_ASSERT(0);
    //        m_context->dynamicObjectSystem()
    //                ->setPropertyTextureSettings(inName, inPropName, inPropPath, inTexType, inCoordOp, inMagFilterOp, inMinFilterOp);
}

void QSSGEffectSystem::setEffectRequiresDepthTexture(const QByteArray &inEffectName, bool inValue)
{
    Q_UNUSED(inEffectName)
    Q_UNUSED(inValue)
    Q_ASSERT(0);
    //        auto theClass = getEffectClass(inEffectName);
    //        if (theClass == nullptr) {
    //            Q_ASSERT(false);
    //            return;
    //        }
    //        theClass->dynamicClass->setRequiresDepthTexture(inValue);
}

bool QSSGEffectSystem::doesEffectRequireDepthTexture(const QByteArray &inEffectName) const
{
    Q_UNUSED(inEffectName)
    Q_ASSERT(0);
    //        const auto theClass = getEffectClass(inEffectName);
    //        if (theClass == nullptr) {
    //            Q_ASSERT(false);
    //            return false;
    //        }
    //        return theClass->dynamicClass->requiresDepthTexture();
    return false;
}

void QSSGEffectSystem::setEffectRequiresCompilation(const QByteArray &inEffectName, bool inValue)
{
    Q_UNUSED(inValue)
    Q_ASSERT(0);
    auto theClass = getEffectClass(inEffectName);
    if (theClass == nullptr) {
        Q_ASSERT(false);
        return;
    }
//    theClass->dynamicClass->setRequiresCompilation(inValue);
}

bool QSSGEffectSystem::doesEffectRequireCompilation(const QByteArray &inEffectName) const
{
    Q_UNUSED(inEffectName)
    Q_ASSERT(0);
    //        const auto theClass = getEffectClass(inEffectName);
    //        if (theClass == nullptr) {
    //            Q_ASSERT(false);
    //            return false;
    //        }
    //        return theClass->dynamicClass->requiresCompilation();
    return false;
}

QSSGRenderEffect *QSSGEffectSystem::createEffectInstance(const QByteArray &inEffectName)
{
    auto theClass = getEffectClass(inEffectName);
    if (theClass == nullptr)
        return nullptr;
    //        StaticAssert<(sizeof(SEffect) % 4 == 0)>::valid_expression();

    //        QSSGRenderEffect *theEffect = static_cast<QSSGRenderEffect *>(
    //                m_context->dynamicObjectSystem()->createInstance(inEffectName));
    //        theEffect->initialize();
    //        return theEffect;
    return nullptr;
}

void QSSGEffectSystem::allocateBuffer(QSSGRenderEffect &inEffect, const QSSGAllocateBuffer &inCommand, quint32 inFinalWidth, quint32 inFinalHeight, QSSGRenderTextureFormat inSourceTextureFormat)
{
    // Check to see if it is already allocated and if it is, is it the correct size. If both of
    // these assumptions hold, then we are good.
    QSSGRef<QSSGRenderTexture2D> theBufferTexture;
    const qint32 theWidth = QSSGRendererUtil::nextMultipleOf4((quint32)(inFinalWidth * inCommand.m_sizeMultiplier));
    const qint32 theHeight = QSSGRendererUtil::nextMultipleOf4((quint32)(inFinalHeight * inCommand.m_sizeMultiplier));
    QSSGRenderTextureFormat resultFormat = inCommand.m_format;
    if (resultFormat == QSSGRenderTextureFormat::Unknown)
        resultFormat = inSourceTextureFormat;

    if (inEffect.m_context) {
        QSSGEffectContext &theContext(*inEffect.m_context);
        // size intentionally requiried every loop;
        qint32 bufferIdx = theContext.findBuffer(inCommand.m_name);
        if (bufferIdx < theContext.m_allocatedBuffers.size()) {
            QSSGAllocatedBufferEntry &theEntry(theContext.m_allocatedBuffers[bufferIdx]);
            QSSGTextureDetails theDetails = theEntry.texture->textureDetails();
            if (theDetails.width == theWidth && theDetails.height == theHeight && theDetails.format == resultFormat) {
                theBufferTexture = theEntry.texture;
            } else {
                theContext.releaseBuffer(bufferIdx);
            }
        }
    }
    if (theBufferTexture == nullptr) {
        QSSGEffectContext &theContext(getEffectContext(inEffect));
        auto theFB(m_resourceManager->allocateFrameBuffer());
        auto theTexture(m_resourceManager->allocateTexture2D(theWidth, theHeight, resultFormat));
        theTexture->setMagFilter(inCommand.m_filterOp);
        theTexture->setMinFilter(static_cast<QSSGRenderTextureMinifyingOp>(inCommand.m_filterOp));
        theTexture->setTextureWrapS(inCommand.m_texCoordOp);
        theTexture->setTextureWrapT(inCommand.m_texCoordOp);
        theFB->attach(QSSGRenderFrameBufferAttachment::Color0, theTexture);
        theContext.m_allocatedBuffers.push_back(
                    QSSGAllocatedBufferEntry(inCommand.m_name, *theFB, *theTexture, inCommand.m_bufferFlags));
        theBufferTexture = theTexture;
    }
}

void QSSGEffectSystem::allocateImage(QSSGRenderEffect &inEffect, const QSSGAllocateImage &inCommand, quint32 inFinalWidth, quint32 inFinalHeight)
{
    QSSGRef<QSSGRenderImage2D> theImage;
    qint32 theWidth = QSSGRendererUtil::nextMultipleOf4((quint32)(inFinalWidth * inCommand.m_sizeMultiplier));
    qint32 theHeight = QSSGRendererUtil::nextMultipleOf4((quint32)(inFinalHeight * inCommand.m_sizeMultiplier));

    Q_ASSERT(inCommand.m_format != QSSGRenderTextureFormat::Unknown);

    if (inEffect.m_context) {
        QSSGEffectContext &theContext(*inEffect.m_context);
        // size intentionally requiried every loop;
        qint32 imageIdx = theContext.findImage(inCommand.m_name);
        if (imageIdx < theContext.m_allocatedImages.size()) {
            QSSGAllocatedImageEntry &theEntry(theContext.m_allocatedImages[imageIdx]);
            QSSGTextureDetails theDetails = theEntry.texture->textureDetails();
            if (theDetails.width == theWidth && theDetails.height == theHeight && theDetails.format == inCommand.m_format) {
                theImage = theEntry.image;
            } else {
                theContext.releaseImage(imageIdx);
            }
        }
    }

    if (theImage == nullptr) {
        QSSGEffectContext &theContext(getEffectContext(inEffect));
        // allocate an immutable texture
        auto theTexture(m_resourceManager->allocateTexture2D(theWidth, theHeight, inCommand.m_format, 1, true));
        theTexture->setMagFilter(inCommand.m_filterOp);
        theTexture->setMinFilter(static_cast<QSSGRenderTextureMinifyingOp>(inCommand.m_filterOp));
        theTexture->setTextureWrapS(inCommand.m_texCoordOp);
        theTexture->setTextureWrapT(inCommand.m_texCoordOp);
        auto theImage = (m_resourceManager->allocateImage2D(theTexture, inCommand.m_access));
        theContext.m_allocatedImages.push_back(
                    QSSGAllocatedImageEntry(inCommand.m_name, *theImage, *theTexture, inCommand.m_bufferFlags));
    }
}

void QSSGEffectSystem::allocateDataBuffer(QSSGRenderEffect &inEffect, const QSSGAllocateDataBuffer &inCommand)
{
    const qint32 theBufferSize = qint32(inCommand.m_size);
    Q_ASSERT(theBufferSize > 0);
    QSSGRef<QSSGRenderDataBuffer> theDataBuffer;
    QSSGRef<QSSGRenderDataBuffer> theDataWrapBuffer;

    if (inEffect.m_context) {
        QSSGEffectContext &theContext(*inEffect.m_context);
        // size intentionally requiried every loop;
        qint32 bufferIdx = theContext.findDataBuffer(inCommand.m_name);
        if (bufferIdx < theContext.m_allocatedDataBuffers.size()) {
            QSSGAllocatedDataBufferEntry &theEntry(theContext.m_allocatedDataBuffers[bufferIdx]);
            if (theEntry.bufferType == inCommand.m_dataBufferType && theEntry.bufferData.size() == theBufferSize) {
                theDataBuffer = theEntry.dataBuffer;
            } else {
                // if type and size don't match something is wrong
                Q_ASSERT(false);
            }
        }
    }

    if (theDataBuffer == nullptr) {
        QSSGEffectContext &theContext(getEffectContext(inEffect));
        const auto &theRenderContext(m_context->renderContext());
        quint8 *initialData = (quint8 *)::malloc(theBufferSize);
        QSSGByteRef data((quint8 *)initialData, theBufferSize);
        memset(initialData, 0x0L, theBufferSize);
        if (inCommand.m_dataBufferType == QSSGRenderBufferType::Storage) {
            theDataBuffer = new QSSGRenderStorageBuffer(theRenderContext, inCommand.m_name,
                                                          QSSGRenderBufferUsageType::Dynamic,
                                                          data,
                                                          nullptr);
        } else if (inCommand.m_dataBufferType == QSSGRenderBufferType::DrawIndirect) {
            Q_ASSERT(theBufferSize == sizeof(DrawArraysIndirectCommand));
            // init a draw call
            quint32 *pIndirectDrawCall = (quint32 *)initialData;
            // vertex count we draw points right now only
            // the rest we fill in by GPU
            pIndirectDrawCall[0] = 1;
            theDataBuffer = new QSSGRenderDrawIndirectBuffer(theRenderContext, QSSGRenderBufferUsageType::Dynamic, data);
        } else
            Q_ASSERT(false);

        theContext.m_allocatedDataBuffers.push_back(QSSGAllocatedDataBufferEntry(inCommand.m_name,
                                                                                   *theDataBuffer,
                                                                                   inCommand.m_dataBufferType,
                                                                                   data,
                                                                                   inCommand.m_bufferFlags));

        // create wrapper buffer
        if (inCommand.m_dataBufferWrapType == QSSGRenderBufferType::Storage
                && !inCommand.m_wrapName.isEmpty() && theDataBuffer) {
            theDataWrapBuffer = new QSSGRenderStorageBuffer(theRenderContext, inCommand.m_wrapName,
                                                              QSSGRenderBufferUsageType::Dynamic,
                                                              data,
                                                              theDataBuffer.data());
            theContext.m_allocatedDataBuffers.push_back(QSSGAllocatedDataBufferEntry(inCommand.m_wrapName,
                                                                                       *theDataWrapBuffer,
                                                                                       inCommand.m_dataBufferWrapType,
                                                                                       QSSGByteRef(),
                                                                                       inCommand.m_bufferFlags));
        }
        ::free(initialData);
    }
}

QSSGRef<QSSGRenderTexture2D> QSSGEffectSystem::findTexture(QSSGRenderEffect *inEffect, const QByteArray &inName)
{
    if (inEffect->m_context) {
        QSSGEffectContext &theContext(*inEffect->m_context);
        qint32 bufferIdx = theContext.findBuffer(inName);
        if (bufferIdx < theContext.m_allocatedBuffers.size())
            return theContext.m_allocatedBuffers[bufferIdx].texture;
    }
    Q_ASSERT(false);
    return nullptr;
}

QSSGRef<QSSGRenderFrameBuffer> QSSGEffectSystem::bindBuffer(QSSGRenderEffect &inEffect, const QSSGBindBuffer &inCommand, QMatrix4x4 &outMVP, QVector2D &outDestSize)
{
    QSSGRef<QSSGRenderFrameBuffer> theBuffer;
    QSSGRef<QSSGRenderTexture2D> theTexture;
    if (inEffect.m_context) {
        QSSGEffectContext &theContext(*inEffect.m_context);
        qint32 bufferIdx = theContext.findBuffer(inCommand.m_bufferName);
        if (bufferIdx < theContext.m_allocatedBuffers.size()) {
            theBuffer = theContext.m_allocatedBuffers[bufferIdx].frameBuffer;
            theTexture = theContext.m_allocatedBuffers[bufferIdx].texture;
            theContext.m_allocatedBuffers[bufferIdx].needsClear = false;
        }
    }
    if (theBuffer == nullptr) {
        qCCritical(INVALID_OPERATION,
                   "Effect %s: Failed to find buffer %s for bind",
                   inEffect.className,
                   inCommand.m_bufferName.constData());
        QString errorMsg = QObject::tr("Failed to compile \"%1\" effect.\nConsider"
                                       " removing it from the presentation.")
                                   .arg(QString::fromLatin1(inEffect.className));
        // TODO:
        //            QSSG_ALWAYS_ASSERT_MESSAGE(errorMsg.toUtf8());
        outMVP = QMatrix4x4();
        return nullptr;
    }

    if (theTexture) {
        QSSGRenderCamera::setupOrthographicCameraForOffscreenRender(*theTexture, outMVP);
        QSSGTextureDetails theDetails(theTexture->textureDetails());
        m_context->renderContext()->setViewport(QRect(0, 0, (quint32)theDetails.width, (quint32)theDetails.height));
        outDestSize = QVector2D((float)theDetails.width, (float)theDetails.height);
    }

    return theBuffer;
}

QSSGRef<QSSGEffectShader> QSSGEffectSystem::bindShader(const QByteArray &inEffectId, const QSSGBindShader &inCommand)
{
    auto theClass = getEffectClass(inEffectId);
    if (!theClass) {
        Q_ASSERT(false);
        return nullptr;
    }

    Q_ASSERT(0);
    bool forceCompilation = true; // theClass->dynamicClass->requiresCompilation();

    auto key = TStrStrPair(inCommand.m_shaderPath, inCommand.m_shaderDefine);
    auto theInsertResult = m_shaderMap.find(key);
    const bool found = (theInsertResult != m_shaderMap.end());
    if (!found)
        theInsertResult = m_shaderMap.insert(key, QSSGRef<QSSGEffectShader>());

    if (found || forceCompilation) {
        auto theProgram = m_context->dynamicObjectSystem()
                                  ->getShaderProgram(inCommand.m_shaderPath,
                                                     inCommand.m_shaderDefine,
                                                     TShaderFeatureSet(),
                                                     QSSGDynamicShaderProgramFlags(),
                                                     forceCompilation)
                                  .first;
        if (theProgram)
            theInsertResult.value() = QSSGRef<QSSGEffectShader>(new QSSGEffectShader(theProgram));
    }
    if (theInsertResult.value()) {
        const auto &theContext(m_context->renderContext());
        theContext->setActiveShader(theInsertResult.value()->m_shader);
    }

    return theInsertResult.value();
}

void QSSGEffectSystem::doApplyInstanceValue(QSSGRenderEffect *inEffect,
                                              const QByteArray &inPropertyName,
                                              const QVariant &propertyValue,
                                              QSSGRenderShaderDataType inPropertyType,
                                              const QSSGRef<QSSGRenderShaderProgram> &inShader)
{
    auto theConstant = inShader->shaderConstant(inPropertyName);
    if (theConstant) {
        if (theConstant->getShaderConstantType() == inPropertyType) {
            if (inPropertyType == QSSGRenderShaderDataType::Texture2D) {
                const auto &theBufferManager(m_context->bufferManager());
                auto theOffscreenRenderer = m_context->offscreenRenderManager();
                bool needsAlphaMultiply = true;

                const QSSGRenderEffect::TextureProperty *textureProperty = reinterpret_cast<QSSGRenderEffect::TextureProperty *>(propertyValue.value<void *>());
                QSSGRenderImage *image = textureProperty->texImage;
                if (image) {
                    const QString &imageSource = image->m_imagePath;
                    QSSGRef<QSSGRenderTexture2D> theTexture;
                    if (!imageSource.isEmpty()) {
                        if (theOffscreenRenderer->hasOffscreenRenderer(imageSource)) {
                            QSSGOffscreenRenderResult theResult = theOffscreenRenderer->getRenderedItem(imageSource);
                            needsAlphaMultiply = false;
                            theTexture = theResult.texture;
                        } else {
                            QSSGRenderImageTextureData theTextureData = theBufferManager->loadRenderImage(imageSource, QSSGRenderTextureFormat::Unknown);
                            needsAlphaMultiply = true;
                            theTexture = theTextureData.m_texture;
                        }
                    }
                    getEffectContext(*inEffect).setTexture(inShader,
                                                           inPropertyName,
                                                           theTexture,
                                                           needsAlphaMultiply,
                                                           m_textureStringBuilder,
                                                           m_textureStringBuilder2,
                                                           textureProperty);
                }
            } else if (inPropertyType == QSSGRenderShaderDataType::Image2D) {
                // TODO:
                //                    StaticAssert<sizeof(QString)
                //                            == sizeof(QSSGRenderTexture2DPtr)>::valid_expression();
                QSSGRef<QSSGRenderImage2D> theImage;
                getEffectContext(*inEffect).setImage(inShader, inPropertyName, theImage);
            } else if (inPropertyType == QSSGRenderShaderDataType::DataBuffer) {
                // we don't handle this here
            } else {
                switch (inPropertyType) {
                case QSSGRenderShaderDataType::Integer:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.toInt());
                    break;
                case QSSGRenderShaderDataType::IntegerVec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<qint32_2>());
                    break;
                case QSSGRenderShaderDataType::IntegerVec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<qint32_3>());
                    break;
                case QSSGRenderShaderDataType::IntegerVec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<qint32_4>());
                    break;
                case QSSGRenderShaderDataType::Boolean:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool>());
                    break;
                case QSSGRenderShaderDataType::BooleanVec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool_2>());
                    break;
                case QSSGRenderShaderDataType::BooleanVec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool_3>());
                    break;
                case QSSGRenderShaderDataType::BooleanVec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<bool_4>());
                    break;
                case QSSGRenderShaderDataType::Float:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<float>());
                    break;
                case QSSGRenderShaderDataType::Vec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QVector2D>());
                    break;
                case QSSGRenderShaderDataType::Vec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QVector3D>());
                    break;
                case QSSGRenderShaderDataType::Vec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QVector4D>());
                    break;
                case QSSGRenderShaderDataType::UnsignedInteger:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32>());
                    break;
                case QSSGRenderShaderDataType::UnsignedIntegerVec2:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32_2>());
                    break;
                case QSSGRenderShaderDataType::UnsignedIntegerVec3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32_3>());
                    break;
                case QSSGRenderShaderDataType::UnsignedIntegerVec4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<quint32_4>());
                    break;
                case QSSGRenderShaderDataType::Matrix3x3:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QMatrix3x3>());
                    break;
                case QSSGRenderShaderDataType::Matrix4x4:
                    inShader->setPropertyValue(theConstant.data(), propertyValue.value<QMatrix4x4>());
                    break;
                case QSSGRenderShaderDataType::Texture2D:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderTexture2D **>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::Texture2DHandle:
                    inShader->setPropertyValue(theConstant.data(),
                                               *(reinterpret_cast<QSSGRenderTexture2D ***>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::Texture2DArray:
                    inShader->setPropertyValue(theConstant.data(),
                                               *(reinterpret_cast<QSSGRenderTexture2DArray **>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::TextureCube:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderTextureCube **>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::TextureCubeHandle:
                    inShader->setPropertyValue(theConstant.data(),
                                               *(reinterpret_cast<QSSGRenderTextureCube ***>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::Image2D:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderImage2D **>(propertyValue.value<void *>())));
                    break;
                case QSSGRenderShaderDataType::DataBuffer:
                    inShader->setPropertyValue(theConstant.data(), *(reinterpret_cast<QSSGRenderDataBuffer **>(propertyValue.value<void *>())));
                    break;
                default:
                    Q_UNREACHABLE();
                }
            }

        } else {
            qCCritical(INVALID_OPERATION,
                       "Effect ApplyInstanceValue command datatype "
                       "and shader datatypes differ for property %s",
                       inPropertyName.constData());
            Q_ASSERT(false);
        }
    }
}

void QSSGEffectSystem::applyInstanceValue(QSSGRenderEffect *inEffect,
                                            const QSSGRef<QSSGEffectClass> &inClass,
                                            const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                            const QSSGApplyInstanceValue &inCommand)
{
    Q_UNUSED(inEffect)
    Q_UNUSED(inClass)
    Q_UNUSED(inShader)
    Q_UNUSED(inCommand)
    Q_ASSERT(0);
    // sanity check
    //        if (!inCommand.m_propertyName.isEmpty()) {
    //            bool canGetData = inCommand.m_valueOffset + getSizeofShaderDataType(inCommand.m_valueType) <= inEffect->dataSectionByteSize;
    //            if (canGetData == false) {
    //                Q_ASSERT(false);
    //                return;
    //            }
    //            quint8 *dataPtr = inEffect->getDataSectionBegin() + inCommand.m_valueOffset;
    //            const QSSGPropertyDefinition *theDefinition = inClass->dynamicClass->findPropertyByName(inCommand.m_propertyName);
    //            if (theDefinition)
    //                doApplyInstanceValue(inEffect, dataPtr, inCommand.m_propertyName, inCommand.m_valueType, inShader, *theDefinition);
    //        } else {
    //            QSSGDataView<QSSGPropertyDefinition> theDefs = inClass->dynamicClass->getProperties();
    //            for (quint32 idx = 0, end = theDefs.size(); idx < end; ++idx) {
    //                const QSSGPropertyDefinition &theDefinition(theDefs[idx]);
    //                auto theConstant = inShader->shaderConstant(theDefinition.name);

    //                // This is fine, the property wasn't found and we continue, no problem.
    //                if (!theConstant)
    //                    continue;
    //                quint8 *dataPtr = inEffect->getDataSectionBegin() + theDefinition.offset;
    //                doApplyInstanceValue(inEffect, dataPtr, theDefinition.name, theDefinition.dataType, inShader, theDefinition);
    //            }
    //        }
}

void QSSGEffectSystem::applyValue(QSSGRenderEffect *inEffect, const QSSGRef<QSSGEffectClass> &inClass, const QSSGRef<QSSGRenderShaderProgram> &inShader, const QSSGApplyValue &inCommand)
{
    Q_UNUSED(inEffect)
    Q_UNUSED(inClass)
    Q_UNUSED(inShader)
    Q_UNUSED(inCommand)
    Q_ASSERT(0);
    //        if (!inCommand.m_propertyName.isEmpty()) {
    //            quint8 *dataPtr = inCommand.m_value.mData;
    //            const QSSGPropertyDefinition *theDefinition = inClass->dynamicClass->findPropertyByName(inCommand.m_propertyName);
    //            if (theDefinition)
    //                doApplyInstanceValue(inEffect, dataPtr, inCommand.m_propertyName, inCommand.m_valueType, inShader, *theDefinition);
    //        }
}

bool QSSGEffectSystem::applyBlending(const QSSGApplyBlending &inCommand)
{
    const auto &theContext(m_context->renderContext());

    theContext->setBlendingEnabled(true);

    QSSGRenderBlendFunctionArgument blendFunc = QSSGRenderBlendFunctionArgument(inCommand.m_srcBlendFunc,
                                                                                    inCommand.m_dstBlendFunc,
                                                                                    inCommand.m_srcBlendFunc,
                                                                                    inCommand.m_dstBlendFunc);

    QSSGRenderBlendEquationArgument blendEqu(QSSGRenderBlendEquation::Add, QSSGRenderBlendEquation::Add);

    theContext->setBlendFunction(blendFunc);
    theContext->setBlendEquation(blendEqu);

    return true;
}

QSSGEffectTextureData QSSGEffectSystem::applyBufferValue(QSSGRenderEffect *inEffect, const QSSGRef<QSSGRenderShaderProgram> &inShader, const QSSGApplyBufferValue &inCommand, const QSSGRef<QSSGRenderTexture2D> &inSourceTexture, const QSSGEffectTextureData &inCurrentSourceTexture)
{
    QSSGEffectTextureData theTextureToBind;
    if (!inCommand.m_bufferName.isEmpty()) {
        if (inEffect->m_context) {
            QSSGEffectContext &theContext(*inEffect->m_context);
            qint32 bufferIdx = theContext.findBuffer(inCommand.m_bufferName);
            if (bufferIdx < theContext.m_allocatedBuffers.size()) {
                QSSGAllocatedBufferEntry &theEntry(theContext.m_allocatedBuffers[bufferIdx]);
                if (theEntry.needsClear) {
                    auto theRenderContext(m_context->renderContext());

                    theRenderContext->setRenderTarget(theEntry.frameBuffer);
                    // Note that depth/stencil buffers need an explicit clear in their bind
                    // commands in order to ensure
                    // we clear the least amount of information possible.

                    if (theEntry.texture) {
                        QSSGRenderTextureFormat theTextureFormat = theEntry.texture->textureDetails().format;
                        if (theTextureFormat != QSSGRenderTextureFormat::Depth16
                                && theTextureFormat != QSSGRenderTextureFormat::Depth24
                                && theTextureFormat != QSSGRenderTextureFormat::Depth32
                                && theTextureFormat != QSSGRenderTextureFormat::Depth24Stencil8) {
                            QSSGRenderContextScopedProperty<QVector4D> __clearColor(*theRenderContext,
                                                                                      &QSSGRenderContext::clearColor,
                                                                                      &QSSGRenderContext::setClearColor,
                                                                                      QVector4D());
                            theRenderContext->clear(QSSGRenderClearValues::Color);
                        }
                    }
                    theEntry.needsClear = false;
                }
                theTextureToBind = QSSGEffectTextureData(theEntry.texture, false);
            }
        }
        if (theTextureToBind.texture == nullptr) {
            Q_ASSERT(false);
            qCCritical(INVALID_OPERATION,
                       "Effect %s: Failed to find buffer %s for bind",
                       inEffect->className,
                       inCommand.m_bufferName.constData());
            Q_ASSERT(false);
        }
    } else { // no name means bind the source
        theTextureToBind = QSSGEffectTextureData(inSourceTexture, false);
    }

    if (!inCommand.m_paramName.isEmpty()) {
        auto theConstant = inShader->shaderConstant(inCommand.m_paramName);

        if (theConstant) {
            if (theConstant->getShaderConstantType() != QSSGRenderShaderDataType::Texture2D) {
                qCCritical(INVALID_OPERATION,
                           "Effect %s: Binding buffer to parameter %s that is not a texture",
                           inEffect->className,
                           inCommand.m_paramName.constData());
                Q_ASSERT(false);
            } else {
                getEffectContext(*inEffect).setTexture(inShader,
                                                       inCommand.m_paramName,
                                                       theTextureToBind.texture,
                                                       theTextureToBind.needsAlphaMultiply,
                                                       m_textureStringBuilder,
                                                       m_textureStringBuilder2);
            }
        }
        return inCurrentSourceTexture;
    } else {
        return theTextureToBind;
    }
}

void QSSGEffectSystem::applyDepthValue(QSSGRenderEffect *inEffect, const QSSGRef<QSSGRenderShaderProgram> &inShader, const QSSGApplyDepthValue &inCommand, const QSSGRef<QSSGRenderTexture2D> &inTexture)
{
    auto theConstant = inShader->shaderConstant(inCommand.m_paramName);

    if (theConstant) {
        if (theConstant->getShaderConstantType() != QSSGRenderShaderDataType::Texture2D) {
            qCCritical(INVALID_OPERATION,
                       "Effect %s: Binding buffer to parameter %s that is not a texture",
                       inEffect->className,
                       inCommand.m_paramName.constData());
            Q_ASSERT(false);
        } else {
            getEffectContext(*inEffect).setTexture(inShader, inCommand.m_paramName, inTexture, false, m_textureStringBuilder, m_textureStringBuilder2);
        }
    }
}

void QSSGEffectSystem::applyImageValue(QSSGRenderEffect *inEffect, const QSSGRef<QSSGRenderShaderProgram> &inShader, const QSSGApplyImageValue &inCommand)
{
    QSSGAllocatedImageEntry theImageToBind;
    if (!inCommand.m_imageName.isEmpty()) {
        if (inEffect->m_context) {
            QSSGEffectContext &theContext(*inEffect->m_context);
            qint32 bufferIdx = theContext.findImage(inCommand.m_imageName);
            if (bufferIdx < theContext.m_allocatedImages.size()) {
                theImageToBind = QSSGAllocatedImageEntry(theContext.m_allocatedImages[bufferIdx]);
            }
        }
    }

    if (theImageToBind.image == nullptr) {
        qCCritical(INVALID_OPERATION,
                   "Effect %s: Failed to find image %s for bind",
                   inEffect->className,
                   inCommand.m_imageName.constData());
        Q_ASSERT(false);
    }

    if (!inCommand.m_paramName.isEmpty()) {
        auto theConstant = inShader->shaderConstant(inCommand.m_paramName);

        if (theConstant) {
            if (inCommand.m_needSync) {
                QSSGRenderBufferBarrierFlags flags(QSSGRenderBufferBarrierValues::TextureFetch
                                                     | QSSGRenderBufferBarrierValues::TextureUpdate);
                inShader->renderContext()->setMemoryBarrier(flags);
            }

            if (theConstant->getShaderConstantType() == QSSGRenderShaderDataType::Image2D && !inCommand.m_bindAsTexture) {
                getEffectContext(*inEffect).setImage(inShader, inCommand.m_paramName, theImageToBind.image);
            } else if (theConstant->getShaderConstantType() == QSSGRenderShaderDataType::Texture2D && inCommand.m_bindAsTexture) {
                getEffectContext(*inEffect).setTexture(inShader, inCommand.m_paramName, theImageToBind.texture, false, m_textureStringBuilder, m_textureStringBuilder2);
            } else {
                qCCritical(INVALID_OPERATION,
                           "Effect %s: Binding buffer to parameter %s that is not a texture",
                           inEffect->className,
                           inCommand.m_paramName.constData());
                Q_ASSERT(false);
            }
        }
    }
}

void QSSGEffectSystem::applyDataBufferValue(QSSGRenderEffect *inEffect, const QSSGRef<QSSGRenderShaderProgram> &inShader, const QSSGApplyDataBufferValue &inCommand)
{
    QSSGAllocatedDataBufferEntry theBufferToBind;
    if (!inCommand.m_paramName.isEmpty()) {
        if (inEffect->m_context) {
            QSSGEffectContext &theContext(*inEffect->m_context);
            qint32 bufferIdx = theContext.findDataBuffer(inCommand.m_paramName);
            if (bufferIdx < theContext.m_allocatedDataBuffers.size()) {
                theBufferToBind = QSSGAllocatedDataBufferEntry(theContext.m_allocatedDataBuffers[bufferIdx]);
                if (theBufferToBind.needsClear) {
                    QSSGByteRef pData = theBufferToBind.dataBuffer->mapBuffer();
                    memset(pData.begin(), 0x0L, theBufferToBind.bufferData.size());
                    theBufferToBind.dataBuffer->unmapBuffer();
                    theBufferToBind.needsClear = false;
                }
            }
        }

        if (theBufferToBind.dataBuffer == nullptr) {
            qCCritical(INVALID_OPERATION,
                       "Effect %s: Failed to find buffer %s for bind",
                       inEffect->className,
                       inCommand.m_paramName.constData());
            Q_ASSERT(false);
        }

        auto theConstant = inShader->shaderBuffer(inCommand.m_paramName);

        if (theConstant) {
            getEffectContext(*inEffect).setDataBuffer(inShader, inCommand.m_paramName, theBufferToBind.dataBuffer);
        } else if (theBufferToBind.bufferType == QSSGRenderBufferType::DrawIndirect) {
            // since we filled part of this buffer on the GPU we need a sync before usage
            QSSGRenderBufferBarrierFlags flags(QSSGRenderBufferBarrierValues::CommandBuffer);
            inShader->renderContext()->setMemoryBarrier(flags);
        }
    }
}

void QSSGEffectSystem::applyRenderStateValue(QSSGRenderFrameBuffer *inTarget, const QSSGRef<QSSGRenderTexture2D> &inDepthStencilTexture, const QSSGApplyRenderState &theCommand)
{
    const auto &theContext(m_context->renderContext());
    bool inEnable = theCommand.m_enabled;

    switch (theCommand.m_renderState) {
    case QSSGRenderState::StencilTest: {
        if (inEnable && inTarget) {
            inTarget->attach(QSSGRenderFrameBufferAttachment::DepthStencil, inDepthStencilTexture);
        } else if (inTarget) {
            inTarget->attach(QSSGRenderFrameBufferAttachment::DepthStencil, QSSGRenderTextureOrRenderBuffer());
        }

        theContext->setStencilTestEnabled(inEnable);
    } break;
    default:
        Q_ASSERT(false);
        break;
    }
}

bool QSSGEffectSystem::compareDepthStencilState(QSSGRenderDepthStencilState &inState, QSSGDepthStencil &inStencil)
{
    QSSGRenderStencilFunction theFunction = inState.stencilFunction(QSSGRenderFace::Front);
    QSSGRenderStencilOperation theOperation = inState.stencilOperation(QSSGRenderFace::Front);

    return theFunction.m_function == inStencil.m_stencilFunction && theFunction.m_mask == inStencil.m_mask
            && theFunction.m_referenceValue == inStencil.m_reference && theOperation.m_stencilFail == inStencil.m_stencilFailOperation
            && theOperation.m_depthFail == inStencil.m_depthFailOperation
            && theOperation.m_depthPass == inStencil.m_depthPassOperation;
}

void QSSGEffectSystem::renderPass(QSSGEffectShader &inShader, const QMatrix4x4 &inMVP, const QSSGEffectTextureData &inSourceTexture, const QSSGRef<QSSGRenderFrameBuffer> &inFrameBuffer, QVector2D &inDestSize, const QVector2D &inCameraClipRange, const QSSGRef<QSSGRenderTexture2D> &inDepthStencil, QSSGOption<QSSGDepthStencil> inDepthStencilCommand, bool drawIndirect)
{
    const auto &theContext(m_context->renderContext());
    theContext->setRenderTarget(inFrameBuffer);
    if (inDepthStencil && inFrameBuffer) {
        inFrameBuffer->attach(QSSGRenderFrameBufferAttachment::DepthStencil, inDepthStencil);
        if (inDepthStencilCommand.hasValue()) {
            QSSGDepthStencil &theDepthStencil(*inDepthStencilCommand);
            QSSGRenderClearFlags clearFlags;
            if (theDepthStencil.m_glags.hasClearStencil())
                clearFlags |= QSSGRenderClearValues::Stencil;
            if (theDepthStencil.m_glags.hasClearDepth())
                clearFlags |= QSSGRenderClearValues::Depth;

            if (clearFlags)
                theContext->clear(clearFlags);

            QSSGRef<QSSGRenderDepthStencilState> targetState;
            for (quint32 idx = 0, end = m_depthStencilStates.size(); idx < end && targetState == nullptr; ++idx) {
                QSSGRef<QSSGRenderDepthStencilState> theState = m_depthStencilStates[idx];
                if (compareDepthStencilState(*theState, theDepthStencil))
                    targetState = theState;
            }
            if (targetState == nullptr) {
                QSSGRenderStencilFunction theFunctionArg(theDepthStencil.m_stencilFunction,
                                                           theDepthStencil.m_reference,
                                                           theDepthStencil.m_mask);
                QSSGRenderStencilOperation theOpArg(theDepthStencil.m_stencilFailOperation,
                                                      theDepthStencil.m_depthFailOperation,
                                                      theDepthStencil.m_depthPassOperation);
                targetState = new QSSGRenderDepthStencilState(theContext,
                                                                theContext->isDepthTestEnabled(),
                                                                theContext->isDepthWriteEnabled(),
                                                                theContext->depthFunction(),
                                                                true,
                                                                theFunctionArg,
                                                                theFunctionArg,
                                                                theOpArg,
                                                                theOpArg);
                m_depthStencilStates.push_back(targetState);
            }
            theContext->setDepthStencilState(targetState);
        }
    }

    theContext->setActiveShader(inShader.m_shader);
    inShader.m_mvp.set(inMVP);
    if (inSourceTexture.texture) {
        inShader.m_textureEntry.set(inSourceTexture.texture, inSourceTexture.needsAlphaMultiply, nullptr);
    } else {
        qCCritical(INTERNAL_ERROR, "Failed to setup pass due to null source texture");
        Q_ASSERT(false);
    }
    inShader.m_fragColorAlphaSettings.set(QVector2D(1.0f, 0.0f));
    inShader.m_destSize.set(inDestSize);
    if (inShader.m_appFrame.isValid())
        inShader.m_appFrame.set((float)m_context->frameCount());
    if (inShader.m_fps.isValid())
        inShader.m_fps.set((float)m_context->getFPS().first);
    if (inShader.m_cameraClipRange.isValid())
        inShader.m_cameraClipRange.set(inCameraClipRange);

    if (!drawIndirect)
        m_context->renderer()->renderQuad();
    else
        m_context->renderer()->renderPointsIndirect();

    if (inDepthStencil && inFrameBuffer) {
        inFrameBuffer->attach(QSSGRenderFrameBufferAttachment::DepthStencil, QSSGRenderTextureOrRenderBuffer());
        theContext->setDepthStencilState(m_defaultStencilState);
    }
}

void QSSGEffectSystem::doRenderEffect(QSSGRenderEffect *inEffect,
                                        const QSSGRef<QSSGEffectClass> &inClass,
                                        const QSSGRef<QSSGRenderTexture2D> &inSourceTexture,
                                        QMatrix4x4 &inMVP,
                                        const QSSGRef<QSSGRenderFrameBuffer> &inTarget,
                                        bool inEnableBlendWhenRenderToTarget,
                                        const QSSGRef<QSSGRenderTexture2D> &inDepthTexture,
                                        const QSSGRef<QSSGRenderTexture2D> &inDepthStencilTexture,
                                        const QVector2D &inCameraClipRange)
{
    // Run through the effect commands and render the effect.
    // QSSGRenderTexture2D* theCurrentTexture(&inSourceTexture);
    const auto &theContext = m_context->renderContext();

    // Context variables that are updated during the course of a pass.
    QSSGEffectTextureData theCurrentSourceTexture(inSourceTexture, false);
    QSSGRef<QSSGRenderTexture2D> theCurrentDepthStencilTexture;
    QSSGRef<QSSGRenderFrameBuffer> theCurrentRenderTarget(inTarget);
    QSSGRef<QSSGEffectShader> theCurrentShader;
    QRect theOriginalViewport(theContext->viewport());
    bool wasScissorEnabled = theContext->isScissorTestEnabled();
    bool wasBlendingEnabled = theContext->isBlendingEnabled();
    // save current blending setup
    QSSGRenderBlendFunctionArgument theBlendFunc = theContext->blendFunction();
    QSSGRenderBlendEquationArgument theBlendEqu = theContext->blendEquation();
    bool intermediateBlendingEnabled = false;
    QSSGTextureDetails theDetails(inSourceTexture->textureDetails());
    const qint32 theFinalWidth = theDetails.width;
    const qint32 theFinalHeight = theDetails.height;
    QVector2D theDestSize;
    {
        // Ensure no matter the command run goes we replace the rendering system to some
        // semblance of the approprate
        // setting.
        QSSGRenderContextScopedProperty<QSSGRef<QSSGRenderFrameBuffer>> __framebuffer(*theContext,
                                                                                            &QSSGRenderContext::renderTarget,
                                                                                            &QSSGRenderContext::setRenderTarget);
        QSSGRenderContextScopedProperty<QRect> __viewport(*theContext,
                                                            &QSSGRenderContext::viewport,
                                                            &QSSGRenderContext::setViewport);
        QSSGRenderContextScopedProperty<bool> __scissorEnabled(*theContext,
                                                                 &QSSGRenderContext::isScissorTestEnabled,
                                                                 &QSSGRenderContext::setScissorTestEnabled);
        QSSGRenderContextScopedProperty<bool> __stencilTest(*theContext,
                                                              &QSSGRenderContext::isStencilTestEnabled,
                                                              &QSSGRenderContext::setStencilTestEnabled);
        QSSGRenderContextScopedProperty<QSSGRenderBoolOp> __depthFunction(*theContext,
                                                                              &QSSGRenderContext::depthFunction,
                                                                              &QSSGRenderContext::setDepthFunction);
        QSSGOption<QSSGDepthStencil> theCurrentDepthStencil;

        theContext->setScissorTestEnabled(false);
        theContext->setBlendingEnabled(false);
        theContext->setCullingEnabled(false);
        theContext->setDepthTestEnabled(false);
        theContext->setDepthWriteEnabled(false);

        QMatrix4x4 theMVP;
        const auto &theCommands = inEffect->commands;
        for (const auto &theCommand : theCommands) {
            switch (theCommand->m_type) {
            case CommandType::AllocateBuffer:
                allocateBuffer(*inEffect,
                               static_cast<const QSSGAllocateBuffer &>(*theCommand),
                               theFinalWidth,
                               theFinalHeight,
                               theDetails.format);
                break;

            case CommandType::AllocateImage:
                allocateImage(*inEffect, static_cast<const QSSGAllocateImage &>(*theCommand), theFinalWidth, theFinalHeight);
                break;

            case CommandType::AllocateDataBuffer:
                allocateDataBuffer(*inEffect, static_cast<const QSSGAllocateDataBuffer &>(*theCommand));
                break;

            case CommandType::BindBuffer:
                theCurrentRenderTarget = bindBuffer(*inEffect, static_cast<const QSSGBindBuffer &>(*theCommand), theMVP, theDestSize);
                break;

            case CommandType::BindTarget: {
                m_context->renderContext()->setRenderTarget(inTarget);
                theCurrentRenderTarget = inTarget;
                theMVP = inMVP;
                theContext->setViewport(theOriginalViewport);
                theDestSize = QVector2D(float(theFinalWidth), float(theFinalHeight));
                // This isn't necessary if we are rendering to an offscreen buffer and not
                // compositing
                // with other objects.
                if (inEnableBlendWhenRenderToTarget) {
                    theContext->setBlendingEnabled(wasBlendingEnabled);
                    theContext->setScissorTestEnabled(wasScissorEnabled);
                    // The blending setup was done before we apply the effect
                    theContext->setBlendFunction(theBlendFunc);
                    theContext->setBlendEquation(theBlendEqu);
                }
            } break;
            case CommandType::BindShader:
                theCurrentShader = bindShader(inEffect->className, static_cast<const QSSGBindShader &>(*theCommand));
                break;
            case CommandType::ApplyInstanceValue:
                if (theCurrentShader)
                    applyInstanceValue(inEffect,
                                       inClass,
                                       theCurrentShader->m_shader,
                                       static_cast<const QSSGApplyInstanceValue &>(*theCommand));
                break;
            case CommandType::ApplyValue:
                if (theCurrentShader)
                    applyValue(inEffect, inClass, theCurrentShader->m_shader, static_cast<const QSSGApplyValue &>(*theCommand));
                break;
            case CommandType::ApplyBlending:
                intermediateBlendingEnabled = applyBlending(static_cast<const QSSGApplyBlending &>(*theCommand));
                break;
            case CommandType::ApplyBufferValue:
                if (theCurrentShader)
                    theCurrentSourceTexture = applyBufferValue(inEffect,
                                                               theCurrentShader->m_shader,
                                                               static_cast<const QSSGApplyBufferValue &>(*theCommand),
                                                               inSourceTexture,
                                                               theCurrentSourceTexture);
                break;
            case CommandType::ApplyDepthValue:
                if (theCurrentShader)
                    applyDepthValue(inEffect, theCurrentShader->m_shader, static_cast<const QSSGApplyDepthValue &>(*theCommand), inDepthTexture);
                if (!inDepthTexture) {
                    qCCritical(INVALID_OPERATION,
                               "Depth value command detected but no "
                               "depth buffer provided for effect %s",
                               inEffect->className);
                    Q_ASSERT(false);
                }
                break;
            case CommandType::ApplyImageValue:
                if (theCurrentShader)
                    applyImageValue(inEffect, theCurrentShader->m_shader, static_cast<const QSSGApplyImageValue &>(*theCommand));
                break;
            case CommandType::ApplyDataBufferValue:
                if (theCurrentShader)
                    applyDataBufferValue(inEffect,
                                         theCurrentShader->m_shader,
                                         static_cast<const QSSGApplyDataBufferValue &>(*theCommand));
                break;
            case CommandType::DepthStencil: {
                const QSSGDepthStencil &theDepthStencil = static_cast<const QSSGDepthStencil &>(*theCommand);
                theCurrentDepthStencilTexture = findTexture(inEffect, theDepthStencil.m_bufferName);
                if (theCurrentDepthStencilTexture)
                    theCurrentDepthStencil = theDepthStencil;
            } break;
            case CommandType::Render:
                if (theCurrentShader && theCurrentSourceTexture.texture) {
                    renderPass(*theCurrentShader,
                               theMVP,
                               theCurrentSourceTexture,
                               theCurrentRenderTarget,
                               theDestSize,
                               inCameraClipRange,
                               theCurrentDepthStencilTexture,
                               theCurrentDepthStencil,
                               static_cast<const QSSGRender &>(*theCommand).m_drawIndirect);
                }
                // Reset the source texture regardless
                theCurrentSourceTexture = QSSGEffectTextureData(inSourceTexture, false);
                theCurrentDepthStencilTexture = nullptr;
                theCurrentDepthStencil = QSSGOption<QSSGDepthStencil>();
                // reset intermediate blending state
                if (intermediateBlendingEnabled) {
                    theContext->setBlendingEnabled(false);
                    intermediateBlendingEnabled = false;
                }
                break;
            case CommandType::ApplyRenderState:
                applyRenderStateValue(theCurrentRenderTarget.data(),
                                      inDepthStencilTexture,
                                      static_cast<const QSSGApplyRenderState &>(*theCommand));
                break;
            default:
                Q_ASSERT(false);
                break;
            }
        }

        setEffectRequiresCompilation(inEffect->className, false);

        // reset to default stencil state
        if (inDepthStencilTexture)
            theContext->setDepthStencilState(m_defaultStencilState);

        // Release any per-frame buffers
        if (inEffect->m_context) {
            QSSGEffectContext &theContext(*inEffect->m_context);
            // Query for size on every loop intentional
            for (qint32 idx = 0; idx < theContext.m_allocatedBuffers.size(); ++idx) {
                if (theContext.m_allocatedBuffers[idx].flags.isSceneLifetime() == false) {
                    theContext.releaseBuffer(idx);
                    --idx;
                }
            }
            for (qint32 idx = 0; idx < theContext.m_allocatedImages.size(); ++idx) {
                if (theContext.m_allocatedImages[idx].flags.isSceneLifetime() == false) {
                    theContext.releaseImage(idx);
                    --idx;
                }
            }
        }
    }
}

QSSGRef<QSSGRenderTexture2D> QSSGEffectSystem::renderEffect(QSSGEffectRenderArgument inRenderArgument)
{
    auto theClass = getEffectClass(inRenderArgument.m_effect->className);
    if (!theClass) {
        Q_ASSERT(false);
        return nullptr;
    }
    QMatrix4x4 theMVP;
    QSSGRenderCamera::setupOrthographicCameraForOffscreenRender(*inRenderArgument.m_colorBuffer, theMVP);
    // setup a render target
    const auto &theContext(m_context->renderContext());
    const auto &theManager(m_context->resourceManager());
    QSSGRenderContextScopedProperty<QSSGRef<QSSGRenderFrameBuffer>> __framebuffer(*theContext,
                                                                                        &QSSGRenderContext::renderTarget,
                                                                                        &QSSGRenderContext::setRenderTarget);
    QSSGTextureDetails theDetails(inRenderArgument.m_colorBuffer->textureDetails());
    quint32 theFinalWidth = QSSGRendererUtil::nextMultipleOf4((quint32)(theDetails.width));
    quint32 theFinalHeight = QSSGRendererUtil::nextMultipleOf4((quint32)(theDetails.height));
    auto theBuffer = theManager->allocateFrameBuffer();
    // UdoL Some Effects may need to run before HDR tonemap. This means we need to keep the
    // input format
    QSSGRenderTextureFormat theOutputFormat = QSSGRenderTextureFormat::RGBA8;
    // TODO:
    //        if (theClass->dynamicClass->getOutputTextureFormat() == QSSGRenderTextureFormat::Unknown)
    //            theOutputFormat = theDetails.format;
    auto theTargetTexture = theManager->allocateTexture2D(theFinalWidth, theFinalHeight, theOutputFormat);
    theBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, theTargetTexture);
    theContext->setRenderTarget(theBuffer);
    QSSGRenderContextScopedProperty<QRect> __viewport(*theContext,
                                                        &QSSGRenderContext::viewport,
                                                        &QSSGRenderContext::setViewport,
                                                        QRect(0, 0, theFinalWidth, theFinalHeight));

    QSSGRenderContextScopedProperty<bool> __scissorEnable(*theContext,
                                                            &QSSGRenderContext::isScissorTestEnabled,
                                                            &QSSGRenderContext::setScissorTestEnabled,
                                                            false);

    doRenderEffect(inRenderArgument.m_effect,
                   theClass,
                   inRenderArgument.m_colorBuffer,
                   theMVP,
                   m_context->renderContext()->renderTarget(),
                   false,
                   inRenderArgument.m_depthTexture,
                   inRenderArgument.m_depthStencilBuffer,
                   inRenderArgument.m_cameraClipRange);

    theBuffer->attach(QSSGRenderFrameBufferAttachment::Color0, QSSGRenderTextureOrRenderBuffer());
    theManager->release(theBuffer);
    return theTargetTexture;
}

bool QSSGEffectSystem::renderEffect(QSSGEffectRenderArgument inRenderArgument, QMatrix4x4 &inMVP, bool inEnableBlendWhenRenderToTarget)
{
    auto theClass = getEffectClass(inRenderArgument.m_effect->className);
    if (!theClass) {
        Q_ASSERT(false);
        return false;
    }

    doRenderEffect(inRenderArgument.m_effect,
                   theClass,
                   inRenderArgument.m_colorBuffer,
                   inMVP,
                   m_context->renderContext()->renderTarget(),
                   inEnableBlendWhenRenderToTarget,
                   inRenderArgument.m_depthTexture,
                   inRenderArgument.m_depthStencilBuffer,
                   inRenderArgument.m_cameraClipRange);
    return true;
}

void QSSGEffectSystem::releaseEffectContext(QSSGEffectContext *inContext)
{
    if (inContext == nullptr)
        return;
    for (quint32 idx = 0, end = m_contexts.size(); idx < end; ++idx) {
        if (m_contexts[idx] == inContext) {
            { // replace_with_last
                m_contexts[idx] = m_contexts.back();
                m_contexts.pop_back();
            }
        }
    }
}

void QSSGEffectSystem::resetEffectFrameData(QSSGEffectContext &inContext)
{ // Query for size on every loop intentional
    for (qint32 idx = 0; idx < inContext.m_allocatedBuffers.size(); ++idx) {
        QSSGAllocatedBufferEntry &theBuffer(inContext.m_allocatedBuffers[idx]);
        if (theBuffer.flags.isSceneLifetime() == true)
            theBuffer.needsClear = true;
    }
    for (qint32 idx = 0; idx < inContext.m_allocatedDataBuffers.size(); ++idx) {
        QSSGAllocatedDataBufferEntry &theDataBuffer(inContext.m_allocatedDataBuffers[idx]);
        if (theDataBuffer.flags.isSceneLifetime() == true)
            theDataBuffer.needsClear = true;
    }
}

void QSSGEffectSystem::setShaderData(const QByteArray &path, const char *data, const char *inShaderType, const char *inShaderVersion, bool inHasGeomShader, bool inIsComputeShader)
{
    m_context->dynamicObjectSystem()->setShaderData(path, data, inShaderType, inShaderVersion, inHasGeomShader, inIsComputeShader);
}

void QSSGEffectSystem::init()
{
    const auto &theContext(m_context->renderContext());

    m_resourceManager = m_context->resourceManager();

    // create default stencil state
    QSSGRenderStencilFunction stencilDefaultFunc(QSSGRenderBoolOp::AlwaysTrue, 0x0, 0xFF);
    QSSGRenderStencilOperation stencilDefaultOp(QSSGRenderStencilOp::Keep,
                                                  QSSGRenderStencilOp::Keep,
                                                  QSSGRenderStencilOp::Keep);
    m_defaultStencilState = new QSSGRenderDepthStencilState(theContext,
                                                              theContext->isDepthTestEnabled(),
                                                              theContext->isDepthWriteEnabled(),
                                                              theContext->depthFunction(),
                                                              theContext->isStencilTestEnabled(),
                                                              stencilDefaultFunc,
                                                              stencilDefaultFunc,
                                                              stencilDefaultOp,
                                                              stencilDefaultOp);
}

QSSGRef<QSSGResourceManager> QSSGEffectSystem::getResourceManager() { return m_resourceManager; }

QSSGEffectTextureData::QSSGEffectTextureData(const QSSGRef<QSSGRenderTexture2D> &inTexture, bool inNeedsMultiply)
    : texture(inTexture), needsAlphaMultiply(inNeedsMultiply)
{
}

QT_END_NAMESPACE
