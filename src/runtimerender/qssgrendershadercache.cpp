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

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

namespace {
// using QSSGRenderContextScopedProperty;
// const char *TessellationEnabledStr = "TessellationStageEnabled";
// const char *GeometryEnabledStr = "GeometryStageEnabled";
// inline void AppendFlagValue(QString &inStr, const char *flag)
//{
//    if (inStr.length())
//        inStr.append(QStringLiteral(","));
//    inStr.append(QString::fromLocal8Bit(flag));
//}
// inline void CacheFlagsToStr(const QSSGShaderCacheProgramFlags &inFlags, QString &inString)
//{
//    inString.clear();
//    if (inFlags.isTessellationEnabled())
//        AppendFlagValue(inString, TessellationEnabledStr);
//    if (inFlags.isGeometryShaderEnabled())
//        AppendFlagValue(inString, GeometryEnabledStr);
//}

// inline ShaderType StringToShaderType(QString &inShaderType)
//{
//    ShaderType retval = ShaderType::Vertex;

//    if (inShaderType.size() == 0)
//        return retval;

//    if (!inShaderType.compare("VertexCode"))
//        retval = ShaderType::Vertex;
//    else if (!inShaderType.compare("FragmentCode"))
//        retval = ShaderType::Fragment;
//    else if (!inShaderType.compare("TessControlCode"))
//        retval = ShaderType::TessControl;
//    else if (!inShaderType.compare("TessEvalCode"))
//        retval = ShaderType::TessEval;
//    else if (!inShaderType.compare("GeometryCode"))
//        retval = ShaderType::Geometry;
//    else
//        Q_ASSERT(false);

//    return retval;
//}

// inline QSSGShaderCacheProgramFlags CacheFlagsToStr(const QString &inString)
//{
//    QSSGShaderCacheProgramFlags retval;
//    if (inString.contains(QString::fromLocal8Bit(TessellationEnabledStr)))
//        retval.setTessellationEnabled(true);
//    if (inString.contains(QString::fromLocal8Bit(GeometryEnabledStr)))
//        retval.setGeometryShaderEnabled(true);
//    return retval;
//}

// typedef QPair<const char *, QSSGRenderContextValues> TStringToContextValuePair;

/*GLES2	= 1 << 0,
GL2		= 1 << 1,
GLES3	= 1 << 2,
GL3		= 1 << 3,
GL4		= 1 << 4,
NullContext = 1 << 5,*/
// TStringToContextValuePair g_StringToContextTypeValue[] = {
//    TStringToContextValuePair("GLES2", QSSGRenderContextValues::GLES2),
//    TStringToContextValuePair("GL2", QSSGRenderContextValues::GL2),
//    TStringToContextValuePair("GLES3", QSSGRenderContextValues::GLES3),
//    TStringToContextValuePair("GLES3PLUS", QSSGRenderContextValues::GLES3PLUS),
//    TStringToContextValuePair("GL3", QSSGRenderContextValues::GL3),
//    TStringToContextValuePair("GL4", QSSGRenderContextValues::GL4),
//    TStringToContextValuePair("NullContext", QSSGRenderContextValues::NullContext),
//};

// size_t g_NumStringToContextValueEntries =
//        sizeof(g_StringToContextTypeValue) / sizeof(*g_StringToContextTypeValue);

// inline void ContextTypeToString(QSSGRenderContextType inType,
//                                QString &outContextType)
//{
//    outContextType.clear();
//    for (size_t idx = 0, end = g_NumStringToContextValueEntries; idx < end; ++idx) {
//        if (inType & g_StringToContextTypeValue[idx].second) {
//            if (outContextType.size())
//                outContextType.append("|");
//            outContextType.append(QString::fromLocal8Bit(g_StringToContextTypeValue[idx].first));
//        }
//    }
//}

// inline QSSGRenderContextType StringToContextType(const QString &inContextType)
//{
//    QSSGRenderContextType retval;
//    char tempBuffer[128];
//    memZero(tempBuffer, 128);
//    const QString::size_type lastTempBufIdx = 127;
//    QString::size_type pos = 0, lastpos = 0;
//    if (inContextType.size() == 0)
//        return retval;

//    do {
//        pos = int(inContextType.indexOf('|', lastpos));
//        if (pos == -1)
//            pos = int(inContextType.size());
//        {
//            size_t sectionLen = size_t(qMin(pos - lastpos, lastTempBufIdx));
//            ::memcpy(tempBuffer, inContextType.data() + lastpos, sectionLen);
//            tempBuffer[lastTempBufIdx] = 0;
//            for (size_t idx = 0, end = g_NumStringToContextValueEntries; idx < end; ++idx) {
//                if (strcmp(g_StringToContextTypeValue[idx].first, tempBuffer) == 0)
//                    retval = retval | g_StringToContextTypeValue[idx].second;
//            }
//        }
//        // iterate past the bar
//        ++pos;
//        lastpos = pos;
//    } while (pos < inContextType.size() && pos != -1);

//    return retval;
//}
}

static QByteArray defaultShaderPrecision(const QByteArray &defPrecision)
{
    static const QByteArray precision = qEnvironmentVariable("QT_QUICK3D_SHADER_PRECISION").toLatin1();
    if (precision.isEmpty() || (precision != QByteArrayLiteral("mediump")
                                    && precision != QByteArrayLiteral("lowp")
                                    && precision != QByteArrayLiteral("highp"))) {
        return defPrecision;
    }
    return precision;
}

static QByteArray defaultSamplerPrecision(const QByteArray &defPrecision)
{
    static const QByteArray samplerPrecision = qEnvironmentVariable("QT_QUICK3D_SAMPLER_PRECISION").toLatin1();
    if (samplerPrecision.isEmpty() || (samplerPrecision != QByteArrayLiteral("mediump")
                                            && samplerPrecision != QByteArrayLiteral("lowp")
                                            && samplerPrecision != QByteArrayLiteral("highp"))) {
        return defPrecision;
    }
    return samplerPrecision;
}

static const char *defineTable[QSSGShaderDefines::Count] {
    "QSSG_ENABLE_LIGHT_PROBE",
    "QSSG_ENABLE_LIGHT_PROBE_2",
    "QSSG_ENABLE_IBL_FOV",
    "QSSG_ENABLE_SSM",
    "QSSG_ENABLE_SSAO",
    "QSSG_ENABLE_SSDO",
    "QSSG_ENABLE_CG_LIGHTING"
};

const char *QSSGShaderDefines::asString(QSSGShaderDefines::Define def) { return defineTable[def]; }

uint qHash(const QSSGShaderCacheKey &key)
{
    return key.m_hashCode;
}

uint hashShaderFeatureSet(const ShaderFeatureSetList &inFeatureSet)
{
    uint retval(0);
    for (int idx = 0, end = inFeatureSet.size(); idx < end; ++idx) {
        // From previous implementation, it seems we need to ignore the order of the features.
        // But we need to bind the feature flag together with its name, so that the flags will
        // influence
        // the final hash not only by the true-value count.
        retval ^= (inFeatureSet.at(idx).key ^ uint(inFeatureSet.at(idx).enabled));
    }
    return retval;
}

QSSGShaderCache::~QSSGShaderCache() {}

QSSGRef<QSSGShaderCache> QSSGShaderCache::createShaderCache(const QSSGRef<QSSGRenderContext> &inContext,
                                                                  const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory,
                                                                  QSSGPerfTimer *inPerfTimer)
{
    return QSSGRef<QSSGShaderCache>(new QSSGShaderCache(inContext, inInputStreamFactory, inPerfTimer));
}

QSSGShaderCache::QSSGShaderCache(const QSSGRef<QSSGRenderContext> &ctx, const QSSGRef<QSSGInputStreamFactory> &inInputStreamFactory, QSSGPerfTimer *)
    : m_renderContext(ctx), /*m_perfTimer(inPerfTimer),*/ m_inputStreamFactory(inInputStreamFactory), m_shaderCompilationEnabled(true)
{
}

QSSGRef<QSSGRenderShaderProgram> QSSGShaderCache::getProgram(const QByteArray &inKey, const ShaderFeatureSetList &inFeatures)
{
    m_tempKey.m_key = inKey;
    m_tempKey.m_features = inFeatures;
    m_tempKey.generateHashCode();
    const auto theIter = m_shaders.constFind(m_tempKey);
    if (theIter != m_shaders.cend())
        return theIter.value();
    return nullptr;
}

void QSSGShaderCache::addBackwardCompatibilityDefines(ShaderType shaderType)
{
    if (shaderType == ShaderType::Vertex || shaderType == ShaderType::TessControl
            || shaderType == ShaderType::TessEval || shaderType == ShaderType::Geometry) {
        m_insertStr += "#define attribute in\n";
        m_insertStr += "#define varying out\n";
    } else if (shaderType == ShaderType::Fragment) {
        m_insertStr += "#define varying in\n";
        m_insertStr += "#define texture2D texture\n";
        m_insertStr += "#define gl_FragColor fragOutput\n";

        if (m_renderContext->supportsAdvancedBlendHwKHR())
            m_insertStr += "layout(blend_support_all_equations) out;\n ";

        m_insertStr += "#ifndef NO_FRAG_OUTPUT\n";
        m_insertStr += "out vec4 fragOutput;\n";
        m_insertStr += "#endif\n";
    }
}

void QSSGShaderCache::addShaderExtensionStrings(ShaderType shaderType, bool isGLES)
{
    if (isGLES) {
        if (m_renderContext->supportsStandardDerivatives())
            m_insertStr += "#extension GL_OES_standard_derivatives : enable\n";
        else
            m_insertStr += "#extension GL_OES_standard_derivatives : disable\n";
    }

    if (QSSGRendererInterface::isGlEs3Context(m_renderContext->renderContextType())) {
        if (shaderType == ShaderType::TessControl || shaderType == ShaderType::TessEval) {
            m_insertStr += "#extension GL_EXT_tessellation_shader : enable\n";
        } else if (shaderType == ShaderType::Geometry) {
            m_insertStr += "#extension GL_EXT_geometry_shader : enable\n";
        } else if (shaderType == ShaderType::Vertex || shaderType == ShaderType::Fragment) {
            if (m_renderContext->renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::gpuShader5))
                m_insertStr += "#extension GL_EXT_gpu_shader5 : enable\n";
            if (m_renderContext->supportsAdvancedBlendHwKHR())
                m_insertStr += "#extension GL_KHR_blend_equation_advanced : enable\n";
        }
    } else {
        if (shaderType == ShaderType::Vertex || shaderType == ShaderType::Fragment || shaderType == ShaderType::Geometry) {
            if (m_renderContext->renderContextType() != QSSGRenderContextType::GLES2) {
                m_insertStr += "#extension GL_ARB_gpu_shader5 : enable\n";
//                m_insertStr += "#extension GL_ARB_shading_language_420pack : enable\n";
            }
            if (isGLES && m_renderContext->supportsTextureLod())
                m_insertStr += "#extension GL_EXT_shader_texture_lod : enable\n";
            if (m_renderContext->supportsShaderImageLoadStore())
                m_insertStr += "#extension GL_ARB_shader_image_load_store : enable\n";
            if (m_renderContext->supportsStorageBuffer())
                m_insertStr += "#extension GL_ARB_shader_storage_buffer_object : enable\n";
            if (m_renderContext->supportsAdvancedBlendHwKHR())
                m_insertStr += "#extension GL_KHR_blend_equation_advanced : enable\n";
        }
    }
}

void QSSGShaderCache::addShaderPreprocessor(QByteArray &str, const QByteArray &inKey, ShaderType shaderType, const ShaderFeatureSetList &inFeatures)
{
    // Don't use shading language version returned by the driver as it might
    // differ from the context version. Instead use the context type to specify
    // the version string.
    const auto contextType = m_renderContext->renderContextType();
    const bool isGlES = QSSGRendererInterface::isGlEsContext(contextType);
    m_insertStr.clear();

    m_insertStr.append(m_renderContext->shadingLanguageVersion());

    if (inFeatures.size()) {
        for (int idx = 0, end = inFeatures.size(); idx < end; ++idx) {
            QSSGShaderPreprocessorFeature feature(inFeatures[idx]);
            m_insertStr.append("#define ");
            m_insertStr.append(inFeatures[idx].name);
            m_insertStr.append(" ");
            m_insertStr.append(feature.enabled ? "1" : "0");
            m_insertStr.append("\n");
        }
    }

    if (isGlES) {
        if (!QSSGRendererInterface::isGlEs3Context(contextType)) {
            if (shaderType == ShaderType::Fragment) {
                m_insertStr += "#define fragOutput gl_FragData[0]\n";
            }
        } else {
            m_insertStr += "#define texture2D texture\n";
        }

        // add extenions strings before any other non-processor token
        addShaderExtensionStrings(shaderType, isGlES);

        // add precision qualifier depending on backend
        if (QSSGRendererInterface::isGlEs3Context(contextType)) {
            const QByteArray precision = defaultShaderPrecision(QByteArrayLiteral("highp"));
            const QByteArray samplerPrecision = defaultSamplerPrecision(QByteArrayLiteral("mediump"));

            QByteArray precisionQualifiers = "precision " + precision + " float;\n";
            precisionQualifiers += "precision " + precision + " int;\n";
            m_insertStr.append(precisionQualifiers);

            if (m_renderContext->renderBackendCap(QSSGRenderBackend::QSSGRenderBackendCaps::gpuShader5)) {
                precisionQualifiers = "precision " + samplerPrecision + " sampler2D;\n";
                precisionQualifiers += "precision " + samplerPrecision + " sampler2DArray;\n";
                precisionQualifiers += "precision " + samplerPrecision + " sampler2DShadow;\n";
                m_insertStr.append(precisionQualifiers);

                if (m_renderContext->supportsShaderImageLoadStore()) {
                    precisionQualifiers = "precision " + samplerPrecision + " image2D;\n";
                    m_insertStr.append(precisionQualifiers);
                }
            }

            addBackwardCompatibilityDefines(shaderType);
        } else {
            // GLES2
            const QByteArray precision = defaultShaderPrecision(QByteArrayLiteral("mediump"));

            QByteArray precisionQualifiers = "precision " + precision + " float;\n";
            precisionQualifiers += "precision " + precision + " int;\n";
            m_insertStr.append(precisionQualifiers);

            m_insertStr.append("#define texture texture2D\n");
            if (m_renderContext->supportsTextureLod())
                m_insertStr.append("#define textureLod texture2DLodEXT\n");
            else
                m_insertStr.append("#define textureLod(s, co, lod) texture2D(s, co)\n");
        }
    } else {
        if (!QSSGRendererInterface::isGl2Context(contextType)) {
            m_insertStr += "#define texture2D texture\n";

            addShaderExtensionStrings(shaderType, isGlES);

            m_insertStr += "#if __VERSION__ >= 330\n";

            addBackwardCompatibilityDefines(shaderType);

            m_insertStr += "#else\n";
            if (shaderType == ShaderType::Fragment) {
                m_insertStr += "#define fragOutput gl_FragData[0]\n";
            }
            m_insertStr += "#endif\n";
        }
    }

    if (!inKey.isNull()) {
        m_insertStr += "//Shader name -";
        m_insertStr += inKey;
        m_insertStr += "\n";
    }

    if (shaderType == ShaderType::TessControl) {
        m_insertStr += "#define TESSELLATION_CONTROL_SHADER 1\n";
        m_insertStr += "#define TESSELLATION_EVALUATION_SHADER 0\n";
    } else if (shaderType == ShaderType::TessEval) {
        m_insertStr += "#define TESSELLATION_CONTROL_SHADER 0\n";
        m_insertStr += "#define TESSELLATION_EVALUATION_SHADER 1\n";
    }

    str.insert(0, m_insertStr);
}

QSSGRef<QSSGRenderShaderProgram> QSSGShaderCache::forceCompileProgram(const QByteArray &inKey, const QByteArray &inVert, const QByteArray &inFrag, const QByteArray &inTessCtrl, const QByteArray &inTessEval, const QByteArray &inGeom, const QSSGShaderCacheProgramFlags &inFlags, const ShaderFeatureSetList &inFeatures, bool separableProgram, bool fromDisk)
{
    if (!m_shaderCompilationEnabled)
        return nullptr;
    QSSGShaderCacheKey tempKey(inKey);
    tempKey.m_features = inFeatures;
    tempKey.generateHashCode();

    if (fromDisk) {
        qCInfo(TRACE_INFO) << "Loading from persistent shader cache: '<" << tempKey.m_key << ">'";
    } else {
        qCInfo(TRACE_INFO) << "Compiling into shader cache: '" << tempKey.m_key << ">'";
    }

    // SStackPerfTimer __perfTimer(m_PerfTimer, "Shader Compilation");
    m_vertexCode = inVert;
    m_tessCtrlCode = inTessCtrl;
    m_tessEvalCode = inTessEval;
    m_geometryCode = inGeom;
    m_fragmentCode = inFrag;
    // Add defines and such so we can write unified shaders that work across platforms.
    // vertex and fragment shaders are optional for separable shaders
    if (!separableProgram || !m_vertexCode.isEmpty())
        addShaderPreprocessor(m_vertexCode, inKey, ShaderType::Vertex, inFeatures);
    if (!separableProgram || !m_fragmentCode.isEmpty())
        addShaderPreprocessor(m_fragmentCode, inKey, ShaderType::Fragment, inFeatures);
    // optional shaders
    if (inFlags & ShaderCacheProgramFlagValues::TessellationEnabled) {
        Q_ASSERT(m_tessCtrlCode.size() && m_tessEvalCode.size());
        addShaderPreprocessor(m_tessCtrlCode, inKey, ShaderType::TessControl, inFeatures);
        addShaderPreprocessor(m_tessEvalCode, inKey, ShaderType::TessEval, inFeatures);
    }
    if (inFlags & ShaderCacheProgramFlagValues::GeometryShaderEnabled)
        addShaderPreprocessor(m_geometryCode, inKey, ShaderType::Geometry, inFeatures);

    auto shaderProgram = m_renderContext->compileSource(inKey.constData(),
                                                        toByteView(m_vertexCode),
                                                        toByteView(m_fragmentCode),
                                                        toByteView(m_tessCtrlCode),
                                                        toByteView(m_tessEvalCode),
                                                        toByteView(m_geometryCode),
                                                        separableProgram).m_shader;
    const auto inserted = m_shaders.insert(tempKey, shaderProgram);
    if (shaderProgram) {
        // This is unnecessary memory waste in final deployed product, so we don't store this
        // information when shaders were initialized from a cache.
        // Unfortunately it is not practical to just regenerate shader source from scratch, when we
        // want to export it, as the triggers and original sources are spread all over the place.
        if (!m_shadersInitializedFromCache && inserted != m_shaders.end()) {
            // Store sources for possible cache generation later
            QSSGShaderSource ss;
            for (int i = 0, end = inFeatures.size(); i < end; ++i)
                ss.features.append(inFeatures[i]);
            ss.key = inKey;
            ss.flags = inFlags;
            ss.vertexCode = inVert;
            ss.fragmentCode = inFrag;
            ss.tessCtrlCode = inTessCtrl;
            ss.tessEvalCode = inTessEval;
            ss.geometryCode = inGeom;
            m_shaderSourceCache.append(ss);
        }
        // ### Shader Chache Writing Code is disabled
        //            if (m_ShaderCache) {
        //                IDOMWriter::Scope __writeScope(*m_ShaderCache, "Program");
        //                m_ShaderCache->Att("key", inKey.toLocal8Bit().constData());
        //                CacheFlagsToStr(inFlags, m_FlagString);
        //                if (m_FlagString.size())
        //                    m_ShaderCache->Att("glflags", m_FlagString.toLocal8Bit().constData());
        //                // write out the GL version.
        //                {
        //                    QSSGRenderContextType theContextType =
        //                            m_RenderContext.GetRenderContextType();
        //                    ContextTypeToString(theContextType, m_ContextTypeString);
        //                    m_ShaderCache->Att("gl-context-type", m_ContextTypeString.toLocal8Bit().constData());
        //                }
        //                if (inFeatures.size()) {
        //                    IDOMWriter::Scope __writeScope(*m_ShaderCache, "Features");
        //                    for (int idx = 0, end = inFeatures.size(); idx < end; ++idx) {
        //                        m_ShaderCache->Att(inFeatures[idx].m_Name, inFeatures[idx].m_Enabled);
        //                    }
        //                }

        //                {
        //                    IDOMWriter::Scope __writeScope(*m_ShaderCache, "VertexCode");
        //                    m_ShaderCache->Value(inVert);
        //                }
        //                {
        //                    IDOMWriter::Scope __writeScope(*m_ShaderCache, "FragmentCode");
        //                    m_ShaderCache->Value(inFrag);
        //                }
        //                if (m_TessCtrlCode.size()) {
        //                    IDOMWriter::Scope __writeScope(*m_ShaderCache, "TessControlCode");
        //                    m_ShaderCache->Value(inTessCtrl);
        //                }
        //                if (m_TessEvalCode.size()) {
        //                    IDOMWriter::Scope __writeScope(*m_ShaderCache, "TessEvalCode");
        //                    m_ShaderCache->Value(inTessEval);
        //                }
        //                if (m_GeometryCode.size()) {
        //                    IDOMWriter::Scope __writeScope(*m_ShaderCache, "GeometryCode");
        //                    m_ShaderCache->Value(inGeom);
        //                }
        //            }
    }
    return inserted.value();
}

QSSGRef<QSSGRenderShaderProgram> QSSGShaderCache::compileProgram(const QByteArray &inKey, const QByteArray &inVert, const QByteArray &inFrag, const QByteArray &inTessCtrl, const QByteArray &inTessEval, const QByteArray &inGeom, const QSSGShaderCacheProgramFlags &inFlags, const ShaderFeatureSetList &inFeatures, bool separableProgram)
{
    const QSSGRef<QSSGRenderShaderProgram> &theProgram = getProgram(inKey, inFeatures);
    if (theProgram)
        return theProgram;

    const QSSGRef<QSSGRenderShaderProgram> &retval = forceCompileProgram(inKey, inVert, inFrag, inTessCtrl, inTessEval, inGeom, inFlags, inFeatures, separableProgram);
    // ### Shader Chache Writing Code is disabled
    //        if (m_CacheFilePath.toLocal8Bit().constData() && m_ShaderCache && m_ShaderCompilationEnabled) {
    //            CFileSeekableIOStream theStream(m_CacheFilePath.toLocal8Bit().constData(), FileWriteFlags());
    //            if (theStream.IsOpen()) {
    //                CDOMSerializer::WriteXMLHeader(theStream);
    //                CDOMSerializer::Write(*m_ShaderCache->GetTopElement(), theStream);
    //            }
    //        }
    return retval;
}

void QSSGShaderCache::setShaderCachePersistenceEnabled(const QString &inDirectory)
{
    // ### Shader Chache Writing Code is disabled
    Q_UNUSED(inDirectory)

    //        if (inDirectory == nullptr) {
    //            m_ShaderCache = nullptr;
    //            return;
    //        }
    //        BootupDOMWriter();
    //        m_CacheFilePath = QDir(inDirectory).filePath(GetShaderCacheFileName()).toStdString();

    //        QSSGRef<IRefCountedInputStream> theInStream =
    //                m_InputStreamFactory.GetStreamForFile(m_CacheFilePath.c_str());
    //        if (theInStream) {
    //            SStackPerfTimer __perfTimer(m_PerfTimer, "ShaderCache - Load");
    //            QSSGRef<IDOMFactory> theFactory(
    //                        IDOMFactory::CreateDOMFactory(m_RenderContext.GetAllocator(), theStringTable));
    //            QVector<SShaderPreprocessorFeature> theFeatures;

    //            SDOMElement *theElem = CDOMSerializer::Read(*theFactory, *theInStream).second;
    //            if (theElem) {
    //                QSSGRef<IDOMReader> theReader = IDOMReader::CreateDOMReader(
    //                            m_RenderContext.GetAllocator(), *theElem, theStringTable, theFactory);
    //                quint32 theAttValue = 0;
    //                theReader->Att("cache_version", theAttValue);
    //                if (theAttValue == IShaderCache::GetShaderVersion()) {
    //                    QString loadVertexData;
    //                    QString loadFragmentData;
    //                    QString loadTessControlData;
    //                    QString loadTessEvalData;
    //                    QString loadGeometryData;
    //                    QString shaderTypeString;
    //                    for (bool success = theReader->MoveToFirstChild(); success;
    //                         success = theReader->MoveToNextSibling()) {
    //                        const char *theKeyStr = nullptr;
    //                        theReader->UnregisteredAtt("key", theKeyStr);

    //                        QString theKey = QString::fromLocal8Bit(theKeyStr);
    //                        if (theKey.IsValid()) {
    //                            m_FlagString.clear();
    //                            const char *theFlagStr = "";
    //                            SShaderCacheProgramFlags theFlags;
    //                            if (theReader->UnregisteredAtt("glflags", theFlagStr)) {
    //                                m_FlagString.assign(theFlagStr);
    //                                theFlags = CacheFlagsToStr(m_FlagString);
    //                            }

    //                            m_ContextTypeString.clear();
    //                            if (theReader->UnregisteredAtt("gl-context-type", theFlagStr))
    //                                m_ContextTypeString.assign(theFlagStr);

    //                            theFeatures.clear();
    //                            {
    //                                IDOMReader::Scope __featureScope(*theReader);
    //                                if (theReader->MoveToFirstChild("Features")) {
    //                                    for (SDOMAttribute *theAttribute =
    //                                         theReader->GetFirstAttribute();
    //                                         theAttribute;
    //                                         theAttribute = theAttribute->m_NextAttribute) {
    //                                        bool featureValue = false;
    //                                        StringConversion<bool>().StrTo(theAttribute->m_Value,
    //                                                                       featureValue);
    //                                        theFeatures.push_back(SShaderPreprocessorFeature(
    //                                                                  QString::fromLocal8Bit(
    //                                                                      theAttribute->m_Name.c_str()),
    //                                                                  featureValue));
    //                                    }
    //                                }
    //                            }

    //                            QSSGRenderContextType theContextType =
    //                                    StringToContextType(m_ContextTypeString);
    //                            if (((quint32)theContextType != 0)
    //                                    && (theContextType & m_RenderContext.GetRenderContextType())
    //                                    == theContextType) {
    //                                IDOMReader::Scope __readerScope(*theReader);
    //                                loadVertexData.clear();
    //                                loadFragmentData.clear();
    //                                loadTessControlData.clear();
    //                                loadTessEvalData.clear();
    //                                loadGeometryData.clear();

    //                                // Vertex *MUST* be the first
    //                                // Todo deal with pure compute shader programs
    //                                if (theReader->MoveToFirstChild("VertexCode")) {
    //                                    const char *theValue = nullptr;
    //                                    theReader->Value(theValue);
    //                                    loadVertexData.assign(theValue);
    //                                    while (theReader->MoveToNextSibling()) {
    //                                        theReader->Value(theValue);

    //                                        shaderTypeString.assign(
    //                                                    theReader->GetElementName().c_str());
    //                                        ShaderType shaderType =
    //                                                StringToShaderType(shaderTypeString);

    //                                        if (shaderType == ShaderType::Fragment)
    //                                            loadFragmentData.assign(theValue);
    //                                        else if (shaderType == ShaderType::TessControl)
    //                                            loadTessControlData.assign(theValue);
    //                                        else if (shaderType == ShaderType::TessEval)
    //                                            loadTessEvalData.assign(theValue);
    //                                        else if (shaderType == ShaderType::Geometry)
    //                                            loadGeometryData.assign(theValue);
    //                                    }
    //                                }

    //                                if (loadVertexData.size()
    //                                        && (loadFragmentData.size() || loadGeometryData.size())) {

    //                                    QSSGRef<QSSGRenderShaderProgram> theShader = ForceCompileProgram(
    //                                                theKey, loadVertexData.toLocal8Bit().constData(),
    //                                                loadFragmentData.toLocal8Bit().constData(), loadTessControlData.toLocal8Bit().constData(),
    //                                                loadTessEvalData.toLocal8Bit().constData(), loadGeometryData.toLocal8Bit().constData(),
    //                                                theFlags, theFeatures, false, true /*fromDisk*/);
    //                                    // If something doesn't save or load correctly, get the runtime
    //                                    // to re-generate.
    //                                    if (!theShader)
    //                                        m_Shaders.remove(theKey);
    //                                }
    //                            }
    //                        }
    //                    }
    //                }
    //            }
    //        }
}

bool QSSGShaderCache::isShaderCachePersistenceEnabled() const
{
    // ### Shader Chache Writing Code is disabled
    // return m_ShaderCache != nullptr;
    return false;
}

void QSSGShaderCache::setShaderCompilationEnabled(bool inEnableShaderCompilation)
{
    m_shaderCompilationEnabled = inEnableShaderCompilation;
}

quint32 QSSGShaderCache::shaderCacheVersion() const
{
    return 1;
}

quint32 QSSGShaderCache::shaderCacheFileId() const
{
    return 0x26a9b358;
}

void QSSGShaderCache::importShaderCache(const QByteArray &shaderCache, QByteArray &errors)
{
    #define BAILOUT(details) { \
        QByteArray errorMsg = QByteArrayLiteral("importShaderCache failed to import shader cache: " details); \
        qWarning() << errorMsg; \
        errors.append(errorMsg); \
        return; \
    }

    if (shaderCache.isEmpty())
        BAILOUT("Shader cache Empty")

    QDataStream data(shaderCache);
    quint32 type;
    quint32 version;
    bool isBinary;
    data >> type;

    auto binaryShadersSupported = [this]() -> bool {
        return !(m_renderContext->format().renderableType() == QSurfaceFormat::OpenGLES
                                && m_renderContext->format().majorVersion() == 2);
    };

    if (type != shaderCacheFileId())
        BAILOUT("Not a shader cache")
    data >> isBinary;
    if (isBinary && !binaryShadersSupported())
        BAILOUT("Binary shaders are not supported")
    data >> version;
    if (version != shaderCacheVersion())
        BAILOUT("Version mismatch")

    #undef BAILOUT

    int progCount;
    data >> progCount;
    m_shadersInitializedFromCache = progCount > 0;
    for (int i = 0; i < progCount; ++i) {
        QByteArray key;
        int featCount;

        data >> key;
        data >> featCount;

        ShaderFeatureSetList features;
        for (int j = 0; j < featCount; ++j) {
            QByteArray featName;
            bool featVal;
            data >> featName;
            data >> featVal;
            features.push_back(QSSGShaderPreprocessorFeature(featName, featVal));
        }
        QSSGRef<QSSGRenderShaderProgram> theShader;
        QSSGShaderCacheKey tempKey(key);
        tempKey.m_features = features;
        tempKey.generateHashCode();
        if (isBinary) {
            quint32 format;
            QByteArray binary;
            data >> format;
            data >> binary;

            qCInfo(TRACE_INFO) << "Loading binary program from shader cache: '<" << key << ">'";

            QSSGRenderVertFragCompilationResult result = m_renderContext->compileBinary(key, format, binary);
            theShader = result.m_shader;
            if (theShader.isNull())
                errors += theShader->errorMessage();
            else
                m_shaders.insert(tempKey, theShader);
        } else {
            QByteArray loadVertexData;
            QByteArray loadFragmentData;
            QByteArray loadTessControlData;
            QByteArray loadTessEvalData;
            QByteArray loadGeometryData;

            data >> loadVertexData;
            data >> loadFragmentData;
            data >> loadTessControlData;
            data >> loadTessEvalData;
            data >> loadGeometryData;

            if (!loadVertexData.isEmpty() && (!loadFragmentData.isEmpty()
                                              || !loadGeometryData.isEmpty())) {
                QByteArray error;
                QSSGRenderVertFragCompilationResult result
                        = m_renderContext->compileSource(key, QSSGByteView(loadVertexData), QSSGByteView(loadFragmentData),
                                                         QSSGByteView(loadTessControlData), QSSGByteView(loadTessControlData),
                                                         QSSGByteView(loadGeometryData));
                theShader = result.m_shader;
                if (theShader.isNull())
                    errors += theShader->errorMessage();
                else
                    m_shaders.insert(tempKey, theShader);
            }
        }
        // If something doesn't save or load correctly, get the runtime to re-generate.
        if (theShader.isNull()) {
            qWarning() << __FUNCTION__ << "Failed to load a cached a shader:" << key;
            m_shadersInitializedFromCache = false;
        }
    }
}

QByteArray QSSGShaderCache::exportShaderCache(bool binaryShaders)
{
    if (m_shadersInitializedFromCache) {
        qWarning() << __FUNCTION__ << "Warning: Shader cache export is not supported when"
                                      " shaders were originally imported from a cache file.";
        return {};
    }

    auto binaryShadersSupported = [this]() -> bool {
        return !(m_renderContext->format().renderableType() == QSurfaceFormat::OpenGLES
                                && m_renderContext->format().majorVersion() == 2);
    };

    // The assumption is that cache was generated on the same environment it will be read.
    // Attempting to load a cache generated on another environment will likely lead to crash.

    QByteArray retval;
    QDataStream data(&retval, QIODevice::WriteOnly);
    bool saveBinary = binaryShaders && binaryShadersSupported();
    data << shaderCacheFileId();
    data << saveBinary;
    data << shaderCacheVersion();
    data << m_shaderSourceCache.size();

    for (const auto &ss : qAsConst(m_shaderSourceCache))
    {
        data << ss.key;
        data << ss.features.size();
        for (int i = 0, end = ss.features.size(); i < end; ++i) {
            data << ss.features[i].name;
            data << ss.features[i].enabled;
        }

        if (saveBinary) {
            auto program = getProgram(ss.key, ss.features);
            quint32 format = 0;
            QByteArray binaryData;
            program->getProgramBinary(format, binaryData);
            data << format;
            data << binaryData;
        } else {
            m_vertexCode = ss.vertexCode;
            m_tessCtrlCode = ss.tessCtrlCode;
            m_tessEvalCode = ss.tessEvalCode;
            m_geometryCode = ss.geometryCode;
            m_fragmentCode = ss.fragmentCode;
            // Add defines and such so we can write unified shaders that work across platforms.
            // vertex and fragment shaders are optional for separable shaders
            if (m_vertexCode.size())
                addShaderPreprocessor(m_vertexCode, ss.key, ShaderType::Vertex, ss.features);
            if (m_fragmentCode.size())
                addShaderPreprocessor(m_fragmentCode, ss.key, ShaderType::Fragment, ss.features);
            // optional shaders
            if (m_tessCtrlCode.size() && m_tessEvalCode.size()) {
                addShaderPreprocessor(m_tessCtrlCode, ss.key, ShaderType::TessControl, ss.features);
                addShaderPreprocessor(m_tessEvalCode, ss.key, ShaderType::TessEval, ss.features);
            }
            if (m_geometryCode.size())
                addShaderPreprocessor(m_geometryCode, ss.key, ShaderType::Geometry, ss.features);

            auto writeShaderElement = [&data](const QByteArray &shaderSource) {
                QByteArray stripped = shaderSource;
                int start = stripped.indexOf(QByteArrayLiteral("/*"));
                while (start != -1) {
                    int end = stripped.indexOf(QByteArrayLiteral("*/"));
                    if (end == -1)
                        break; // Mismatched comment
                    stripped.replace(start, end - start + 2, QByteArray());
                    start = stripped.indexOf(QByteArrayLiteral("/*"));
                }
                data << stripped;
            };

            writeShaderElement(m_vertexCode);
            writeShaderElement(m_fragmentCode);
            writeShaderElement(m_tessCtrlCode);
            writeShaderElement(m_tessEvalCode);
            writeShaderElement(m_geometryCode);
        }
    }
    return retval;
}

QT_END_NAMESPACE
