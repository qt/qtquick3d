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

#include <QXmlStreamReader>
#include <QFileInfo>
#include <QCryptographicHash>

#include <QtQuick3DRuntimeRender/private/qssgruntimerenderlogging_p.h>

QT_BEGIN_NAMESPACE

QString QSSGShaderLibraryManager::getShaderCodeLibraryDirectory()
{
    return QStringLiteral("res/effectlib");
}
static QByteArray includeSearch() { return QByteArrayLiteral("#include \""); };
static QByteArray copyrightHeaderStart() { return QByteArrayLiteral("/****************************************************************************"); }
static QByteArray copyrightHeaderEnd() { return QByteArrayLiteral("****************************************************************************/"); }

QSSGShaderLibraryManager::QSSGShaderLibraryManager(const QSSGRef<QSSGInputStreamFactory> &inputStreamFactory)
    : m_inputStreamFactory(inputStreamFactory)
{
}

QSSGShaderLibraryManager::~QSSGShaderLibraryManager() {}

void QSSGShaderLibraryManager::setShaderSource(const QByteArray &inShaderPathKey, const QByteArray &inSource)
{
    auto foundIt = m_expandedFiles.find(inShaderPathKey);
    if (foundIt != m_expandedFiles.end())
        foundIt.value() = inSource;
    else
        m_expandedFiles.insert(inShaderPathKey, inSource);
}

static inline char stageKey(QSSGShaderCache::ShaderType type)
{
    switch (type) {
    case QSSGShaderCache::ShaderType::Vertex:
        return 'V';
    case QSSGShaderCache::ShaderType::Fragment:
        return 'F';
    default:
        break;
    }
    return '?';
}

void QSSGShaderLibraryManager::setShaderSource(const QByteArray &inShaderPathKey, QSSGShaderCache::ShaderType type,
                                               const QByteArray &inSource, const QSSGCustomShaderMetaData &meta)
{
    const QByteArray perStageKey = stageKey(type) + inShaderPathKey;
    setShaderSource(perStageKey, inSource);
    auto it = m_metadata.find(perStageKey);
    if (it != m_metadata.end())
        it.value() = meta;
    else
        m_metadata.insert(perStageKey, meta);
}

void QSSGShaderLibraryManager::resolveIncludeFiles(QByteArray &theReadBuffer, const QByteArray &inMaterialInfoString)
{
    // Now do search and replace for the headers
    for (int thePos = theReadBuffer.indexOf(includeSearch()); thePos != -1;
         thePos = theReadBuffer.indexOf(includeSearch(), thePos + 1)) {
        int theEndQuote = theReadBuffer.indexOf('\"', thePos + includeSearch().length() + 1);
        // Indicates an unterminated include file.
        if (theEndQuote == -1) {
            qCCritical(INVALID_OPERATION, "Unterminated include in file: %s", inMaterialInfoString.constData());
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

QByteArrayList QSSGShaderLibraryManager::getParameters(const QByteArray &str, int begin, int end)
{
    const QByteArray s = str.mid(begin, end - begin + 1);
    return s.split(',');
}

void QSSGShaderLibraryManager::insertSnapperDirectives(QByteArray &str)
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

QByteArray QSSGShaderLibraryManager::getShaderSource(const QByteArray &inShaderPathKey)
{
    auto theInsert = m_expandedFiles.find(inShaderPathKey);
    const bool found = (theInsert != m_expandedFiles.end());

    QByteArray theReadBuffer;
    if (!found) {
        const QString defaultDir = getShaderCodeLibraryDirectory();
        const auto ver = QByteArrayLiteral("rhi");

        QString fullPath;
        QSharedPointer<QIODevice> theStream;
        QTextStream stream(&fullPath);
        stream << defaultDir << QLatin1Char('/') << ver << QLatin1Char('/') << QString::fromLocal8Bit(inShaderPathKey);
        theStream = m_inputStreamFactory->getStreamForFile(fullPath, true);
        if (theStream.isNull()) {
            fullPath.clear();
            QTextStream stream(&fullPath);
            stream << defaultDir << QLatin1Char('/') << QString::fromLocal8Bit(inShaderPathKey);
            theStream = m_inputStreamFactory->getStreamForFile(fullPath, false);
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
            qCCritical(INVALID_OPERATION, "Failed to find include file %s", qPrintable(QString::fromLocal8Bit(inShaderPathKey)));
            Q_ASSERT(false);
        }
        theInsert = m_expandedFiles.insert(inShaderPathKey, theReadBuffer);
    } else {
        theReadBuffer = theInsert.value();
    }
    resolveIncludeFiles(theReadBuffer, inShaderPathKey);
    return theReadBuffer;
}

QByteArray QSSGShaderLibraryManager::getShaderSource(const QByteArray &inShaderPathKey, QSSGShaderCache::ShaderType type)
{
    const QByteArray perStageKey = stageKey(type) + inShaderPathKey;
    auto it = m_expandedFiles.find(perStageKey);
    if (it != m_expandedFiles.end())
        return it.value();

    qWarning("No shader source stored for key %s", inShaderPathKey.constData());
    return QByteArray();
}

QSSGCustomShaderMetaData QSSGShaderLibraryManager::getShaderMetaData(const QByteArray &inShaderPathKey, QSSGShaderCache::ShaderType type)
{
    const QByteArray perStageKey = stageKey(type) + inShaderPathKey;
    auto it = m_metadata.find(perStageKey);
    if (it != m_metadata.end())
        return it.value();

    qWarning("No shader metadata stored for key %s", inShaderPathKey.constData());
    return {};
}

void QSSGShaderLibraryManager::loadPregeneratedShaderInfo(const QByteArray& keyfile)
{
    QFile readFile(QString::fromLocal8Bit(keyfile));
    if (!readFile.open(QFile::ReadOnly | QFile::Text))
        return;

    QVector<QString> keyFiles;
    QXmlStreamReader reader(&readFile);
    while (reader.readNextStartElement()){
        if (reader.name() == QStringLiteral("keys")) {
            while (reader.readNextStartElement()) {
                if (reader.name() == QStringLiteral("key"))
                    keyFiles.push_back(reader.readElementText());
                else
                    reader.skipCurrentElement();
            }
        } else {
            reader.skipCurrentElement();
        }
    }
    auto getSha = [](const QString &filename) {
        QFileInfo info(filename);
        auto name = info.fileName();
        return name.left(name.lastIndexOf(QStringLiteral("."))).toUtf8();
    };
    auto readKeyBytes = [](const QString &filename) {
        QFile inFile(filename);
        if (inFile.open(QFile::Text | QFile::ReadOnly))
            return inFile.readAll();
        return QByteArray();
    };
    QElapsedTimer timer;
    timer.start();
    for (const auto &f : keyFiles) {
        QByteArray keySha = getSha(f);
        QByteArray shaderKey = readKeyBytes(QChar(u':') + f);
        QSSGShaderDefaultMaterialKey key;
        key.fromByteArray(shaderKey);
        m_shaderKeys.insert(keySha, key);
    }
    qCDebug(PERF_INFO, "PregeneratedShaderInfo loaded in %lld ms", timer.elapsed());
}

static int calcLightPoint(const QSSGShaderDefaultMaterialKey &key, int i) {
    QSSGShaderDefaultMaterialKeyProperties prop;
    return prop.m_lightFlags[i].getValue(key) + prop.m_lightSpotFlags[i].getValue(key) * 2
            + prop.m_lightAreaFlags[i].getValue(key) * 4 + prop.m_lightShadowFlags[i].getValue(key) * 8;
};

bool QSSGShaderLibraryManager::compare(const QSSGShaderDefaultMaterialKey &key1, const QSSGShaderDefaultMaterialKey &key2)
{
    QSSGShaderDefaultMaterialKeyProperties props;
#define COMPARE_PROP(x) \
    if (props.x.getValue(key1) < props.x.getValue(key2)) return true;

    COMPARE_PROP(m_hasLighting)
    COMPARE_PROP(m_hasIbl)
    COMPARE_PROP(m_specularEnabled)
    COMPARE_PROP(m_fresnelEnabled)
    COMPARE_PROP(m_vertexColorsEnabled)
    COMPARE_PROP(m_specularModel)
    COMPARE_PROP(m_vertexAttributes)
    COMPARE_PROP(m_alphaMode)

    for (int i = 0; i < QSSGShaderDefaultMaterialKeyProperties::ImageMapCount; i++) {
        COMPARE_PROP(m_imageMaps[i])
        COMPARE_PROP(m_textureSwizzle[i])
    }
    for (int i = 0; i < QSSGShaderDefaultMaterialKeyProperties::SingleChannelImageCount; i++) {
        COMPARE_PROP(m_textureChannels[i])
    }
    COMPARE_PROP(m_lightCount)
    for (int i = 0; i < QSSGShaderDefaultMaterialKeyProperties::LightCount; i++) {
        int lp1 = calcLightPoint(key1, i);
        int lp2 = calcLightPoint(key2, i);
        if (lp1 < lp2)
            return true;
    }
#undef COMPARE_PROP
    return false;
}

QT_END_NAMESPACE
