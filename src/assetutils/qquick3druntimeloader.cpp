// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qquick3druntimeloader_p.h"

#include <QtQuick3DAssetUtils/private/qssgscenedesc_p.h>
#include <QtQuick3DAssetUtils/private/qssgqmlutilities_p.h>
#include <QtQuick3DAssetUtils/private/qssgrtutilities_p.h>
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderbuffermanager_p.h>
#if QT_CONFIG(mimetype)
#include <QtCore/qmimedatabase.h>
#endif

/*!
    \qmltype RuntimeLoader
    \inherits Node
    \inqmlmodule QtQuick3D.AssetUtils
    \since 6.2
    \brief Imports a 3D asset at runtime.

    The RuntimeLoader type provides a way to load a 3D asset directly from source at runtime,
    without converting it to QtQuick3D's internal format first.

    RuntimeLoader supports .obj and glTF version 2.0 files in both in text (.gltf) and binary
    (.glb) formats.

    \warning RuntimeLoader does not sandbox or validate asset contents. Loading
    malformed or untrusted assets may have security implications. See \l source
    for details.
*/

/*!
    \qmlproperty url RuntimeLoader::source

    This property holds the location of the source file containing the 3D asset.
    Changing this property will unload the current asset and attempt to load an asset from
    the given URL.

    The success or failure of the load operation is indicated by \l status.

    \warning RuntimeLoader does not sandbox or validate asset contents. Loading
    malformed or untrusted assets may have security implications. Application
    developers should carefully consider these before allowing the loading of
    user-provided content that is not part of the application.
*/

/*!
    \qmlproperty enumeration RuntimeLoader::status

    This property holds the status of the latest load operation.

    \value RuntimeLoader.Empty
        No URL was specified.
    \value RuntimeLoader.Success
        The load operation was successful.
    \value RuntimeLoader.Error
        The load operation failed. A human-readable error message is provided by \l errorString.

    \readonly
*/

/*!
    \qmlproperty string RuntimeLoader::errorString

    This property holds a human-readable string indicating the status of the latest load operation.

    \readonly
*/

/*!
    \qmlproperty Bounds RuntimeLoader::bounds

    This property describes the extents of the bounding volume around the imported model.

    \note The value may not be available before the first render

    \readonly
*/

/*!
    \qmlproperty Instancing RuntimeLoader::instancing

    If this property is set, the imported model will not be rendered normally. Instead, a number of
    instances will be rendered, as defined by the instance table.

    See the \l{Instanced Rendering} overview documentation for more information.
*/

QT_BEGIN_NAMESPACE

QQuick3DRuntimeLoader::QQuick3DRuntimeLoader(QQuick3DNode *parent)
    : QQuick3DNode(parent)
{

}

QUrl QQuick3DRuntimeLoader::source() const
{
    return m_source;
}

void QQuick3DRuntimeLoader::setSource(const QUrl &newSource)
{
    if (m_source == newSource)
        return;

    const QQmlContext *context = qmlContext(this);
    auto resolvedUrl = (context ? context->resolvedUrl(newSource) : newSource);

    if (m_source == resolvedUrl)
        return;

    m_source = resolvedUrl;
    emit sourceChanged();

    if (isComponentComplete())
        loadSource();
}

void QQuick3DRuntimeLoader::componentComplete()
{
    QQuick3DNode::componentComplete();
    loadSource();
}

QStringList QQuick3DRuntimeLoader::supportedExtensions()
{
    static QStringList extensions;
    if (!extensions.isEmpty())
        return extensions;

    static const QStringList supportedExtensions = { QLatin1StringView("obj"),
                                                     QLatin1StringView("gltf"),
                                                     QLatin1StringView("glb")};

    QSSGAssetImportManager importManager;
    const auto types = importManager.getImporterPluginInfos();

    for (const auto &t : types) {
        for (const QString &extension : t.inputExtensions) {
            if (supportedExtensions.contains(extension))
                extensions << extension;
        }
    }
    return extensions;
}

#if QT_CONFIG(mimetype)
QList<QMimeType> QQuick3DRuntimeLoader::supportedMimeTypes()
{
    static QList<QMimeType> mimeTypes;
    if (!mimeTypes.isEmpty())
        return mimeTypes;

    const QStringList &extensions = supportedExtensions();

    QMimeDatabase db;
    for (const auto &ext : extensions) {
        // TODO: Change to db.mimeTypesForExtension(ext), once it is implemented (QTBUG-118566)
        const QString fileName = QLatin1StringView("test.") + ext;
        mimeTypes << db.mimeTypesForFileName(fileName);
    }

    return mimeTypes;
}
#endif

static void boxBoundsRecursive(const QQuick3DNode *baseNode, const QQuick3DNode *node, QQuick3DBounds3 &accBounds)
{
    if (!node)
        return;

    if (auto *model = qobject_cast<const QQuick3DModel *>(node)) {
        auto b = model->bounds();
        for (const QVector3D point : b.bounds.toQSSGBoxPoints()) {
            auto p = model->mapPositionToNode(const_cast<QQuick3DNode *>(baseNode), point);
            if (Q_UNLIKELY(accBounds.bounds.isEmpty()))
                accBounds.bounds = { p, p };
            else
                accBounds.bounds.include(p);
        }
    }
    const auto childItems1 = node->childItems();
    for (auto *child : childItems1)
        boxBoundsRecursive(baseNode, qobject_cast<const QQuick3DNode *>(child), accBounds);
}

template<typename Func>
static void applyToModels(QQuick3DObject *obj, Func &&lambda)
{
    if (!obj)
        return;
    const auto childItems2 = obj->childItems();
    for (auto *child : childItems2) {
        if (auto *model = qobject_cast<QQuick3DModel *>(child))
            lambda(model);
        applyToModels(child, lambda);
    }
}

void QQuick3DRuntimeLoader::loadSource()
{
    delete m_root;
    m_objects.clear();
    m_objectsByType.clear();
    QSSGBufferManager::unregisterMeshData(m_assetId);

    m_status = Status::Empty;
    m_errorString = QStringLiteral("No file selected");
    if (!m_source.isValid()) {
        emit statusChanged();
        emit errorStringChanged();
        return;
    }

    QSSGAssetImportManager importManager;
    QSSGSceneDesc::Scene scene;
    QString error(QStringLiteral("Unknown error"));
    auto result = importManager.importFile(m_source, scene, &error);

    switch (result) {
    case QSSGAssetImportManager::ImportState::Success:
        m_errorString = QStringLiteral("Success!");
        m_status = Status::Success;
        break;
    case QSSGAssetImportManager::ImportState::IoError:
        m_errorString = QStringLiteral("IO Error: ") + error;
        m_status = Status::Error;
        break;
    case QSSGAssetImportManager::ImportState::Unsupported:
        m_errorString = QStringLiteral("Unsupported: ") + error;
        m_status = Status::Error;
        break;
    }

    if (m_status == Status::Success) {
        // We create a dummy root node here, as it will be the parent to the first-level nodes
        // and resources. If we use 'this' those first-level nodes/resources won't be deleted
        // when a new scene is loaded.
        m_root = new QQuick3DNode(this);
        m_root->setObjectName("RuntimeLoaderRoot");
        m_imported = QSSGRuntimeUtils::createScene(*m_root, scene, &m_objects, &m_objectsByType);
        m_assetId = scene.id;
        m_boundsDirty = true;
        m_instancingChanged = m_instancing != nullptr;
        updateModels();
        // Cleanup scene before deleting.
        scene.cleanup();
    } else {
        m_source.clear();
        emit sourceChanged();
    }

    emit statusChanged();
    emit errorStringChanged();

}

void QQuick3DRuntimeLoader::updateModels()
{
    if (m_instancingChanged) {
        applyToModels(m_imported, [this](QQuick3DModel *model) {
            model->setInstancing(m_instancing);
            model->setInstanceRoot(m_imported);
        });
        m_instancingChanged = false;
    }
}

QQuick3DRuntimeLoader::Status QQuick3DRuntimeLoader::status() const
{
    return m_status;
}

QString QQuick3DRuntimeLoader::errorString() const
{
    return m_errorString;
}

QSSGRenderGraphObject *QQuick3DRuntimeLoader::updateSpatialNode(QSSGRenderGraphObject *node)
{
    auto *result = QQuick3DNode::updateSpatialNode(node);
    if (m_boundsDirty)
        QMetaObject::invokeMethod(this, &QQuick3DRuntimeLoader::boundsChanged, Qt::QueuedConnection);
    return result;
}

void QQuick3DRuntimeLoader::calculateBounds()
{
    if (!m_imported || !m_boundsDirty)
        return;

    m_bounds.bounds.setEmpty();
    boxBoundsRecursive(m_imported, m_imported, m_bounds);
    m_boundsDirty = false;
}

const QQuick3DBounds3 &QQuick3DRuntimeLoader::bounds() const
{
    if (m_boundsDirty) {
        auto *that = const_cast<QQuick3DRuntimeLoader *>(this);
        that->calculateBounds();
        return that->m_bounds;
    }

    return m_bounds;
}

QQuick3DInstancing *QQuick3DRuntimeLoader::instancing() const
{
    return m_instancing;
}

void QQuick3DRuntimeLoader::setInstancing(QQuick3DInstancing *newInstancing)
{
    if (m_instancing == newInstancing)
        return;

    QQuick3DObjectPrivate::attachWatcher(this, &QQuick3DRuntimeLoader::setInstancing,
                                         newInstancing, m_instancing);

    m_instancing = newInstancing;
    m_instancingChanged = true;
    updateModels();
    emit instancingChanged();
}

/*!
    \qmlmethod Object3D RuntimeLoader::query(string arg)
    \since 6.12

    Returns the object with the given name, or \c null if no object with that name exists.

    The \a arg parameter is the name of the object to query or a query string.
    For example, to query the object named "PaintMaterialX", use the following code:

    \badcode
    var object = runtimeLoader.query("PaintMaterial")
    \endcode

    The above code works as expected assuming the objects are sensibly named in the source asset file.
    However, if there are multiple objects with the same name, the query will return the first object
    found matching the given name. To query a specific object, and avoid ambiguity, use the full object path.

    \badcode
    var object = runtimeLoader.query("/House2/Wall001/Mirror/Material")
    \endcode

    \note Even with paths it's possible for a improperly structured asset file to have multiple objects with the same path,
    as the paths are built up from the object names in the source asset file.

    \note The object names are defined as in the source asset file.
*/

QQuick3DObject *QQuick3DRuntimeLoader::query(const QString &name) const
{
    const auto index = name.lastIndexOf(QChar(u'/'));

    if (index != -1) {
        const QString shortName = name.mid(index + 1);
        const auto range = m_objects.equal_range({shortName, QString()});
        for (auto it = range.first; it != range.second; ++it) {
            if (it.key().path == name)
                return it.value();
        }
    }

    return m_objects.value(QSSGRuntimeObjectNameKey{name, QString()});
}

static inline QSSGRenderGraphObject::BaseType queryFilterToBaseType(QQuick3DRuntimeLoader::QueryFilter filter)
{
    using Type = QSSGRenderGraphObject::Type;
    switch (filter) {
    case QQuick3DRuntimeLoader::QueryFilter::Textures:
        return QSSGRenderGraphObjectUtils::getBaseType(Type::Image2D);
    case QQuick3DRuntimeLoader::QueryFilter::Materials:
        return QSSGRenderGraphObjectUtils::getBaseType(Type::PrincipledMaterial);
    case QQuick3DRuntimeLoader::QueryFilter::Nodes:
        return QSSGRenderGraphObjectUtils::getBaseType(Type::Node);
    case QQuick3DRuntimeLoader::QueryFilter::Cameras:
        return QSSGRenderGraphObjectUtils::getBaseType(Type::OrthographicCamera);
    case QQuick3DRuntimeLoader::QueryFilter::Lights:
        return QSSGRenderGraphObjectUtils::getBaseType(Type::DirectionalLight);
    case QQuick3DRuntimeLoader::QueryFilter::Models:
        return QSSGRenderGraphObjectUtils::getBaseType(Type::Model);
    }

    Q_UNREACHABLE_RETURN(QSSGRenderGraphObject::BaseType(0));
}

/*!
    \qmlmethod List<Object3D> RuntimeLoader::queryAll(QueryFilter filter)
    \since 6.12

    Returns a list of all objects matching the given filter.

    The \a filter parameter specifies the type of objects to query for.
    For example, to query for all materials, use the following code:

    \badcode
    var materials = runtimeLoader.queryAll(RuntimeLoader.Materials)
    \endcode

    The above code returns a list of all materials in the source asset file.
*/

QList<QQuick3DObject *> QQuick3DRuntimeLoader::queryAll(QueryFilter filter) const
{
    QList<QQuick3DObject *> results;
    const auto range = m_objectsByType.equal_range(queryFilterToBaseType(filter));
    for (auto it = range.first; it != range.second; ++it) {
        if (auto *obj = it.value().data())
            results << obj;
    }
    return results;
}

QT_END_NAMESPACE
