// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "graphobjects/qssgrendergraphobject_p.h"
#include "qssgrendershadercache_p.h"
#include "qssgrendercontextcore_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qquick3dprofiler_p.h>

#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>
#include <qtquick3d_tracepoints_p.h>

#include <QCoreApplication>
#include <QStandardPaths>
#include <QString>
#include <QFile>
#include <QDir>

#include <QtGui/qsurfaceformat.h>
#if QT_CONFIG(opengl)
# include <QtGui/qopenglcontext.h>
#endif

#ifdef QT_QUICK3D_HAS_RUNTIME_SHADERS
#include <rhi/qshaderbaker.h>
#endif

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

Q_TRACE_POINT(qtquick3d, QSSG_loadShader_entry)
Q_TRACE_POINT(qtquick3d, QSSG_loadShader_exit)

static QtQuick3DEditorHelpers::ShaderBaker::StatusCallback s_statusCallback = nullptr;
Q_GLOBAL_STATIC(QMutex, s_statusMutex);

size_t qHash(QSSGShaderFeatures features) noexcept { return (features.flags & (~QSSGShaderFeatures::IndexMask)); }

static QString dumpFilename(QShader::Stage stage)
{
    switch (stage) {
    case QShader::VertexStage:
        return QStringLiteral("failedvert.txt");
        break;
    case QShader::FragmentStage:
        return QStringLiteral("failedfrag.txt");
        break;
    default:
        return QStringLiteral("failedshader.txt");
    }
}

struct DefineEntry
{
    const char *name = nullptr;
    QSSGShaderFeatures::Feature feature {};
};

static constexpr DefineEntry DefineTable[] {
    { "QSSG_ENABLE_LIGHT_PROBE", QSSGShaderFeatures::Feature::LightProbe },
    { "QSSG_ENABLE_IBL_ORIENTATION", QSSGShaderFeatures::Feature::IblOrientation },
    { "QSSG_ENABLE_SSM", QSSGShaderFeatures::Feature::Ssm },
    { "QSSG_ENABLE_SSAO", QSSGShaderFeatures::Feature::Ssao },
    { "QSSG_ENABLE_DEPTH_PASS", QSSGShaderFeatures::Feature::DepthPass },
    { "QSSG_ENABLE_ORTHO_SHADOW_PASS", QSSGShaderFeatures::Feature::OrthoShadowPass },
    { "QSSG_ENABLE_CUBE_SHADOW_PASS", QSSGShaderFeatures::Feature::CubeShadowPass },
    { "QSSG_ENABLE_LINEAR_TONEMAPPING", QSSGShaderFeatures::Feature::LinearTonemapping },
    { "QSSG_ENABLE_ACES_TONEMAPPING", QSSGShaderFeatures::Feature::AcesTonemapping },
    { "QSSG_ENABLE_HEJLDAWSON_TONEMAPPING", QSSGShaderFeatures::Feature::HejlDawsonTonemapping },
    { "QSSG_ENABLE_FILMIC_TONEMAPPING", QSSGShaderFeatures::Feature::FilmicTonemapping },
    { "QSSG_ENABLE_RGBE_LIGHT_PROBE", QSSGShaderFeatures::Feature::RGBELightProbe },
    { "QSSG_ENABLE_OPAQUE_DEPTH_PRE_PASS", QSSGShaderFeatures::Feature::OpaqueDepthPrePass },
    { "QSSG_ENABLE_REFLECTION_PROBE", QSSGShaderFeatures::Feature::ReflectionProbe },
    { "QSSG_REDUCE_MAX_NUM_LIGHTS", QSSGShaderFeatures::Feature::ReduceMaxNumLights },
    { "QSSG_ENABLE_LIGHTMAP", QSSGShaderFeatures::Feature::Lightmap }
};

static_assert(std::size(DefineTable) == QSSGShaderFeatures::Count, "Missing feature define?");

const char *QSSGShaderFeatures::asDefineString(QSSGShaderFeatures::Feature feature) { return DefineTable[static_cast<FlagType>(feature) & QSSGShaderFeatures::IndexMask].name; }
QSSGShaderFeatures::Feature QSSGShaderFeatures::fromIndex(quint32 idx) { return DefineTable[idx].feature; }

void QSSGShaderFeatures::set(QSSGShaderFeatures::Feature feature, bool val)
{
    if (val)
        flags |= (static_cast<FlagType>(feature) & ~IndexMask);
    else
        flags &= ~(static_cast<FlagType>(feature) & ~IndexMask);
}

#ifdef QT_QUICK3D_HAS_RUNTIME_SHADERS
static void initBakerForNonPersistentUse(QShaderBaker *baker, QRhi *rhi)
{
    QVector<QShaderBaker::GeneratedShader> outputs;
    switch (rhi->backend()) {
    case QRhi::D3D11:
    case QRhi::D3D12:
        outputs.append({ QShader::HlslShader, QShaderVersion(50) }); // Shader Model 5.0
        break;
    case QRhi::Metal:
        outputs.append({ QShader::MslShader, QShaderVersion(12) }); // Metal 1.2
        break;
    case QRhi::OpenGLES2:
    {
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
#if QT_CONFIG(opengl)
        auto h = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
        if (h && h->context)
            format = h->context->format();
#endif
        if (format.profile() == QSurfaceFormat::CoreProfile && format.version() >= qMakePair(3, 3)) {
            outputs.append({ QShader::GlslShader, QShaderVersion(330) }); // OpenGL 3.3+
        } else {
            bool isGLESModule = false;
#if QT_CONFIG(opengl)
            isGLESModule = QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES;
#endif
            if (format.renderableType() == QSurfaceFormat::OpenGLES || isGLESModule) {
                if (format.majorVersion() >= 3)
                    outputs.append({ QShader::GlslShader, QShaderVersion(300, QShaderVersion::GlslEs) }); // GLES 3.0+
                else
                    outputs.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) }); // GLES 2.0
            } else {
                // Need to default to at least GLSL 130 (OpenGL 3.0), not 120.
                // The difference is actually relevant when it comes to certain
                // GLSL features (textureSize, unsigned integers, and with
                // SPIRV-Cross even bool), and we do not have to care about
                // pure OpenGL (non-ES) 2.x implementations in practice.

                // For full feature set we need GLSL 140 (OpenGL 3.1), e.g.
                // because of inverse() used for instancing.

                // GLSL 130 should still be attempted, to support old Mesa
                // llvmpipe that only gives us OpenGL 3.0. At the time of
                // writing the opengl32sw.dll shipped with pre-built Qt is one
                // of these still.

                if (format.version() >= qMakePair(3, 1))
                    outputs.append({ QShader::GlslShader, QShaderVersion(140) }); // OpenGL 3.1+
                else
                    outputs.append({ QShader::GlslShader, QShaderVersion(130) }); // OpenGL 3.0+
            }
        }
    }
        break;
    default: // Vulkan, Null
        outputs.append({ QShader::SpirvShader, QShaderVersion(100) });
        break;
    }

    baker->setGeneratedShaders(outputs);
    baker->setGeneratedShaderVariants({ QShader::StandardShader });
}

static void initBakerForPersistentUse(QShaderBaker *baker, QRhi *)
{
    QVector<QShaderBaker::GeneratedShader> outputs;
    outputs.reserve(8);
    outputs.append({ QShader::SpirvShader, QShaderVersion(100) });
    outputs.append({ QShader::HlslShader, QShaderVersion(50) }); // Shader Model 5.0
    outputs.append({ QShader::MslShader, QShaderVersion(12) }); // Metal 1.2
    outputs.append({ QShader::GlslShader, QShaderVersion(330) }); // OpenGL 3.3+
    outputs.append({ QShader::GlslShader, QShaderVersion(140) }); // OpenGL 3.1+
    outputs.append({ QShader::GlslShader, QShaderVersion(130) }); // OpenGL 3.0+
    outputs.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) }); // GLES 2.0
    outputs.append({ QShader::GlslShader, QShaderVersion(300, QShaderVersion::GlslEs) }); // GLES 3.0+

    // If one of the above cannot be generated due to failing at the
    // SPIRV-Cross translation stage, it will be skipped, but bake() will not
    // fail. This is essential, because with the default fail if anything fails
    // behavior many shaders could not be baked at all due to failing for e.g.
    // GLSL ES 100. This is a non-issue when choosing the targets dynamically
    // based on the current API/context, but here we need to ensure what we
    // generate will work with a different RHI backend, graphics API, and
    // perhaps even on a different platform (if the cache file is manually
    // moved). So have to generate what we can, without breaking the
    // application when the shader is not compatible with a target. (if that
    // shader is not used at runtime, it's fine anyway, it it is, it won't work
    // just as with the other, non-caching path)
    baker->setBreakOnShaderTranslationError(false);

    baker->setGeneratedShaders(outputs);
    baker->setGeneratedShaderVariants({ QShader::StandardShader });
}

#else
static void initBakerForNonPersistentUse(QShaderBaker *, QRhi *)
{
}

static void initBakerForPersistentUse(QShaderBaker *, QRhi *)
{
}
#endif // QT_QUICK3D_HAS_RUNTIME_SHADERS

static bool s_autoDiskCacheEnabled = true;

static bool isAutoDiskCacheEnabled()
{
    // these three mirror QOpenGLShaderProgram/QQuickGraphicsConfiguration/QSGRhiSupport
    static const bool diskCacheDisabled = qEnvironmentVariableIntValue("QT_DISABLE_SHADER_DISK_CACHE")
                                          || qEnvironmentVariableIntValue("QSG_RHI_DISABLE_DISK_CACHE");
    const bool attrDiskCacheDisabled = (qApp ? qApp->testAttribute(Qt::AA_DisableShaderDiskCache) : false);
    return (!diskCacheDisabled && !attrDiskCacheDisabled && s_autoDiskCacheEnabled);

}

static inline bool ensureWritableDir(const QString &name)
{
    QDir::root().mkpath(name);
    return QFileInfo(name).isWritable();
}

static QString persistentQsbcDir()
{
    static bool checked = false;
    static QString currentCacheDir;
    static bool cacheWritable = false;

    if (checked)
        return cacheWritable ? currentCacheDir : QString();

    checked = true;
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString subPath = QLatin1String("/q3dshadercache-") + QSysInfo::buildAbi() + QLatin1Char('/');

    if (!cachePath.isEmpty()) {
        currentCacheDir = cachePath + subPath;
        cacheWritable = ensureWritableDir(currentCacheDir);
    }

    return cacheWritable ? currentCacheDir : QString();
}

static inline QString persistentQsbcFileName()
{
    const QString cacheDir = persistentQsbcDir();
    if (!cacheDir.isEmpty())
        return cacheDir + QLatin1String("q3dshadercache.qsbc");

    return QString();
}

QSSGShaderCache::QSSGShaderCache(QSSGRhiContext &ctx,
                                 const InitBakerFunc initBakeFn)
    : m_rhiContext(ctx),
      m_initBaker(initBakeFn)
{
    if (isAutoDiskCacheEnabled()) {
        const bool shaderDebug = !QSSGRhiContext::editorMode() && QSSGRhiContext::shaderDebuggingEnabled();
        m_persistentShaderStorageFileName = persistentQsbcFileName();
        if (!m_persistentShaderStorageFileName.isEmpty()) {
            const bool skipCacheFile = qEnvironmentVariableIntValue("QT_QUICK3D_NO_SHADER_CACHE_LOAD");
            if (!skipCacheFile && QFileInfo::exists(m_persistentShaderStorageFileName)) {
                if (shaderDebug)
                    qDebug("Attempting to seed material shader cache from %s", qPrintable(m_persistentShaderStorageFileName));
                if (m_persistentShaderBakingCache.load(m_persistentShaderStorageFileName)) {
                    if (shaderDebug) {
                        const int count = m_persistentShaderBakingCache.availableEntries().count();
                        qDebug("Loaded %d shader pipelines into the material shader cache", count);
                    }
                }
            }
        }
    }

    if (!m_initBaker) {
        // It is important to generate all possible shader variants if the qsb
        // collection is going to be stored on disk. Otherwise switching the
        // rhi backend could break the application. This is however an overkill
        // if we know that what we bake will not be reused in future runs of
        // the application, so do not do it if the disk cache was disabled or
        // the cache directory was not available (no file system, no
        // permissions, etc.).
        m_initBaker = m_persistentShaderStorageFileName.isEmpty() ? initBakerForNonPersistentUse
                                                                  : initBakerForPersistentUse;
    }
}

QSSGShaderCache::~QSSGShaderCache()
{
    if (!m_persistentShaderStorageFileName.isEmpty())
        m_persistentShaderBakingCache.save(m_persistentShaderStorageFileName);
}

void QSSGShaderCache::releaseCachedResources()
{
    m_rhiShaders.clear();

    // m_persistentShaderBakingCache is not cleared, that is intentional,
    // otherwise we would permanently lose what got loaded at startup.
}

QSSGRhiShaderPipelinePtr QSSGShaderCache::tryGetRhiShaderPipeline(const QByteArray &inKey,
                                                                        const QSSGShaderFeatures &inFeatures)
{
    QSSGShaderCacheKey cacheKey(inKey);
    cacheKey.m_features = inFeatures;
    cacheKey.updateHashCode();
    const auto theIter = m_rhiShaders.constFind(cacheKey);
    if (theIter != m_rhiShaders.cend())
        return theIter.value();
    return nullptr;
}


void QSSGShaderCache::addShaderPreprocessor(QByteArray &str,
                                            const QByteArray &inKey,
                                            ShaderType shaderType,
                                            const QSSGShaderFeatures &inFeatures)
{
    m_insertStr.clear();

    m_insertStr += "#version 440\n";

    if (!inKey.isNull()) {
        m_insertStr += "//Shader name -";
        m_insertStr += inKey;
        m_insertStr += "\n";
    }

    m_insertStr += "#define texture2D texture\n";

    str.insert(0, m_insertStr);
    QString::size_type insertPos = int(m_insertStr.size());

    m_insertStr.clear();
    const bool fragOutputEnabled = (!inFeatures.isSet(QSSGShaderFeatures::Feature::DepthPass)) && shaderType == ShaderType::Fragment;
    for (const auto &def : DefineTable) {
        m_insertStr.append("#define ");
        m_insertStr.append(def.name);
        m_insertStr.append(" ");
        m_insertStr.append(inFeatures.isSet(def.feature) ? "1" : "0");
        m_insertStr.append("\n");
    }

    str.insert(insertPos, m_insertStr);
    insertPos += int(m_insertStr.size());

    m_insertStr.clear();
    if (fragOutputEnabled)
        m_insertStr += "layout(location = 0) out vec4 fragOutput;\n";

    str.insert(insertPos, m_insertStr);
}

QByteArray QSSGShaderCache::resourceFolder()
{
    return QByteArrayLiteral(":/res/rhishaders/");
}

QByteArray QSSGShaderCache::shaderCollectionFile()
{
    return QByteArrayLiteral("qtappshaders.qsbc");
}

QSSGRhiShaderPipelinePtr QSSGShaderCache::compileForRhi(const QByteArray &inKey, const QByteArray &inVert, const QByteArray &inFrag,
                                                        const QSSGShaderFeatures &inFeatures, QSSGRhiShaderPipeline::StageFlags stageFlags)
{
#ifdef QT_QUICK3D_HAS_RUNTIME_SHADERS
    const QSSGRhiShaderPipelinePtr &rhiShaders = tryGetRhiShaderPipeline(inKey, inFeatures);
    if (rhiShaders)
        return rhiShaders;

    QSSGShaderCacheKey tempKey(inKey);
    tempKey.m_features = inFeatures;
    tempKey.updateHashCode();

    QByteArray vertexCode = inVert;
    QByteArray fragmentCode = inFrag;

    if (!vertexCode.isEmpty())
        addShaderPreprocessor(vertexCode, inKey, ShaderType::Vertex, inFeatures);

    if (!fragmentCode.isEmpty())
        addShaderPreprocessor(fragmentCode, inKey, ShaderType::Fragment, inFeatures);

    // lo and behold the final shader strings are ready

    QSSGRhiShaderPipelinePtr shaders;
    QString vertErr, fragErr;

    QShaderBaker baker;
    m_initBaker(&baker, m_rhiContext.rhi());

    const bool editorMode = QSSGRhiContext::editorMode();
    // Shader debug is disabled in editor mode
    const bool shaderDebug = !editorMode && QSSGRhiContext::shaderDebuggingEnabled();

   static auto dumpShader = [](QShader::Stage stage, const QByteArray &code) {
       switch (stage) {
       case QShader::Stage::VertexStage:
           qDebug("VERTEX SHADER:\n*****\n");
           break;
       case QShader::Stage::FragmentStage:
           qDebug("FRAGMENT SHADER:\n*****\n");
           break;
       default:
           qDebug("SHADER:\n*****\n");
           break;
       }
       const auto lines = code.split('\n');
       for (int i = 0; i < lines.size(); i++)
           qDebug("%3d  %s", i + 1, lines.at(i).constData());
       qDebug("\n*****\n");
   };

   static auto dumpShaderToFile = [](QShader::Stage stage, const QByteArray &data) {
       QFile f(dumpFilename(stage));
       f.open(QIODevice::WriteOnly | QIODevice::Text);
       f.write(data);
       f.close();
   };

    baker.setSourceString(vertexCode, QShader::VertexStage);
    QShader vertexShader = baker.bake();
    const auto vertShaderValid = vertexShader.isValid();
    if (!vertShaderValid) {
        vertErr = baker.errorMessage();
        if (!editorMode) {
            qWarning("Failed to compile vertex shader:\n");
            if (!shaderDebug)
                qWarning() << inKey << '\n' << vertErr;
        }
    }

    if (shaderDebug) {
        dumpShader(QShader::Stage::VertexStage, vertexCode);
        if (!vertShaderValid)
            dumpShaderToFile(QShader::Stage::VertexStage, vertexCode);
    }

    baker.setSourceString(fragmentCode, QShader::FragmentStage);
    QShader fragmentShader = baker.bake();
    const bool fragShaderValid = fragmentShader.isValid();
    if (!fragShaderValid) {
        fragErr = baker.errorMessage();
        if (!editorMode) {
            qWarning("Failed to compile fragment shader \n");
            if (!shaderDebug)
                qWarning() << inKey << '\n' << fragErr;
        }
    }

    if (shaderDebug) {
        dumpShader(QShader::Stage::FragmentStage, fragmentCode);
        if (!fragShaderValid)
            dumpShaderToFile(QShader::Stage::FragmentStage, fragmentCode);
    }

    if (vertShaderValid && fragShaderValid) {
        shaders = std::make_shared<QSSGRhiShaderPipeline>(m_rhiContext);
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader), stageFlags);
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader), stageFlags);
        if (shaderDebug)
            qDebug("Compilation for vertex and fragment stages succeeded");
    }

    if (editorMode && s_statusCallback) {
        using namespace QtQuick3DEditorHelpers::ShaderBaker;
        const auto vertStatus = vertShaderValid ? Status::Success : Status::Error;
        const auto fragStatus = fragShaderValid ? Status::Success : Status::Error;
        QMutexLocker locker(&*s_statusMutex);
        s_statusCallback(inKey, vertStatus, vertErr, QShader::VertexStage);
        s_statusCallback(inKey, fragStatus, fragErr, QShader::FragmentStage);
    }

    auto result = m_rhiShaders.insert(tempKey, shaders).value();
    if (result && result->vertexStage() && result->fragmentStage()) {
        QQsbCollection::EntryDesc entryDesc = {
            inKey,
            QQsbCollection::toFeatureSet(inFeatures),
            result->vertexStage()->shader(),
            result->fragmentStage()->shader()
        };
        m_persistentShaderBakingCache.addEntry(entryDesc.generateSha(), entryDesc);
    }
    return result;

#else
    Q_UNUSED(inKey);
    Q_UNUSED(inVert);
    Q_UNUSED(inFrag);
    Q_UNUSED(inFeatures);
    Q_UNUSED(stageFlags);
    qWarning("Cannot compile and condition shaders at runtime because this build of Qt Quick 3D is not linking to Qt Shader Tools. "
             "Only pre-processed materials are supported.");
    return {};
#endif
}

QSSGRhiShaderPipelinePtr QSSGShaderCache::newPipelineFromPregenerated(const QByteArray &inKey,
                                                                      const QSSGShaderFeatures &inFeatures,
                                                                      QQsbCollection::Entry entry,
                                                                      const QSSGRenderGraphObject &obj,
                                                                      QSSGRhiShaderPipeline::StageFlags stageFlags)
{
    // No lookup in m_rhiShaders. It is up to the caller to do that, if they
    // want to. We will insert into it at the end, but there is intentionally
    // no lookup. The result from this function is always a new
    // QSSGRhiShaderPipeline (it's just much faster to create than the
    // full-blown generator). That is important for some clients (effect
    // system) so returning an existing QSSGRhiShaderPipeline is _wrong_.

    const bool shaderDebug = !QSSGRhiContext::editorMode() && QSSGRhiContext::shaderDebuggingEnabled();
    if (shaderDebug)
        qDebug("Loading pregenerated rhi shader(s)");

    Q_TRACE_SCOPE(QSSG_loadShader);
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DLoadShader);

    // Note that we are required to return a non-null (but empty) shader set even if loading fails.
    QSSGRhiShaderPipelinePtr shaders(new QSSGRhiShaderPipeline(m_rhiContext));

    const QString collectionFile = QString::fromLatin1(resourceFolder() + shaderCollectionFile());

    QQsbIODeviceCollection qsbc(collectionFile);
    QQsbCollection::EntryDesc entryDesc;
    if (qsbc.map(QQsbIODeviceCollection::Read))
        qsbc.extractEntry(entry, entryDesc);
    else
        qWarning("Failed to open entry %s", entry.key.constData());

    if (entryDesc.vertShader.isValid() && entryDesc.fragShader.isValid()) {
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, entryDesc.vertShader), stageFlags);
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, entryDesc.fragShader), stageFlags);
        if (shaderDebug)
            qDebug("Loading of vertex and fragment stages succeeded");
    }

    Q_QUICK3D_PROFILE_END_WITH_ID(QQuick3DProfiler::Quick3DLoadShader, 0, obj.profilingId);

    QSSGShaderCacheKey cacheKey(inKey);
    cacheKey.m_features = inFeatures;
    cacheKey.updateHashCode();

    const auto inserted = m_rhiShaders.insert(cacheKey, shaders);
    qsbc.unmap();
    return inserted.value();
}

QSSGRhiShaderPipelinePtr QSSGShaderCache::tryNewPipelineFromPersistentCache(const QByteArray &qsbcKey,
                                                                            const QByteArray &inKey,
                                                                            const QSSGShaderFeatures &inFeatures,
                                                                            QSSGRhiShaderPipeline::StageFlags stageFlags)
{
    // No lookup in m_rhiShaders. it is up to the caller to do that, if they
    // want to. We will insert into it at the end, but there is intentionally
    // no lookup. The result from this function is always a new
    // QSSGRhiShaderPipeline (it's just much faster to create than the
    // full-blown generator). That is important for some clients (effect
    // system) so returning an existing QSSGRhiShaderPipeline is _wrong_.

    QQsbCollection::EntryDesc entryDesc;

    // Here we are allowed to return null to indicate that there is no such
    // entry in this particular cache.
    if (!m_persistentShaderBakingCache.extractEntry(QQsbCollection::Entry(qsbcKey), entryDesc))
        return {};

    if (entryDesc.vertShader.isValid() && entryDesc.fragShader.isValid()) {
        const bool shaderDebug = !QSSGRhiContext::editorMode() && QSSGRhiContext::shaderDebuggingEnabled();
        if (shaderDebug)
            qDebug("Loading rhi shaders from disk cache for %s (%s)", qsbcKey.constData(), inKey.constData());

        QSSGRhiShaderPipelinePtr shaders(new QSSGRhiShaderPipeline(m_rhiContext));
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, entryDesc.vertShader), stageFlags);
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, entryDesc.fragShader), stageFlags);
        QSSGShaderCacheKey cacheKey(inKey);
        cacheKey.m_features = inFeatures;
        cacheKey.updateHashCode();
        return m_rhiShaders.insert(cacheKey, shaders).value();
    }

    return {};
}

QSSGRhiShaderPipelinePtr QSSGShaderCache::loadBuiltinForRhi(const QByteArray &inKey)
{
    const QSSGRhiShaderPipelinePtr &rhiShaders = tryGetRhiShaderPipeline(inKey, QSSGShaderFeatures());
    if (rhiShaders)
        return rhiShaders;

    const bool shaderDebug = !QSSGRhiContext::editorMode() && QSSGRhiContext::shaderDebuggingEnabled();
    if (shaderDebug)
        qDebug("Loading builtin rhi shader: %s", inKey.constData());

    Q_TRACE_SCOPE(QSSG_loadShader);
    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DLoadShader);

    // Note that we are required to return a non-null (but empty) shader set even if loading fails.
    QSSGRhiShaderPipelinePtr shaders(new QSSGRhiShaderPipeline(m_rhiContext));

    // inShaderName is a prefix of a .qsb file, so "abc" means we should
    // look for abc.vert.qsb and abc.frag.qsb.

    const QString prefix = QString::fromUtf8(resourceFolder() + inKey);
    const QString vertexFileName = prefix + QLatin1String(".vert.qsb");
    const QString fragmentFileName = prefix + QLatin1String(".frag.qsb");

    QShader vertexShader;
    QShader fragmentShader;

    QFile f;
    f.setFileName(vertexFileName);
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray vsData = f.readAll();
        vertexShader = QShader::fromSerialized(vsData);
        f.close();
    } else {
        qWarning("Failed to open %s", qPrintable(f.fileName()));
    }
    f.setFileName(fragmentFileName);
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray fsData = f.readAll();
        fragmentShader = QShader::fromSerialized(fsData);
        f.close();
    } else {
        qWarning("Failed to open %s", qPrintable(f.fileName()));
    }

    if (vertexShader.isValid() && fragmentShader.isValid()) {
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader), QSSGRhiShaderPipeline::UsedWithoutIa);
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader));
        if (shaderDebug)
            qDebug("Loading of vertex and fragment stages succeeded");
    }

    Q_QUICK3D_PROFILE_END_WITH_STRING(QQuick3DProfiler::Quick3DLoadShader, 0, inKey);

    QSSGShaderCacheKey cacheKey(inKey);
    cacheKey.m_features = QSSGShaderFeatures();
    cacheKey.updateHashCode();

    const auto inserted = m_rhiShaders.insert(cacheKey, shaders);
    return inserted.value();
}

namespace QtQuick3DEditorHelpers {
void ShaderBaker::setStatusCallback(StatusCallback cb)
{
    QMutexLocker locker(&*s_statusMutex);
    s_statusCallback = cb;
}

void ShaderCache::setAutomaticDiskCache(bool enable)
{
    s_autoDiskCacheEnabled = enable;
}

bool ShaderCache::isAutomaticDiskCacheEnabled()
{
    return ::isAutoDiskCacheEnabled();
}

}

QT_END_NAMESPACE
