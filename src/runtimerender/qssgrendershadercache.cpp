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

#include "qssgrendershadercache_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/qfile.h>

#include <QtGui/qsurfaceformat.h>
#if QT_CONFIG(opengl)
# include <QtGui/qopenglcontext.h>
#endif

#include <QtShaderTools/private/qshaderbaker_p.h>

QT_BEGIN_NAMESPACE

static const char *defineTable[QSSGShaderDefines::Count] {
    "QSSG_ENABLE_LIGHT_PROBE",
    "QSSG_ENABLE_IBL_ORIENTATION",
    "QSSG_ENABLE_SSM",
    "QSSG_ENABLE_SSAO",
    "QSSG_ENABLE_DEPTH_PASS",
    "QSSG_ENABLE_ORTHO_SHADOW_PASS",
    "QSSG_ENABLE_CUBE_SHADOW_PASS",
    "QSSG_ENABLE_LINEAR_TONEMAPPING",
    "QSSG_ENABLE_ACES_TONEMAPPING",
    "QSSG_ENABLE_HEJLDAWSON_TONEMAPPING",
    "QSSG_ENABLE_FILMIC_TONEMAPPING",
    "QSSG_ENABLE_RGBE_LIGHT_PROBE"
};

const char *QSSGShaderDefines::asString(QSSGShaderDefines::Define def) { return defineTable[def]; }

size_t qHash(const QSSGShaderCacheKey &key)
{
    return key.m_hashCode;
}

size_t hashShaderFeatureSet(const ShaderFeatureSetList &inFeatureSet)
{
    size_t retval(0);
    for (int idx = 0, end = inFeatureSet.size(); idx < end; ++idx) {
        // From previous implementation, it seems we need to ignore the order of the features.
        // But we need to bind the feature flag together with its name, so that the flags will
        // influence
        // the final hash not only by the true-value count.
        retval ^= (qHash(int(inFeatureSet.at(idx).feature)) ^ size_t(inFeatureSet.at(idx).enabled));
    }
    return retval;
}

static void initBaker(QShaderBaker *baker, QRhi::Implementation target)
{
    QVector<QShaderBaker::GeneratedShader> outputs;
    switch (target) {
    case QRhi::D3D11:
        outputs.append({ QShader::HlslShader, QShaderVersion(50) }); // Shader Model 5.0
        break;
    case QRhi::Metal:
        outputs.append({ QShader::MslShader, QShaderVersion(12) }); // Metal 1.2
        break;
    case QRhi::OpenGLES2:
    {
        const QSurfaceFormat format = QSurfaceFormat::defaultFormat();
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
                // Default to GLSL 130 (OpenGL 3.0), not 120. The difference is
                // actually relevant when it comes to certain GLSL features
                // (textureSize, unsigned integers, and with SPIRV-Cross even
                // bool), and we do not have to care about pure OpenGL (non-ES)
                // 2.x implementations in practice.
                outputs.append({ QShader::GlslShader, QShaderVersion(130) }); // OpenGL 3.0
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

QSSGShaderCache::~QSSGShaderCache() {}

QSSGShaderCache::QSSGShaderCache(const QSSGRef<QSSGRhiContext> &ctx,
                                 const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory,
                                 const InitBakerFunc initBakeFn)
    : m_rhiContext(ctx)
    , m_initBaker(initBakeFn ? initBakeFn : &initBaker)
    , m_inputStreamFactory(inInputStreamFactory)
{
}

QSSGRef<QSSGRhiShaderPipeline> QSSGShaderCache::getRhiShaderPipeline(const QByteArray &inKey,
                                                                     const ShaderFeatureSetList &inFeatures)
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
                                            const ShaderFeatureSetList &inFeatures)
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

    bool fragOutputEnabled = shaderType == ShaderType::Fragment;
    if (inFeatures.size()) {
        m_insertStr.clear();
        for (int idx = 0, end = inFeatures.size(); idx < end; ++idx) {
            QSSGShaderPreprocessorFeature feature(inFeatures[idx]);
            m_insertStr.append("#define ");
            m_insertStr.append(inFeatures[idx].name);
            m_insertStr.append(" ");
            m_insertStr.append(feature.enabled ? "1" : "0");
            m_insertStr.append("\n");
            if (feature.enabled && inFeatures[idx].name == QSSGShaderDefines::asString(QSSGShaderDefines::DepthPass))
                fragOutputEnabled = false;
        }
        str.insert(insertPos, m_insertStr);
        insertPos += int(m_insertStr.size());
    }

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
                                                              const ShaderFeatureSetList &inFeatures, QSSGRhiShaderPipeline::StageFlags stageFlags)
{
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
    bool valid = true;

    QShaderBaker baker;
    m_initBaker(&baker, m_rhiContext->rhi()->backend());

   static const bool shaderDebug = qEnvironmentVariableIntValue("QT_RHI_SHADER_DEBUG");

    if (shaderDebug) {
        qDebug("VERTEX SHADER:\n*****\n");
        QList<QByteArray> lines = m_vertexCode.split('\n');
        for (int i = 0; i < lines.size(); i++)
            qDebug("%3d  %s", i + 1, lines.at(i).constData());
        qDebug("\n*****\n");
    }
    baker.setSourceString(m_vertexCode, QShader::VertexStage);
    QShader vertexShader = baker.bake();
    if (!vertexShader.isValid()) {
        const QString err = baker.errorMessage();
        qWarning("Failed to compile vertex shader (%s): %s", inKey.constData(), qPrintable(err));
        valid = false;
        if (shaderDebug) {
            QFile f(QLatin1String("failedvert.txt"));
            f.open(QIODevice::WriteOnly | QIODevice::Text);
            f.write(m_vertexCode);
            f.close();
        }
    }

    if (shaderDebug) {
        qDebug("FRAGMENT SHADER:\n*****\n");
        QList<QByteArray> lines = m_fragmentCode.split('\n');
        for (int i = 0; i < lines.size(); i++)
            qDebug("%3d  %s", i + 1, lines.at(i).constData());
        qDebug("\n*****\n");
    }
    baker.setSourceString(m_fragmentCode, QShader::FragmentStage);
    QShader fragmentShader = baker.bake();
    if (!fragmentShader.isValid()) {
        const QString err = baker.errorMessage();
        qWarning("Failed to compile fragment shader (%s): %s", inKey.constData(), qPrintable(err));
        valid = false;
        if (shaderDebug) {
            QFile f(QLatin1String("failedfrag.txt"));
            f.open(QIODevice::WriteOnly | QIODevice::Text);
            f.write(m_fragmentCode);
            f.close();
        }
    }

    if (valid) {
        shaders = new QSSGRhiShaderPipeline(*m_rhiContext.data());
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader), stageFlags);
        shaders->addStage(QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader), stageFlags);
        if (shaderDebug)
            qDebug("Compilation for vertex and fragment stages succeeded");
    }

    const auto inserted = m_rhiShaders.insert(tempKey, shaders);
    return inserted.value();
}

QSSGRef<QSSGRhiShaderPipeline> QSSGShaderCache::loadGeneratedShader(const QByteArray &inKey, QQsbCollection::Entry entry)
{
    const QSSGRef<QSSGRhiShaderPipeline> &rhiShaders = getRhiShaderPipeline(inKey, ShaderFeatureSetList());
    if (rhiShaders)
        return rhiShaders;

    static const bool shaderDebug = qEnvironmentVariableIntValue("QT_RHI_SHADER_DEBUG");
    if (shaderDebug)
        qDebug("Loading pre-compiled rhi shader(s)");

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

    QSSGShaderCacheKey cacheKey(inKey);
    cacheKey.m_features = ShaderFeatureSetList();
    cacheKey.updateHashCode();

    const auto inserted = m_rhiShaders.insert(cacheKey, shaders);
    qsbc.unmap();
    return inserted.value();
}

QSSGRef<QSSGRhiShaderPipeline> QSSGShaderCache::loadBuiltinForRhi(const QByteArray &inKey)
{
    const QSSGRef<QSSGRhiShaderPipeline> &rhiShaders = getRhiShaderPipeline(inKey, ShaderFeatureSetList());
    if (rhiShaders)
        return rhiShaders;

    static const bool shaderDebug = qEnvironmentVariableIntValue("QT_RHI_SHADER_DEBUG");
    if (shaderDebug)
        qDebug("Loading builtin rhi shader: %s", inKey.constData());

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

    QSSGShaderCacheKey cacheKey(inKey);
    cacheKey.m_features = ShaderFeatureSetList();
    cacheKey.updateHashCode();

    const auto inserted = m_rhiShaders.insert(cacheKey, shaders);
    return inserted.value();
}

QT_END_NAMESPACE
