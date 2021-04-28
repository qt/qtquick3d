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

#include "qquick3druntimeloader_p.h"

#include <QtQuick3DAssetUtils/private/qssgscenedesc_p.h>
#include <QtQuick3DAssetUtils/private/qssgqmlutilities_p.h>
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>

/*!
    \qmltype RuntimeLoader
    \inherits Node
    \inqmlmodule QtQuick3D.AssetUtils
    \since 6.2
    \brief Imports a 3D asset at runtime.

    The RuntimeLoader type provides a way to load a 3D asset directly from source at runtime,
    without converting it to QtQuick3D's internal format first.

    Qt 6.2 supports the loading of glTF version 2.0 files in both in text (.gltf) and binary (.glb) formats.
*/

/*!
    \qmlproperty url QtQuick3D::RuntimeLoader::source

    This property holds the location of the source file containing the 3D asset.
    Changing this property will unload the current asset and attempt to load an asset from
    the given URL.

    The success or failure of the load operation is indicated by \l status.
*/

/*!
    \qmlproperty enumeration QtQuick3D::RuntimeLoader::status

    This read-only property holds the status of the latest load operation.

    \value RuntimeLoader.Empty
        No URL was specified.
    \value RuntimeLoader.Success
        The load operation was successful.
    \value RuntimeLoader.Error
        The load operation failed. A human-readable error message is provided by \l errorString.
*/

/*!
    \qmlproperty string QtQuick3D::RuntimeLoader::errorString

    This read-only property holds a human-readable string indicating the status of the latest load operation.
*/

QQuick3DRuntimeLoader::QQuick3DRuntimeLoader()
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
    m_source = newSource;
    emit sourceChanged();

    if (isComponentComplete())
        loadSource();
}

void QQuick3DRuntimeLoader::componentComplete()
{
    QQuick3DNode::componentComplete();
    loadSource();
}

void QQuick3DRuntimeLoader::loadSource()
{
    delete m_imported;
    m_imported.clear();

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

    emit statusChanged();
    emit errorStringChanged();

    if (m_status != Status::Success) {
        m_source.clear();
        emit sourceChanged();
        return;
    }

    m_imported = importManager.importScene(*this, scene);
}

QQuick3DRuntimeLoader::Status QQuick3DRuntimeLoader::status() const
{
    return m_status;
}

QString QQuick3DRuntimeLoader::errorString() const
{
    return m_errorString;
}
