/****************************************************************************
**
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

#ifndef UIPIMPORTER_H
#define UIPIMPORTER_H

#include <QtQuick3DAssetImport/private/qssgassetimporter_p.h>

#include <QtCore/QTextStream>

#include "uipparser.h"
#include "uiaparser.h"

QT_BEGIN_NAMESPACE

class UipPresentation;
class ReferencedMaterial;
class ComponentNode;
class AliasNode;
class UipImporter : public QSSGAssetImporter
{
public:
    UipImporter();

    const QString name() const override;
    const QStringList inputExtensions() const override;
    const QString outputExtension() const override;
    const QString type() const override;
    const QString typeDescription() const override;
    const QVariantMap importOptions() const override;
    const QString import(const QString &sourceFile, const QDir &savePath, const QVariantMap &options, QStringList *generatedFiles) override;

private:
    QString processUipPresentation(UipPresentation *presentation, const QString &ouputFilePath);
    void processNode(GraphObject *object, QTextStream &output, int tabLevel, bool isInRootLevel = false, bool processSiblings = true);
    void checkForResourceFiles(GraphObject *object);
    void generateMaterialComponent(GraphObject *object);
    void generateAliasComponent(GraphObject *reference);
    void generateAnimationTimeLine(QTextStream &output, int tabLevel, UipPresentation *presentation = nullptr, ComponentNode *component = nullptr);
    void generateStatesFromSlides(Slide *masterSlide, QTextStream &output, int tabLevel);
    void generateComponent(GraphObject *component);
    void writeHeader(QTextStream &output, bool isRootLevel = false);
    void generateApplicationComponent(const QString &initialPresentationComponent, const QSize &size);
    void generateQmlComponent(const QString componentName, const QString componentSource);
    void processOptions(const QVariantMap &options);
    bool checkBooleanOption(const QString &optionName, const QJsonObject &options);
    double getRealOption(const QString &optionName, const QJsonObject &options);

    QVector<QString> m_resourcesList;
    UiaParser m_uiaParser;
    UipParser m_uipParser;
    UipPresentation *m_presentation;

    QString m_sourceFile;
    QDir m_exportPath;
    QVariantMap m_options;
    QStringList m_generatedFiles;

    // per presentation
    QVector <ReferencedMaterial *> m_referencedMaterials;
    QVector <AliasNode *> m_aliasNodes;
    QVector <ComponentNode *> m_componentNodes;
    QVector<QDir> m_qmlDirs;
    bool m_hasQMLSubPresentations = false;

    // options
    bool m_createProjectWrapper = false;
    bool m_createIndividualLayers = false;
    float m_fps = 60.f;
};

QT_END_NAMESPACE

#endif // UIPIMPORTER_H
