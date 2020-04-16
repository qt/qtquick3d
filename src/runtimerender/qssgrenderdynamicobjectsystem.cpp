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

#include "qssgrenderdynamicobjectsystem_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderinputstreamfactory_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystemcommands_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystemutil_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshaderresourcemergecontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadermetadata_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

#include <QtQuick3DRender/private/qssgrendershaderconstant_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

QT_BEGIN_NAMESPACE

size_t qHash(const TStrStrPair &item)
{
    return qHash(item.first) ^ qHash(item.second);
}

namespace dynamic {

size_t qHash(const QSSGDynamicShaderMapKey &inKey)
{
    return inKey.m_hashCode;
}

const char *QSSGCommand::typeAsString() const
{
    switch (m_type) {
    case CommandType::Unknown:
        return "Unknown";
    case CommandType::AllocateBuffer:
        return "AllocateBuffer";
    case CommandType::BindTarget:
        return "BindTarget";
    case CommandType::BindBuffer:
        return "BindBuffer";
    case CommandType::BindShader:
        return "BindShader";
    case CommandType::ApplyInstanceValue:
        return "ApplyInstanceValue";
    case CommandType::ApplyBufferValue:
        return "ApplyBufferValue";
    case CommandType::ApplyDepthValue:
        return "ApplyDepthValue";
    case CommandType::Render:
        return "Render";
    case CommandType::ApplyBlending:
        return "ApplyBlending";
    case CommandType::ApplyRenderState:
        return "ApplyRenderState";
    case CommandType::ApplyBlitFramebuffer:
        return "ApplyBlitFramebuffer";
    case CommandType::ApplyValue:
        return "ApplyValue";
    case CommandType::DepthStencil:
        return "DepthStencil";
    case CommandType::AllocateImage:
        return "AllocateImage";
    case CommandType::ApplyImageValue:
        return "ApplyImageValue";
    case CommandType::AllocateDataBuffer:
        return "AllocateDataBuffer";
    case CommandType::ApplyDataBufferValue:
        return "ApplyDataBufferValue";
    case CommandType::ApplyCullMode:
        return "ApplyCullMode";
    default:
        break;
    }
    return "";
}

QString QSSGCommand::debugString() const
{
    QString result;
    QDebug stream(&result);

    switch (m_type) {
    case CommandType::AllocateBuffer:
        static_cast<const QSSGAllocateBuffer*>(this)->addDebug(stream);
        break;
    case CommandType::BindTarget:
        static_cast<const QSSGBindTarget*>(this)->addDebug(stream);
        break;
    case CommandType::BindBuffer:
        static_cast<const QSSGBindBuffer*>(this)->addDebug(stream);
        break;
    case CommandType::BindShader:
        static_cast<const QSSGBindShader*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyInstanceValue:
        static_cast<const QSSGApplyInstanceValue*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyBufferValue:
        static_cast<const QSSGApplyBufferValue*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyDepthValue:
        static_cast<const QSSGApplyDepthValue*>(this)->addDebug(stream);
        break;
    case CommandType::Render:
        static_cast<const QSSGRender*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyBlending:
        static_cast<const QSSGApplyBlending*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyRenderState:
        static_cast<const QSSGApplyRenderState*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyBlitFramebuffer:
        static_cast<const QSSGApplyBlitFramebuffer*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyValue:
        static_cast<const QSSGApplyValue*>(this)->addDebug(stream);
        break;
    case CommandType::DepthStencil:
        static_cast<const QSSGDepthStencil*>(this)->addDebug(stream);
        break;
    case CommandType::AllocateImage:
        static_cast<const QSSGAllocateImage*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyImageValue:
        static_cast<const QSSGApplyImageValue*>(this)->addDebug(stream);
        break;
    case CommandType::AllocateDataBuffer:
        static_cast<const QSSGAllocateDataBuffer*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyDataBufferValue:
        static_cast<const QSSGApplyDataBufferValue*>(this)->addDebug(stream);
        break;
    case CommandType::ApplyCullMode:
        static_cast<const QSSGApplyCullMode*>(this)->addDebug(stream);
        break;
    case CommandType::Unknown:
    default:
        addDebug(stream);
        break;
    }

    return result;
}

void QSSGCommand::addDebug(QDebug &stream) const
{
    stream << "No debug info for " << typeAsString();
}

}

QString QSSGDynamicObjectSystem::getShaderCodeLibraryDirectory()
{
    return QStringLiteral("res/effectlib");
}
static QByteArray includeSearch() { return QByteArrayLiteral("#include \""); };
static QByteArray copyrightHeaderStart() { return QByteArrayLiteral("/****************************************************************************"); }
static QByteArray copyrightHeaderEnd() { return QByteArrayLiteral("****************************************************************************/"); }


QSSGDynamicObjectSystem::QSSGDynamicObjectSystem(QSSGRenderContextInterface *ctx)
    : m_context(ctx), m_propertyLoadMutex()
{
}

QSSGDynamicObjectSystem::~QSSGDynamicObjectSystem() {}

void QSSGDynamicObjectSystem::setShaderData(const QByteArray &inPath,
                                              const QByteArray &inData,
                                              const QByteArray &inShaderType,
                                              const QByteArray &inShaderVersion,
                                              bool inHasGeomShader,
                                              bool inIsComputeShader)
{
    auto foundIt = m_expandedFiles.find(inPath);
    if (foundIt != m_expandedFiles.end())
        foundIt.value() = inData;
    else
        m_expandedFiles.insert(inPath, inData);

    // set shader type and version if available
    if (!inShaderType.isNull() || !inShaderVersion.isNull() || inHasGeomShader || inIsComputeShader) {
        // UdoL TODO: Add this to the load / save setction
        // In addition we should merge the source code into SDynamicObjectShaderInfo as well
        QSSGDynamicObjectShaderInfo &theShaderInfo = m_shaderInfoMap.insert(inPath, QSSGDynamicObjectShaderInfo()).value();
        theShaderInfo.m_type = inShaderType;
        theShaderInfo.m_version = inShaderVersion;
        theShaderInfo.m_hasGeomShader = inHasGeomShader;
        theShaderInfo.m_isComputeShader = inIsComputeShader;
    }
}

QByteArray QSSGDynamicObjectSystem::getShaderCacheKey(const QByteArray &inId,
                                                        const QByteArray &inProgramMacro,
                                                        const dynamic::QSSGDynamicShaderProgramFlags &inFlags)
{
    QByteArray shaderKey = inId;
    if (!inProgramMacro.isEmpty()) {
        shaderKey.append("#");
        shaderKey.append(inProgramMacro);
    }
    if (inFlags & ShaderCacheProgramFlagValues::TessellationEnabled) {
        shaderKey.append("#");
        shaderKey.append(toString(inFlags.tessMode));
    }
    if (inFlags & ShaderCacheProgramFlagValues::GeometryShaderEnabled && inFlags.wireframeMode) {
        shaderKey.append("#");
        shaderKey.append(inFlags.wireframeToString(inFlags.wireframeMode));
    }
    return shaderKey;
}

void QSSGDynamicObjectSystem::resolveIncludeFiles(QByteArray &theReadBuffer, const QByteArray &inPathToEffect)
{
    // Now do search and replace for the headers
    for (int thePos = theReadBuffer.indexOf(includeSearch()); thePos != -1;
         thePos = theReadBuffer.indexOf(includeSearch(), thePos + 1)) {
        int theEndQuote = theReadBuffer.indexOf('\"', thePos + includeSearch().length() + 1);
        // Indicates an unterminated include file.
        if (theEndQuote == -1) {
            qCCritical(INVALID_OPERATION, "Unterminated include in file: %s", inPathToEffect.constData());
            theReadBuffer.clear();
            break;
        }
        const int theActualBegin = thePos + includeSearch().length();
        const auto &theInclude = theReadBuffer.mid(theActualBegin, theEndQuote - theActualBegin);
        // If we haven't included the file yet this round
        auto contents = doLoadShader(theInclude);
        // Strip copywrite headers from include if present
        if (contents.startsWith(copyrightHeaderStart())) {
            int clipPos = contents.indexOf(copyrightHeaderEnd()) ;
            if (clipPos >= 0)
                contents.remove(0, clipPos + copyrightHeaderEnd().count());
        }
        // Write insert comment for begin source
        contents.prepend(QByteArrayLiteral("\n// begin \"") + theInclude + QByteArrayLiteral("\"\n"));
        // Write insert comment for end source
        contents.append(QByteArrayLiteral("\n// end \"" ) + theInclude + QByteArrayLiteral("\"\n"));

        theReadBuffer = theReadBuffer.replace(thePos, (theEndQuote + 1) - thePos, contents);
    }
}

QByteArray QSSGDynamicObjectSystem::doLoadShader(const QByteArray &inPathToEffect)
{
    auto theInsert = m_expandedFiles.find(inPathToEffect);
    const bool found = (theInsert != m_expandedFiles.end());

    QByteArray theReadBuffer;
    if (!found) {
        const QString defaultDir = getShaderCodeLibraryDirectory();
        const QString platformDir = shaderCodeLibraryPlatformDirectory();
        const auto ver = shaderCodeLibraryVersion();

        QString fullPath;
        QSharedPointer<QIODevice> theStream;
        if (!platformDir.isEmpty()) {
            QTextStream stream(&fullPath);
            stream << platformDir << QLatin1Char('/') << QString::fromLocal8Bit(inPathToEffect);
            theStream = m_context->inputStreamFactory()->getStreamForFile(fullPath, true);
        }

        if (theStream.isNull()) {
            fullPath.clear();
            QTextStream stream(&fullPath);
            stream << defaultDir << QLatin1Char('/') << ver << QLatin1Char('/') << QString::fromLocal8Bit(inPathToEffect);
            theStream = m_context->inputStreamFactory()->getStreamForFile(fullPath, true);
            if (theStream.isNull()) {
                fullPath.clear();
                QTextStream stream(&fullPath);
                stream << defaultDir << QLatin1Char('/') << QString::fromLocal8Bit(inPathToEffect);
                theStream = m_context->inputStreamFactory()->getStreamForFile(fullPath, false);
            }
        }
        if (!theStream.isNull()) {
            char readBuf[1024];
            qint64 amountRead = 0;
            do {
                amountRead = theStream->read(readBuf, 1024);
                if (amountRead)
                    theReadBuffer.append(readBuf, int(amountRead));
            } while (amountRead);
        } else {
            qCCritical(INVALID_OPERATION, "Failed to find include file %s", qPrintable(QString::fromLocal8Bit(inPathToEffect)));
            Q_ASSERT(false);
        }
        theInsert = m_expandedFiles.insert(inPathToEffect, theReadBuffer);
    } else {
        theReadBuffer = theInsert.value();
    }
    resolveIncludeFiles(theReadBuffer, inPathToEffect);
    return theReadBuffer;
}

QByteArrayList QSSGDynamicObjectSystem::getParameters(const QByteArray &str, int begin, int end)
{
    const QByteArray s = str.mid(begin, end - begin + 1);
    return s.split(',');
}

void QSSGDynamicObjectSystem::insertSnapperDirectives(QByteArray &str, bool isRhi)
{
    int beginIndex = 0;
    // Snapper macros:
    //  #define SNAPPER_SAMPLER2D(propName, propNiceName, texFilter, texWrap, showUI )
    //      uniform sampler2D propName;
    //      uniform int flag##propName;
    //      uniform vec4 propName##Info;
    //      vec4 texture2D_##propName(vec2 uv)
    //      {
    //          return GetTextureValue( propName, uv, propName##Info.z );
    //      }
    //
    //  #define SNAPPER_SAMPLER2DWITHDEFAULT(propName, propNiceName, texFilter, texWrap, defaultPath, showUI )
    //      SNAPPER_SAMPLER2D( propName, propNiceName, texFilter, texWrap, showUI )
    //
    //  #define SNAPPER_SAMPLERCUBE(propName, propNiceName, texFilter, texWrap )
    //      uniform samplerCube propName;
    //      uniform vec2 propName##UVRange;
    //      uniform int flag##propName;
    //      uniform vec2 propName##Size;
    QByteArray snapperSampler = QByteArrayLiteral("SNAPPER_SAMPLER2D(");
    QByteArray snapperSamplerDefault = QByteArrayLiteral("SNAPPER_SAMPLER2DWITHDEFAULT(");
    QByteArray snapperSamplerCube = QByteArrayLiteral("SNAPPER_SAMPLERCUBE(");
    QByteArray endingBracket = QByteArrayLiteral(")");

    while ((beginIndex = str.indexOf(snapperSampler, beginIndex)) >= 0) {
        int endIndex = str.indexOf(endingBracket, beginIndex);
        const QByteArrayList list = getParameters(str, beginIndex + snapperSampler.length(), endIndex);
        str.remove(beginIndex, endIndex - beginIndex + 1);
        if (list.size() == 5) {
            QByteArray insertStr;
            QTextStream stream(&insertStr);
            if (!isRhi) {
                stream << "uniform sampler2D " << list[0] << ";\n";
                stream << "uniform int flag" << list[0] << ";\n";
                stream << "vec4 " << list[0] << "Info;\n";
            }
            stream << "vec4 texture2D_" << list[0] << "(vec2 uv) "
                   << "{ return GetTextureValue( " << list[0] << ", uv, " << list[0] << "Info.z ); }\n";
            stream.flush();
            str.insert(beginIndex, insertStr);
        }
    }
    beginIndex = 0;
    while ((beginIndex = str.indexOf(snapperSamplerDefault, beginIndex)) >= 0) {
        int endIndex = str.indexOf(endingBracket, beginIndex);
        const QByteArrayList list = getParameters(str, beginIndex + snapperSamplerDefault.length(), endIndex);
        str.remove(beginIndex, endIndex - beginIndex + 1);
        if (list.size() == 5) {
            QByteArray insertStr;
            QTextStream stream(&insertStr);
            if (!isRhi) {
                stream << "uniform sampler2D " << list[0] << ";\n";
                stream << "uniform int flag" << list[0] << ";\n";
                stream << "vec4 " << list[0] << "Info;\n";
            }
            stream << "vec4 texture2D_" << list[0] << "(vec2 uv) "
                   << "{ return GetTextureValue( " << list[0] << ", uv, " << list[0] << "Info.z ); }\n";
            stream.flush();
            str.insert(beginIndex, insertStr);
        }
    }
    beginIndex = 0;
    while (!isRhi && (beginIndex = str.indexOf(snapperSamplerCube, beginIndex)) >= 0) {
        int endIndex = str.indexOf(endingBracket, beginIndex);
        const QByteArrayList list = getParameters(str, beginIndex + snapperSamplerCube.length(), endIndex);
        str.remove(beginIndex, endIndex - beginIndex + 1);
        if (list.size() == 4) {
            QByteArray insertStr;
            QTextStream stream(&insertStr);
            stream << "uniform samplerCube " << list[0] << ";\n";
            stream << "uniform vec2 " << list[0] << "UVRange;\n";
            stream << "uniform int flag" << list[0] << ";\n";
            stream << "uniform vec2 " << list[0] << "Size;\n";
            stream.flush();
            str.insert(beginIndex, insertStr);
        }
    }
}

// this function is hit only with effects, not with custom materials - except for the custom material depth prepass shader
QSSGRef<QSSGRenderShaderProgram> QSSGDynamicObjectSystem::compileShader(const QByteArray &inId,
                                                                              const QByteArray &inProgramSource,
                                                                              const QByteArray &inGeomSource,
                                                                              const QByteArray &inProgramMacroName,
                                                                              const ShaderFeatureSetList &inFeatureSet,
                                                                              const dynamic::QSSGDynamicShaderProgramFlags &inFlags,
                                                                              bool inForceCompilation)
{
    m_vertShader.clear();
    m_fragShader.clear();
    m_geometryShader.clear();
    QSSGShaderCacheProgramFlags theFlags;

    m_vertShader.append("#define VERTEX_SHADER\n");
    m_fragShader.append("#define FRAGMENT_SHADER\n");

    if (!inProgramMacroName.isEmpty()) {
        m_vertShader.append("#define ");
        m_vertShader.append(inProgramMacroName);
        m_vertShader.append("\n");

        m_fragShader.append("#define ");
        m_fragShader.append(inProgramMacroName);
        m_fragShader.append("\n");
    }

    if (!inGeomSource.isEmpty() && inFlags & ShaderCacheProgramFlagValues::GeometryShaderEnabled) {
        theFlags |= ShaderCacheProgramFlagValues::GeometryShaderEnabled;

        m_geometryShader.append("#define GEOMETRY_SHADER 1\n");
        m_geometryShader.append(inGeomSource);

        m_vertShader.append("#define GEOMETRY_SHADER 1\n");
    } else if (inFlags & ShaderCacheProgramFlagValues::GeometryShaderEnabled) {
        theFlags |= ShaderCacheProgramFlagValues::GeometryShaderEnabled;
        m_geometryShader.append("#define USER_GEOMETRY_SHADER 1\n");
        m_geometryShader.append(inProgramSource);
        m_vertShader.append("#define GEOMETRY_SHADER 0\n");
        m_fragShader.append("#define GEOMETRY_WIREFRAME_SHADER 0\n");
    } else {
        m_vertShader.append("#define GEOMETRY_SHADER 0\n");
        m_fragShader.append("#define GEOMETRY_WIREFRAME_SHADER 0\n");
    }

    if (strstr(inProgramSource, "SNAPPER_SAMPLER")) {
        QByteArray programSource = inProgramSource;
        insertSnapperDirectives(programSource);
        const char *source = programSource.constData();

        m_vertShader.append(source);
        m_fragShader.append(source);
    } else {
        m_vertShader.append(inProgramSource);
        m_fragShader.append(inProgramSource);
    }

    QSSGRef<QSSGShaderCache> theShaderCache = m_context->shaderCache();

    QByteArray theKey = getShaderCacheKey(inId, inProgramMacroName, inFlags);
    if (inForceCompilation) {
        return theShaderCache->forceCompileProgram(theKey, m_vertShader, m_fragShader, nullptr, nullptr, m_geometryShader, theFlags, inFeatureSet, false);
    }

    return theShaderCache->compileProgram(theKey, m_vertShader, m_fragShader, nullptr, nullptr, m_geometryShader, theFlags, inFeatureSet);
}

QByteArray QSSGDynamicObjectSystem::getShaderSource(const QByteArray &inPath)
{
    return doLoadShader(inPath);
}

// used only with effects, not used with custom materials
TShaderAndFlags QSSGDynamicObjectSystem::getShaderProgram(const QByteArray &inPath,
                                                            const QByteArray &inProgramMacro,
                                                            const ShaderFeatureSetList &inFeatureSet,
                                                            const dynamic::QSSGDynamicShaderProgramFlags &inFlags,
                                                            bool inForceCompilation)
{
    dynamic::QSSGDynamicShaderMapKey shaderMapKey(TStrStrPair(inPath, inProgramMacro), inFeatureSet, inFlags.tessMode, inFlags.wireframeMode);
    auto theInserter = m_shaderMap.find(shaderMapKey);
    const bool found = (theInserter != m_shaderMap.end());

    if (!found)
        theInserter = m_shaderMap.insert(shaderMapKey, TShaderAndFlags());

    // TODO: This looks funky (if found)...
    if (found || inForceCompilation) {
        QSSGRef<QSSGRenderShaderProgram> theProgram = m_context->shaderCache()
                                                                  ->getProgram(getShaderCacheKey(inPath,
                                                                                                 inProgramMacro,
                                                                                                 inFlags),
                                                                               inFeatureSet);
        dynamic::QSSGDynamicShaderProgramFlags theFlags(inFlags);
        if (!theProgram || inForceCompilation) {
            QSSGDynamicObjectShaderInfo
                    &theShaderInfo = m_shaderInfoMap.insert(inPath, QSSGDynamicObjectShaderInfo()).value();
            if (!theShaderInfo.m_isComputeShader) {
                QByteArray programSource = doLoadShader(inPath);
                if (theShaderInfo.m_hasGeomShader)
                    theFlags |= ShaderCacheProgramFlagValues::GeometryShaderEnabled;
                theProgram = compileShader(inPath, programSource.constData(), nullptr, inProgramMacro, inFeatureSet, theFlags, inForceCompilation);
            } else {
                QByteArray theShaderBuffer;
                const char *shaderVersionStr = "#version 430\n";
                if (m_context->renderContext()->renderContextType() == QSSGRenderContextType::GLES3PLUS)
                    shaderVersionStr = "#version 310 es\n";
                theShaderBuffer = doLoadShader(inPath);
                theShaderBuffer.insert(0, shaderVersionStr);
                theProgram = m_context->renderContext()->compileComputeSource(inPath, toByteView(theShaderBuffer)).m_shader;
            }
        }
        theInserter.value() = TShaderAndFlags(theProgram, theFlags);
    }
    return theInserter.value();
}

TShaderAndFlags QSSGDynamicObjectSystem::getDepthPrepassShader(const QByteArray &inPath, const QByteArray &inPMacro, const ShaderFeatureSetList &inFeatureSet)
{
    QSSGDynamicObjectShaderInfo &theShaderInfo = m_shaderInfoMap.insert(inPath, QSSGDynamicObjectShaderInfo()).value();
    if (!theShaderInfo.m_hasGeomShader)
        return TShaderAndFlags();
    // else, here we go...
    dynamic::QSSGDynamicShaderProgramFlags theFlags;
    const QByteArray shaderKey = inPMacro + QByteArrayLiteral("depthprepass");

    const QByteArray &theProgramMacro = shaderKey;

    const dynamic::QSSGDynamicShaderMapKey shaderMapKey(TStrStrPair(inPath, theProgramMacro),
                                                          inFeatureSet,
                                                          theFlags.tessMode,
                                                          theFlags.wireframeMode);
    const TShaderAndFlags shaderFlags;
    auto theInserter = m_shaderMap.find(shaderMapKey);
    const bool found = theInserter != m_shaderMap.end();
    if (found) {
        QSSGRef<QSSGRenderShaderProgram> theProgram = m_context->shaderCache()
                                                                  ->getProgram(getShaderCacheKey(inPath,
                                                                                                 theProgramMacro,
                                                                                                 theFlags),
                                                                               inFeatureSet);
        dynamic::QSSGDynamicShaderProgramFlags flags(theFlags);
        if (!theProgram) {
            QByteArray geomSource = doLoadShader(inPath);
            QSSGShaderVertexCodeGenerator vertexShader(m_context->renderContext()->renderContextType());
            QSSGShaderFragmentCodeGenerator fragmentShader(vertexShader, m_context->renderContext()->renderContextType());

            vertexShader.addAttribute("attr_pos", "vec3");
            vertexShader.addUniform("modelViewProjection", "mat4");
            vertexShader.append("void main() {");
            vertexShader.append("\tgl_Position = modelViewProjection * vec4(attr_pos, 1.0);");
            vertexShader.append("}");
            fragmentShader.append("void main() {");
            fragmentShader.append("\tfragOutput = vec4(0.0, 0.0, 0.0, 0.0);");
            fragmentShader.append("}");
            QByteArray vertexSource = vertexShader.buildShaderSource();
            QByteArray fragmentSource = fragmentShader.buildShaderSource();

            QByteArray programBuffer(QByteArrayLiteral("#ifdef VERTEX_SHADER\n"));
            programBuffer.append(vertexSource);
            programBuffer.append("\n#endif\n");
            programBuffer.append("\n#ifdef FRAGMENT_SHADER\n");
            programBuffer.append(fragmentSource);
            programBuffer.append("\n#endif");
            flags |= ShaderCacheProgramFlagValues::GeometryShaderEnabled;
            theProgram = compileShader(inPath, programBuffer, geomSource, theProgramMacro, inFeatureSet, flags);
        }
        theInserter.value() = TShaderAndFlags(theProgram, flags);
    }
    return theInserter.value();
}

void QSSGDynamicObjectSystem::setShaderCodeLibraryVersion(const QByteArray &version)
{
    m_shaderLibraryVersion = version;
}

QByteArray QSSGDynamicObjectSystem::shaderCodeLibraryVersion()
{
    return m_shaderLibraryVersion;
}

void QSSGDynamicObjectSystem::setShaderCodeLibraryPlatformDirectory(const QString &directory)
{
    m_shaderLibraryPlatformDirectory = directory;
}

QString QSSGDynamicObjectSystem::shaderCodeLibraryPlatformDirectory()
{
    return m_shaderLibraryPlatformDirectory;
}

QT_END_NAMESPACE
