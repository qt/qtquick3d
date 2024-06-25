// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSG_RENDER_SHADER_CACHE_H
#define QSSG_RENDER_SHADER_CACHE_H

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
#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtQuick3DUtils/private/qqsbcollection_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimplshaders_p.h>

#include <QtCore/QString>
#include <QtCore/qcryptographichash.h>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE

class QSSGRenderContextInterface;
class QSSGRhiShaderPipeline;
class QShaderBaker;
class QRhi;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderFeatures
{

// There are a number of macros used to turn on or off various features.
// This allows those features to be propagated into the shader cache's caching mechanism.
// They will be translated into #define name value where value is 1 or zero depending on
// if the feature is enabled or not.
//
// In snippets that use this feature the code would look something like this:

/*
#ifndef QSSG_ENABLE_<FEATURE>
#define QSSG_ENABLE_<FEATURE> 0
#endif

void func()
{
    ...
#if QSSG_ENABLE_<FEATURE>
     ...
#endif
     ...
}
*/

// NOTE: The order of these will affect generated keys, so re-ordering these
// will break already baked shaders (e.g. shadergen).
using FlagType = quint32;
enum class Feature : FlagType
{
    LightProbe = (1 << 8),
    IblOrientation = (1 << 9) + 1,
    Ssm = (1 << 10) + 2,
    Ssao = (1 << 11) + 3,
    DepthPass = (1 << 12) + 4,
    OrthoShadowPass = (1 << 13) + 5,
    PerspectiveShadowPass = (1 << 14) + 6,
    LinearTonemapping = (1 << 15) + 7,
    AcesTonemapping = (1 << 16) + 8,
    HejlDawsonTonemapping = (1 << 17) + 9,
    FilmicTonemapping = (1 << 18) + 10,
    RGBELightProbe = (1 << 19) + 11,
    OpaqueDepthPrePass = (1 << 20) + 12,
    ReflectionProbe = (1 << 21) + 13,
    ReduceMaxNumLights = (1 << 22) + 14,
    Lightmap = (1 << 23) + 15,
    DisableMultiView = (1 << 24) + 16,
    ForceIblExposure = (1 << 25) + 17,

    LastFeature
};

static constexpr FlagType IndexMask = 0xff;
static constexpr quint32 Count = (static_cast<FlagType>(Feature::LastFeature) & IndexMask);

static const char *asDefineString(QSSGShaderFeatures::Feature feature);
static Feature fromIndex(quint32 idx);

constexpr bool isNull() const { return flags == 0; }
constexpr bool isSet(Feature feature) const { return (static_cast<FlagType>(feature) & flags); }
void set(Feature feature, bool val);

FlagType flags = 0;

inline friend bool operator==(QSSGShaderFeatures a, QSSGShaderFeatures b) { return a.flags == b.flags; }

void disableTonemapping()
{
    set(Feature::LinearTonemapping, false);
    set(Feature::AcesTonemapping, false);
    set(Feature::FilmicTonemapping, false);
    set(Feature::HejlDawsonTonemapping, false);
    set(Feature::ForceIblExposure, false);
}

inline friend QDebug operator<<(QDebug stream, const QSSGShaderFeatures &features)
{
    QVarLengthArray<const char *, Count> enabledFeatureStrings;
    for (quint32 idx = 0; idx < Count; ++idx) {
        const Feature feature = fromIndex(idx);
        if (features.isSet(feature))
            enabledFeatureStrings.append(asDefineString(feature));
    }
    stream.nospace() << "QSSGShaderFeatures(";
    for (int i = 0; i < enabledFeatureStrings.size(); ++i)
        stream.nospace() << (i > 0 ? ", " : "") << enabledFeatureStrings[i];
    stream.nospace() << ")";
    return stream;
}

};

Q_QUICK3DRUNTIMERENDER_EXPORT size_t qHash(QSSGShaderFeatures features) noexcept;

struct QSSGShaderCacheKey
{
    QByteArray m_key;
    QSSGShaderFeatures m_features;
    size_t m_hashCode = 0;

    explicit QSSGShaderCacheKey(const QByteArray &key = QByteArray()) : m_key(key), m_hashCode(0) {}

    QSSGShaderCacheKey(const QSSGShaderCacheKey &other) = default;
    QSSGShaderCacheKey &operator=(const QSSGShaderCacheKey &other) = default;

    static inline size_t generateHashCode(const QByteArray &key, QSSGShaderFeatures features)
    {
        return qHash(key) ^ qHash(features);
    }

    void updateHashCode()
    {
        m_hashCode = generateHashCode(m_key, m_features);
    }

    bool operator==(const QSSGShaderCacheKey &inOther) const
    {
        return m_key == inOther.m_key && m_features == inOther.m_features;
    }
};

inline size_t qHash(const QSSGShaderCacheKey &key)
{
    return key.m_hashCode;
}

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderCache
{
    Q_DISABLE_COPY(QSSGShaderCache)
public:
    enum class ShaderType
    {
        Vertex = 0,
        Fragment = 1
    };

    using InitBakerFunc = void (*)(QShaderBaker *baker, QRhi *rhi);
private:
    friend class QSSGBuiltInRhiShaderCache;

    typedef QHash<QSSGShaderCacheKey, QSSGRhiShaderPipelinePtr> TRhiShaderMap;
    QSSGRhiContext &m_rhiContext; // Not own, the RCI owns us and the QSSGRhiContext.
    TRhiShaderMap m_rhiShaders;
    QByteArray m_insertStr;   // member to potentially reuse the allocation after clear
    QByteArray m_cacheKeyStr; // same here
    InitBakerFunc m_initBaker;
    QQsbInMemoryCollection m_persistentShaderBakingCache;
    QString m_persistentShaderStorageFileName;
    QSSGBuiltInRhiShaderCache m_builtInShaders;

    QSSGRhiShaderPipelinePtr loadBuiltinUncached(const QByteArray &inKey, int viewCount);

    void addShaderPreprocessor(QByteArray &str,
                               const QByteArray &inKey,
                               ShaderType shaderType,
                               const QSSGShaderFeatures &inFeatures,
                               int viewCount);

public:
    QSSGShaderCache(QSSGRhiContext &ctx,
                    const InitBakerFunc initBakeFn = nullptr);
    ~QSSGShaderCache();

    void releaseCachedResources();

    QQsbInMemoryCollection &persistentShaderBakingCache() { return m_persistentShaderBakingCache; }

    QSSGRhiShaderPipelinePtr tryGetRhiShaderPipeline(const QByteArray &inKey,
                                                     const QSSGShaderFeatures &inFeatures);

    QSSGRhiShaderPipelinePtr tryNewPipelineFromPersistentCache(const QByteArray &qsbcKey,
                                                               const QByteArray &inKey,
                                                               const QSSGShaderFeatures &inFeatures,
                                                               QSSGRhiShaderPipeline::StageFlags stageFlags = {});

    QSSGRhiShaderPipelinePtr newPipelineFromPregenerated(const QByteArray &inKey,
                                                         const QSSGShaderFeatures &inFeatures,
                                                         QQsbCollection::Entry entry,
                                                         const QSSGRenderGraphObject &obj,
                                                         QSSGRhiShaderPipeline::StageFlags stageFlags = {});

    QSSGRhiShaderPipelinePtr compileForRhi(const QByteArray &inKey,
                                           const QByteArray &inVert,
                                           const QByteArray &inFrag,
                                           const QSSGShaderFeatures &inFeatures,
                                           QSSGRhiShaderPipeline::StageFlags stageFlags,
                                           int viewCount,
                                           bool perTargetCompilation);

    QSSGBuiltInRhiShaderCache &getBuiltInRhiShaders() { return m_builtInShaders; }

    static QByteArray resourceFolder();
    static QByteArray shaderCollectionFile();
};

namespace QtQuick3DEditorHelpers {
namespace ShaderBaker
{
    enum class Status : quint8
    {
        Success,
        Error
    };
    using StatusCallback = void(*)(const QByteArray &descKey, Status status, const QString &err, QShader::Stage stage);
    Q_QUICK3DRUNTIMERENDER_EXPORT void setStatusCallback(StatusCallback cb);
}

namespace ShaderCache
{
    // Used by DS and the QML puppet!
    // Note: Needs to be called before any QSSGShaderCache instance is created.
    Q_QUICK3DRUNTIMERENDER_EXPORT void setAutomaticDiskCache(bool enable);
    Q_QUICK3DRUNTIMERENDER_EXPORT bool isAutomaticDiskCacheEnabled();
}

}

QT_END_NAMESPACE

#endif
