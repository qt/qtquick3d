/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "materialadapter.h"

#include <QtQuick3D/private/qquick3dshaderutils_p.h>
#include <QtQuick3D/private/qquick3dobject_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/QDataStream>

#include <QtCore/qbuffer.h>

#include <QtQuick3DAssetUtils/private/qssgrtutilities_p.h>
#include <QtQuick3DAssetUtils/private/qssgqmlutilities_p.h>

#include <QtGui/QImageReader>

#include "uniformmodel.h"

QT_BEGIN_NAMESPACE

namespace  {
class CustomMaterialExposed : public QQuick3DCustomMaterial
{
public:
    using Dirty = QQuick3DCustomMaterial::Dirty;
    using QQuick3DCustomMaterial::markDirty;
    CustomMaterialExposed() = delete;
};
}

enum class ShaderType
{
    Vertex,
    Fragment
};

static QString getScheme() { return u"q3dres"_qs; }
static QString getUserType() { return u"material"_qs; }
static QUrl materialShaderUrl(const QUrl &url) { return QUrl(getScheme() + "://" + getUserType() + '@' + url.fileName()); }
static constexpr QStringView fileSuffix(ShaderType type) { return (type == ShaderType::Vertex) ? u".vert" : u".frag"; }

static QUrl defaultShaderUrl(ShaderType type)
{
    return QUrl(getScheme() + "://material@editor" + fileSuffix(type).toString());
}

using BuilderPtr = QPointer<MaterialAdapter>;
Q_GLOBAL_STATIC(BuilderPtr, builderInstance);

[[nodiscard]] static BuildMessage parseErrorMessage(const QString &errorMsg)
{
    // "ERROR: :1: '' :  syntax error, unexpected IDENTIFIER"
    const QString head = QString::fromLatin1("ERROR:");
    qint32 lineNr = -1;
    qint32 columnNr = -1;
    QString identifier;
    auto msg = errorMsg;
    if (errorMsg.startsWith(head)) {
        auto pos = head.length();
        auto idx = errorMsg.indexOf(u':', pos);
        if (idx > pos && idx < pos + 16 /* sanity check */) {
            pos = idx;
            idx = errorMsg.indexOf(u':', pos + 1);
            // Line nr
            if (idx > pos && idx < pos + 6 /* sanity check */) {
                auto mid = errorMsg.mid(pos + 1, idx - pos - 1);
                bool ok = false;
                auto v = mid.toInt(&ok);
                if (ok) {
                    lineNr = v;
                    pos = idx;
                }

                // check if we have a symbol (this might be empty)
                idx = errorMsg.indexOf(u'\'', pos + 1);
                if (idx > pos) {
                    pos = idx;
                    idx = errorMsg.indexOf(u'\'', pos + 1);
                    if (idx > pos && idx > pos + 1)
                        identifier = errorMsg.mid(pos + 1, idx - pos - 1);
                }

                // Find the message
                idx = errorMsg.indexOf(u':', pos + 1);
                if (idx > pos)
                    msg = errorMsg.mid(idx + 1).trimmed();
            }
        }
    }
    return BuildMessage{ msg, identifier, lineNr, columnNr, BuildMessage::Status::Error };
}

// NOTE: We're being called from the render thread here...
void MaterialAdapter::bakerStatusCallback(const QByteArray &descKey, QtQuick3DEditorHelpers::ShaderBaker::Status status, const QString &err, QShader::Stage stage)
{
    (void)descKey;
    if (auto that = (*builderInstance)) {
        using namespace QtQuick3DEditorHelpers::ShaderBaker;
        if (status == Status::Success) {
            if (stage == QShader::Stage::VertexStage) {
                auto fileName = (!that->m_vertexUrl.isEmpty()) ? that->m_vertexUrl.path() : QLatin1String("<VERT_BUFFER>");
                that->m_vertexMsg= { BuildMessage{}, fileName, ShaderBuildMessage::Stage::Vertex };
                Q_EMIT that->vertexStatusChanged();
            } else {
                auto fileName = (!that->m_fragUrl.isEmpty()) ? that->m_fragUrl.path() : QLatin1String("<FRAG_BUFFER>");
                that->m_fragmentMsg = { BuildMessage{}, fileName, ShaderBuildMessage::Stage::Fragment };
                Q_EMIT that->fragmentStatusChanged();
            }
        } else if (status == Status::Error) {
            const auto errList = err.split(u'\n');
            if (errList.size() > 0) {
                auto statusMessage = parseErrorMessage(errList.first());
                if (stage == QShader::Stage::VertexStage) {
                    auto fileName = (!that->m_vertexUrl.isEmpty()) ? that->m_vertexUrl.path() : QLatin1String("<VERT_BUFFER>");
                    that->m_vertexMsg = { statusMessage, fileName, ShaderBuildMessage::Stage::Vertex };
                    Q_EMIT that->vertexStatusChanged();
                } else {
                    auto fileName = (!that->m_fragUrl.isEmpty()) ? that->m_fragUrl.path() : QLatin1String("<FRAG_BUFFER>");
                    that->m_fragmentMsg = { statusMessage, fileName, ShaderBuildMessage::Stage::Fragment };
                    Q_EMIT that->fragmentStatusChanged();
                }
    #if 0
                const auto shaderUrl =  (stage == QShader::Stage::VertexStage) ? that->m_vertexUrl : that->m_fragUrl;
                qDebug() << shaderUrl.host() << "=>" << that->m_error;
    #endif
            }
        } else {
            Q_UNREACHABLE();
        }
    }
}

// NOTE: Called from the sync phase.
static bool resolveShader(const QUrl &url, const QQmlContext *context, QByteArray &shaderData, QByteArray &shaderPathKey)
{
    Q_UNUSED(context);
    Q_UNUSED(shaderPathKey);
    if (auto that = (*builderInstance)) {
        if (url.scheme() == getScheme() && url.userInfo() == getUserType()) {
            const auto filenName = url.host();
            if (filenName.endsWith(fileSuffix(ShaderType::Fragment))) {
                shaderData = that->fragmentShader().toUtf8();
                return true;
            }

            if (filenName.endsWith(fileSuffix(ShaderType::Vertex))) {
                shaderData = that->vertexShader().toUtf8();
                return true;
            }
        } else {
            const QUrl loadUrl = context ? context->resolvedUrl(url) : url;
            const auto path = (loadUrl.scheme() == u"qrc") ? QDir::currentPath() + loadUrl.path()
                                                           : loadUrl.path();
            QFile f(path);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                shaderData = f.readAll();
                if (path.endsWith(fileSuffix(ShaderType::Vertex)))
                    that->setVertexShader(shaderData);
                else if (path.endsWith(fileSuffix(ShaderType::Fragment)))
                    that->setFragmentShader(shaderData);
                return true;
            }
        }
    }

    return false;
}

void MaterialAdapter::updateShader(QQuick3DMaterial &target)
{
    if (QQuick3DObjectPrivate::get(&target)->type == QQuick3DObjectPrivate::Type::CustomMaterial) {
        QQuick3DCustomMaterial &material = static_cast<QQuick3DCustomMaterial &>(target);
        // We mark the material as dirty, this will trigger the material to reload the
        // shader source file, which then again will trigger our resolveShader() function.
        CustomMaterialExposed::markDirty(material, CustomMaterialExposed::Dirty::ShaderSettingsDirty);
        CustomMaterialExposed::markDirty(material, CustomMaterialExposed::Dirty::DynamicPropertiesDirty);
    }
}

void MaterialAdapter::updateMaterialDescription()
{
    // TODO: We might need to make some more clean-up of textures and front-end nodes
    // that are now replaced, but leaving as-is for now.
    auto oldMaterial = m_material;
    if (m_resourceRoot != nullptr) {
        CustomMaterial::Shaders shaders { !m_materialDescr.shaders.vert.isEmpty() ? m_materialDescr.shaders.vert : defaultShaderUrl(ShaderType::Vertex),
                                          !m_materialDescr.shaders.frag.isEmpty() ? m_materialDescr.shaders.frag : defaultShaderUrl(ShaderType::Fragment) };
        if (auto v = m_materialDescr.create(*m_resourceRoot, uniformTable, m_properties, shaders)) {
            m_material = v;
            Q_EMIT materialChanged();
        }
    }
}

MaterialAdapter::MaterialAdapter(QObject *parent)
    : QObject(parent)
{
    // NOTE (todo?): As-is this means there can only be one ShaderAdapter per process.
    Q_ASSERT((*builderInstance).isNull());
    (*builderInstance) = this;
    QSSGShaderUtils::setResolveFunction(&resolveShader);
    QtQuick3DEditorHelpers::ShaderBaker::setStatusCallback(&bakerStatusCallback);
}

QQuick3DCustomMaterial *MaterialAdapter::material() const
{
    return m_material;
}

QString MaterialAdapter::fragmentShader() const
{
    return m_fragmentShader;
}

void MaterialAdapter::setFragmentShader(const QString &newFragmentShader)
{
    if (m_fragmentShader == newFragmentShader)
        return;

    m_fragmentShader = newFragmentShader;
    emit fragmentShaderChanged();
    setUnsavedChanges(true);

    if (m_material)
        updateShader(*m_material);
}

QString MaterialAdapter::vertexShader() const
{
    return m_vertexShader;
}

void MaterialAdapter::setVertexShader(const QString &newVertexShader)
{
    if (m_vertexShader == newVertexShader)
        return;

    m_vertexShader = newVertexShader;
    emit vertexShaderChanged();
    setUnsavedChanges(true);

    if (m_material)
        updateShader(*m_material);
}

ShaderBuildMessage MaterialAdapter::vertexStatus() const
{
    return m_vertexMsg;
}

ShaderBuildMessage MaterialAdapter::fragmentStatus() const
{
    return m_fragmentMsg;
}

QString MaterialAdapter::importShader(const QUrl &shaderFile)
{
    QString shaderContents;
    QFile file = resolveFileFromUrl(shaderFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        shaderContents = file.readAll();
    else
        qWarning() << "Could not open shader file: " << file.fileName();


    return shaderContents;
}

QFile MaterialAdapter::resolveFileFromUrl(const QUrl &fileUrl)
{
    const QQmlContext *context = qmlContext(this);
    const auto resolvedUrl = context ? context->resolvedUrl(fileUrl) : fileUrl;
    const auto qmlSource = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);

    QFileInfo fileInfo(qmlSource);
    QString filePath = fileInfo.canonicalFilePath();
    if (filePath.isEmpty())
        filePath = fileInfo.absoluteFilePath();
    return QFile(filePath);
}

void MaterialAdapter::importFragmentShader(const QUrl &shaderFile)
{
    setFragmentShader(importShader(shaderFile));
}

void MaterialAdapter::importVertexShader(const QUrl &shaderFile)
{
    setVertexShader(importShader(shaderFile));
}

bool MaterialAdapter::save()
{
    if (!m_materialSaveFile.isEmpty())
        return saveMaterial(m_materialSaveFile);
    return false;
}

static const quint32 MATERIAL_MAGIC = 3365961549;
static const quint32 MATERIAL_VERSION = 1;

bool MaterialAdapter::saveMaterial(const QUrl &materialFile)
{
    auto saveFile = resolveFileFromUrl(materialFile);
    if (saveFile.open(QIODevice::WriteOnly)) {
        QDataStream out(&saveFile);
        out.setByteOrder(QDataStream::LittleEndian);
        out.setFloatingPointPrecision(QDataStream::SinglePrecision);
        out.setVersion(QDataStream::Qt_6_3);
        out << MATERIAL_MAGIC << MATERIAL_VERSION;
        out << m_vertexShader;
        out << m_fragmentShader;
        out << m_material->srcBlend();
        out << m_material->dstBlend();
        out << m_material->cullMode();
        out << m_material->depthDrawMode();
        out << m_material->shadingMode();
        // Uniforms
        out << uniformTable.count();
        for (const auto &uniform : qAsConst(uniformTable))
            out << uniform;
    } else {
        emit errorOccurred();
        return false;
    }

    setUnsavedChanges(false);
    setMaterialSaveFile(materialFile);
    emit postMaterialSaved();
    return true;
}

bool MaterialAdapter::loadMaterial(const QUrl &materialFile)
{
    auto loadFile = resolveFileFromUrl(materialFile);
    if (loadFile.open(QIODevice::ReadOnly)) {
        QDataStream in(&loadFile);
        in.setByteOrder(QDataStream::LittleEndian);
        in.setFloatingPointPrecision(QDataStream::SinglePrecision);
        in.setVersion(QDataStream::Qt_6_3);
        quint32 magic = 0;
        quint32 version = 0;
        in >> magic >> version;
        if (magic != MATERIAL_MAGIC && version < MATERIAL_VERSION)
            return false;
        QString vertexShader;
        QString fragmentShader;
        in >> vertexShader >> fragmentShader;
        setVertexShader(vertexShader);
        setFragmentShader(fragmentShader);
        QQuick3DCustomMaterial::BlendMode sourceBlend;
        QQuick3DCustomMaterial::BlendMode destBlend;
        QQuick3DMaterial::CullMode cullMode;
        QQuick3DMaterial::DepthDrawMode depthDrawMode;
        QQuick3DCustomMaterial::ShadingMode shadingMode;
        in >> sourceBlend >> destBlend >> cullMode >> depthDrawMode >> shadingMode;
        m_material->setSrcBlend(sourceBlend);
        m_material->setDstBlend(destBlend);
        m_material->setCullMode(cullMode);
        m_material->setDepthDrawMode(depthDrawMode);
        m_material->setShadingMode(shadingMode);
        // Uniforms
        qsizetype uniformsCount = 0;
        in >> uniformsCount;
        uniformTable.clear();
        for (qsizetype i = 0; i < uniformsCount; ++i) {
            CustomMaterial::Uniform uniform = { };
            in >> uniform;
            uniformTable.append(uniform);
        }
        // We have a new table, so update the table model
        if (m_uniformModel) {
            m_uniformModel->setModelData(&uniformTable);
            updateMaterialDescription();
        }
    } else {
        return false;
    }

    setUnsavedChanges(false);

    return true;
}

bool MaterialAdapter::exportQmlComponent(const QUrl &componentFile, const QString &vertName, const QString &fragName)
{
    QFileInfo fi(componentFile.path());
    auto filename = fi.fileName();
    if (filename.isEmpty())
        return false;

    // Some sanity checks
    // Ensure the component starts with an upper-case letter.
    const auto firstLetter = filename.at(0);
    if (!firstLetter.isLetter()) {
        qWarning() << "Component name needs to start with an upper-case letter!";
        return false;
    }

    // Assume this is what the user wanted and fix it now.
    if (firstLetter.isLower()) {
        qWarning() << "Component name needs to start with an upper-case letter!";
        filename[0] = firstLetter.toUpper();
    }

    static const auto saveShader = [](const QUrl &filePath, const QString &text) {
        auto saveFile = QFile(filePath.path());
        bool ret = false;
        if (saveFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&saveFile);
            out << text;
            ret = true;
        }

        return ret;
    };

    bool ret = false;
    auto dirPath = fi.dir().path();
    if (!dirPath.isEmpty()) {
        if (m_materialDescr.isValid()) {
            m_materialDescr.shaders = { QUrl::fromLocalFile(dirPath + QDir::separator() + vertName + ".vert"), QUrl::fromLocalFile(dirPath + QDir::separator() + fragName + ".frag") };
            const auto &shaders = m_materialDescr.shaders;
            if (saveShader(shaders.vert, m_vertexShader) && saveShader(shaders.frag, m_fragmentShader)) {
                updateMaterialDescription();
                auto saveFile = QFile(dirPath + QDir::separator() + filename);
                if (saveFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                    QTextStream out(&saveFile);
                    out << m_materialDescr;
                }
            } else {
                emit errorOccurred();
                ret = false;
            }
        }
    }

    return ret;
}

void MaterialAdapter::reset()
{
    m_properties = CustomMaterial::Properties{};

    if (m_material == nullptr)
        return;

    delete m_material;

    uniformTable = {};

    if (m_uniformModel)
        m_uniformModel->setModelData(&uniformTable);

    // Set Default Shader Templates
    setFragmentShader(QString());
    setVertexShader(QString());

    updateMaterialDescription();
}

UniformModel *MaterialAdapter::uniformModel() const
{
    return m_uniformModel;
}

bool MaterialAdapter::unsavedChanges() const
{
    return m_unsavedChanges;
}

void MaterialAdapter::setUnsavedChanges(bool newUnsavedChanges)
{
    if (m_unsavedChanges == newUnsavedChanges)
        return;
    m_unsavedChanges = newUnsavedChanges;
    emit unsavedChangesChanged();
}

const QUrl &MaterialAdapter::materialSaveFile() const
{
    return m_materialSaveFile;
}

void MaterialAdapter::setMaterialSaveFile(const QUrl &newMaterialSaveFile)
{
    if (m_materialSaveFile == newMaterialSaveFile)
        return;
    m_materialSaveFile = newMaterialSaveFile;
    emit materialSaveFileChanged();
}

QQuick3DObject *MaterialAdapter::resourceRoot() const
{
    return m_resourceRoot;
}

void MaterialAdapter::setResourceRoot(QQuick3DObject *newResourceNode)
{
    if (m_resourceRoot == newResourceNode)
        return;
    m_resourceRoot = newResourceNode;
    emit resourceRootChanged();

    updateMaterialDescription();
}

MaterialAdapter::CullMode MaterialAdapter::cullMode() const
{
    return m_properties.cullMode;
}

void MaterialAdapter::setCullMode(CullMode newCullMode)
{
    if (m_properties.cullMode == newCullMode)
        return;
    m_properties.cullMode = newCullMode;
    emit cullModeChanged();

    updateMaterialDescription();
}

MaterialAdapter::DepthDrawMode MaterialAdapter::depthDrawMode() const
{
    return m_properties.depthDrawMode;
}

void MaterialAdapter::setDepthDrawMode(DepthDrawMode newDepthDrawMode)
{
    if (m_properties.depthDrawMode == newDepthDrawMode)
        return;
    m_properties.depthDrawMode = newDepthDrawMode;
    emit depthDrawModeChanged();

    updateMaterialDescription();
}

MaterialAdapter::ShadingMode MaterialAdapter::shadingMode() const
{
    return m_properties.shadingMode;
}

void MaterialAdapter::setShadingMode(ShadingMode newShadingMode)
{
    if (m_properties.shadingMode == newShadingMode)
        return;
    m_properties.shadingMode = newShadingMode;
    emit shadingModeChanged();

    updateMaterialDescription();
}

MaterialAdapter::BlendMode MaterialAdapter::srcBlend() const
{
    return m_properties.sourceBlend;
}

void MaterialAdapter::setSrcBlend(BlendMode newSourceBlend)
{
    if (m_properties.sourceBlend == newSourceBlend)
        return;
    m_properties.sourceBlend = newSourceBlend;
    emit srcBlendChanged();

    updateMaterialDescription();
}

MaterialAdapter::BlendMode MaterialAdapter::dstBlend() const
{
    return m_properties.destinationBlend;
}

void MaterialAdapter::setDstBlend(BlendMode newDestinationBlend)
{
    if (m_properties.destinationBlend == newDestinationBlend)
        return;
    m_properties.destinationBlend = newDestinationBlend;
    emit dstBlendChanged();

    updateMaterialDescription();
}

void MaterialAdapter::setUniformModel(UniformModel *newUniformModel)
{
    m_uniformModel = newUniformModel;
    if (m_uniformModel) {
        m_uniformModel->setModelData(&uniformTable);
        connect(m_uniformModel, &UniformModel::dataChanged, [this]() {
            updateMaterialDescription();
        });
    }
}

QString MaterialAdapter::getSupportedImageFormatsFilter() const
{
    auto formats = QImageReader::supportedImageFormats();
    QString imageFilter = QStringLiteral("Image files (");
    for (const auto &format : qAsConst(formats))
        imageFilter += QStringLiteral("*.") + format + QStringLiteral(" ");
    imageFilter += QStringLiteral(")");
    return imageFilter;
}


QT_END_NAMESPACE
