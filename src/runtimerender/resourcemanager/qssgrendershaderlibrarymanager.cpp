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

#include "qssgrendershaderlibrarymanager_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

QT_BEGIN_NAMESPACE

QString QSSGShaderLibraryManger::getShaderCodeLibraryDirectory()
{
    return QStringLiteral("res/effectlib");
}
static QByteArray includeSearch() { return QByteArrayLiteral("#include \""); };
static QByteArray copyrightHeaderStart() { return QByteArrayLiteral("/****************************************************************************"); }
static QByteArray copyrightHeaderEnd() { return QByteArrayLiteral("****************************************************************************/"); }


QSSGShaderLibraryManger::QSSGShaderLibraryManger(QSSGRenderContextInterface *ctx)
    : m_context(ctx)
{
}

QSSGShaderLibraryManger::~QSSGShaderLibraryManger() {}

void QSSGShaderLibraryManger::setShaderData(const QByteArray &inPath,
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
        QSSGShaderInfo &theShaderInfo = m_shaderInfoMap.insert(inPath, QSSGShaderInfo()).value();
        theShaderInfo.m_type = inShaderType;
        theShaderInfo.m_version = inShaderVersion;
        theShaderInfo.m_hasGeomShader = inHasGeomShader;
        theShaderInfo.m_isComputeShader = inIsComputeShader;
    }
}

void QSSGShaderLibraryManger::resolveIncludeFiles(QByteArray &theReadBuffer, const QByteArray &inPath)
{
    // Now do search and replace for the headers
    for (int thePos = theReadBuffer.indexOf(includeSearch()); thePos != -1;
         thePos = theReadBuffer.indexOf(includeSearch(), thePos + 1)) {
        int theEndQuote = theReadBuffer.indexOf('\"', thePos + includeSearch().length() + 1);
        // Indicates an unterminated include file.
        if (theEndQuote == -1) {
            qCCritical(INVALID_OPERATION, "Unterminated include in file: %s", inPath.constData());
            theReadBuffer.clear();
            break;
        }
        const int theActualBegin = thePos + includeSearch().length();
        const auto &theInclude = theReadBuffer.mid(theActualBegin, theEndQuote - theActualBegin);
        // If we haven't included the file yet this round
        auto contents = getShaderSource(theInclude);
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

QByteArrayList QSSGShaderLibraryManger::getParameters(const QByteArray &str, int begin, int end)
{
    const QByteArray s = str.mid(begin, end - begin + 1);
    return s.split(',');
}

void QSSGShaderLibraryManger::insertSnapperDirectives(QByteArray &str)
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
    QByteArray endingBracket = QByteArrayLiteral(")");

    while ((beginIndex = str.indexOf(snapperSampler, beginIndex)) >= 0) {
        int endIndex = str.indexOf(endingBracket, beginIndex);
        const QByteArrayList list = getParameters(str, beginIndex + snapperSampler.length(), endIndex);
        str.remove(beginIndex, endIndex - beginIndex + 1);
        if (list.size() == 5) {
            QByteArray insertStr;
            QTextStream stream(&insertStr);
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
            stream << "vec4 texture2D_" << list[0] << "(vec2 uv) "
                   << "{ return GetTextureValue( " << list[0] << ", uv, " << list[0] << "Info.z ); }\n";
            stream.flush();
            str.insert(beginIndex, insertStr);
        }
    }
}

QByteArray QSSGShaderLibraryManger::getShaderSource(const QByteArray &inPath)
{
    auto theInsert = m_expandedFiles.find(inPath);
    const bool found = (theInsert != m_expandedFiles.end());

    QByteArray theReadBuffer;
    if (!found) {
        const QString defaultDir = getShaderCodeLibraryDirectory();
        const auto ver = QByteArrayLiteral("rhi");

        QString fullPath;
        QSharedPointer<QIODevice> theStream;
        QTextStream stream(&fullPath);
        stream << defaultDir << QLatin1Char('/') << ver << QLatin1Char('/') << QString::fromLocal8Bit(inPath);
        theStream = m_context->inputStreamFactory()->getStreamForFile(fullPath, true);
        if (theStream.isNull()) {
            fullPath.clear();
            QTextStream stream(&fullPath);
            stream << defaultDir << QLatin1Char('/') << QString::fromLocal8Bit(inPath);
            theStream = m_context->inputStreamFactory()->getStreamForFile(fullPath, false);
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
            qCCritical(INVALID_OPERATION, "Failed to find include file %s", qPrintable(QString::fromLocal8Bit(inPath)));
            Q_ASSERT(false);
        }
        theInsert = m_expandedFiles.insert(inPath, theReadBuffer);
    } else {
        theReadBuffer = theInsert.value();
    }
    resolveIncludeFiles(theReadBuffer, inPath);
    return theReadBuffer;
}

QT_END_NAMESPACE
