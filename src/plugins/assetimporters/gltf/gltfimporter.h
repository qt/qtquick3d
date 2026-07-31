// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include <QtQuick3DAssetImport/private/qssgassetimporter_p.h>

#include <QtCore/QJsonObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

class GltfImporter : public QSSGAssetImporter
{
public:
    GltfImporter();

    QString name() const override;
    QStringList inputExtensions() const override;
    QString outputExtension() const override;
    QString type() const override;
    QString typeDescription() const override;
    int priority() const override;
    QJsonObject importOptions() const override;
    QString import(const QString &sourceFile, const QDir &savePath, const QJsonObject &options,
                   QStringList *generatedFiles) override;
    QString import(const QUrl &sourceFile, const QJsonObject &options, QSSGSceneDesc::Scene &scene) override;

private:
    QJsonObject m_options;
};

QT_END_NAMESPACE
