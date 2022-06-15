// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrendershadercache_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>
#include <QtQuick3DUtils/private/qquick3dprofiler_p.h>

#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/qfile.h>

#include <QtGui/qsurfaceformat.h>
#if QT_CONFIG(opengl)
# include <QtGui/qopenglcontext.h>
# include <QtGui/private/qrhigles2_p.h>
#endif

#ifdef QT_QUICK3D_HAS_RUNTIME_SHADERS
#include <QtShaderTools/private/qshaderbaker_p.h>
#endif

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

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

size_t qHash(const QSSGShaderCacheKey &key)
{
    return key.m_hashCode;
}

#ifdef QT_QUICK3D_HAS_RUNTIME_SHADERS
static void initBaker(QShaderBaker *baker, QRhi *rhi)
{
    QVector<QShaderBaker::GeneratedShader> outputs;
    switch (rhi->backend()) {
    case QRhi::D3D11:
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
#else
static void initBaker(QShaderBaker *, QRhi *)
{
}
#endif // QT_QUICK3D_HAS_RUNTIME_SHADERS

QSSGShaderCache::~QSSGShaderCache() {}

QSSGShaderCache::QSSGShaderCache(const QSSGRef<QSSGRhiContext> &ctx,
                                 const InitBakerFunc initBakeFn)
    : m_rhiContext(ctx)
    , m_initBaker(initBakeFn ? initBakeFn : &initBaker)
{
}

QSSGRef<QSSGRhiShaderPipeline> QSSGShaderCache::getRhiShaderPipeline(const QByteArray &inKey,
                                                                     const QSSGShaderFeatures &inFeatures)
{
    m_tempKey.m_key = inKey;
    m_tempKey.m_features = inFeatures;
    m_tempKey.updateHashCode();
    const auto theIter = m_rhiShaders.constFind(m_tempKey);
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

QSSGRef<QSSGRhiShaderPipeline> QSSGShaderCache::compileForRhi(const QByteArray &inKey, const QByteArray &inVert, const QByteArray &inFrag,
                                                              const QSSGShaderFeatures &inFeatures, QSSGRhiShaderPipeline::StageFlags stageFlags)
{
#ifdef QT_QUICK3D_HAS_RUNTIME_SHADERS
    const QSSGRef<QSSGRhiShaderPipeline> &rhiShaders = getRhiShaderPipeline(inKey, inFeatures);
    if (rhiShaders)
        return rhiShaders;

    QSSGShaderCacheKey tempKey(inKey);
    tempKey.m_features = inFeatures;
    tempKey.updateHashCode();

    m_vertexCode = inVert;
    m_fragmentCode = inFrag;

    if (!m_vertexCode.isEmpty())
        addShaderPreprocessor(m_vertexCode, inKey, ShaderType::Vertex, inFeatures);

    if (!m_fragmentCode.isEmpty())
        addShaderPreprocessor(m_fragmentCode, inKey, ShaderType::Fragment, inFeatures);

    // lo and behold the final shader strings are ready

    QSSGRef<QSSGRhiShaderPipeline> shaders;
    QString vertErr, fragErr;

    QShaderBaker baker;
    m_initBaker(&baker, m_rhiContext->rhi());

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

    baker.setSourceString(m_vertexCode, QShader::VertexStage);
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
        dumpShader(QShader::Stage::VertexStage, m_vertexCode);
        if (!vertShaderValid)
            dumpShaderToFile(QShader::Stage::VertexStage, m_vertexCode);
    }

    baker.setSourceString(m_fragmentCode, QShader::FragmentStage);
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
        dumpShader(QShader::Stage::FragmentStage, m_fragmentCode);
        if (!fragShaderValid)
            dumpShaderToFile(QShader::Stage::FragmentStage, m_fragmentCode);
    }

    if (vertShaderValid && fragShaderValid) {
        shaders = new QSSGRhiShaderPipeline(*m_rhiContext.data());
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

    const auto inserted = m_rhiShaders.insert(tempKey, shaders);
    return inserted.value();

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

QSSGRef<QSSGRhiShaderPipeline> QSSGShaderCache::loadGeneratedShader(const QByteArray &inKey, QQsbCollection::Entry entry)
{
    const QSSGRef<QSSGRhiShaderPipeline> &rhiShaders = getRhiShaderPipeline(inKey, QSSGShaderFeatures());
    if (rhiShaders)
        return rhiShaders;

    const bool shaderDebug = QSSGRhiContext::shaderDebuggingEnabled();
    if (shaderDebug)
        qDebug("Loading pregenerated rhi shader(s)");

    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DLoadShader);

    // Note that we are required to return a non-null (but empty) shader set even if loading fails.
    QSSGRef<QSSGRhiShaderPipeline> shaders(new QSSGRhiShaderPipeline(*m_rhiContext.data()));

    const QString collectionFile = QString::fromLatin1(resourceFolder() + shaderCollectionFile());

    QShader vertexShader;
    QShader fragmentShader;

    QQsbCollection qsbc(collectionFile);
    QQsbShaderFeatureSet featureSet;
    if (qsbc.map(QQsbCollection::Read))
        qsbc.extractQsbEntry(entry, nullptr, &featureSet, &vertexShader, &fragmentShader);
    else
        qWarning("Failed to open entry %zu", entry.hkey);

    if (vertexShader.isValid() && fragmentShader.isValid()) {
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader));
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader));
        if (shaderDebug)
            qDebug("Loading of vertex and fragment stages succeeded");
    }

    Q_QUICK3D_PROFILE_END(QQuick3DProfiler::Quick3DLoadShader);

    QSSGShaderCacheKey cacheKey(inKey);
    cacheKey.m_features = QSSGShaderFeatures();
    cacheKey.updateHashCode();

    const auto inserted = m_rhiShaders.insert(cacheKey, shaders);
    qsbc.unmap();
    return inserted.value();
}

QSSGRef<QSSGRhiShaderPipeline> QSSGShaderCache::loadBuiltinForRhi(const QByteArray &inKey)
{
    const QSSGRef<QSSGRhiShaderPipeline> &rhiShaders = getRhiShaderPipeline(inKey, QSSGShaderFeatures());
    if (rhiShaders)
        return rhiShaders;

    const bool shaderDebug = QSSGRhiContext::shaderDebuggingEnabled();
    if (shaderDebug)
        qDebug("Loading builtin rhi shader: %s", inKey.constData());

    Q_QUICK3D_PROFILE_START(QQuick3DProfiler::Quick3DLoadShader);

    // Note that we are required to return a non-null (but empty) shader set even if loading fails.
    QSSGRef<QSSGRhiShaderPipeline> shaders(new QSSGRhiShaderPipeline(*m_rhiContext.data()));

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

    Q_QUICK3D_PROFILE_END(QQuick3DProfiler::Quick3DLoadShader);

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
}

QT_END_NAMESPACE
