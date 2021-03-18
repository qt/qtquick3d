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

#include "uipimporter.h"
#include "uipparser.h"
#include "uiaparser.h"
#include "uippresentation.h"
#include "datamodelparser.h"
#include "keyframegroupgenerator.h"
#include "uniqueidmapper.h"
#include <QtQuick3DAssetImport/private/qssgqmlutilities_p.h>

#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>

QT_BEGIN_NAMESPACE

UipImporter::UipImporter()
{
    QFile optionFile(":/uipimporter/options.json");
    optionFile.open(QIODevice::ReadOnly);
    QByteArray options = optionFile.readAll();
    optionFile.close();
    auto optionsDocument = QJsonDocument::fromJson(options);
    m_options = optionsDocument.object().toVariantMap();
}

const QString UipImporter::name() const
{
    return QStringLiteral("uip");
}

const QStringList UipImporter::inputExtensions() const
{
    QStringList extensions;
    extensions.append(QStringLiteral("uia"));
    extensions.append(QStringLiteral("uip"));
    return extensions;
}

const QString UipImporter::outputExtension() const
{
    return QStringLiteral(".qml");
}

const QString UipImporter::type() const
{
    return QStringLiteral("Scene");
}

const QString UipImporter::typeDescription() const
{
    return QObject::tr("Qt 3D Studio Presentation");
}

const QVariantMap UipImporter::importOptions() const
{
    return m_options;
}

namespace  {
bool copyRecursively(const QString &sourceFolder, const QString &destFolder)
{
    bool success = false;
    QDir sourceDir(sourceFolder);

    if(!sourceDir.exists())
        return false;

    QDir destDir(destFolder);
    if(!destDir.exists())
        destDir.mkdir(destFolder);

    auto files = sourceDir.entryList(QDir::Files);
    for (const auto &file : files) {
        QString srcName = sourceFolder + QDir::separator() + file;
        QString destName = destFolder + QDir::separator() + file;
        success = QFile::copy(srcName, destName);
        if(!success)
            return false;
    }

    files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const auto &file : files) {
        QString srcName = sourceFolder + QDir::separator() + file;
        QString destName = destFolder + QDir::separator() + file;
        success = copyRecursively(srcName, destName);
        if(!success)
            return false;
    }

    return true;
}
}

const QString UipImporter::import(const QString &sourceFile, const QDir &savePath,
                                  const QVariantMap &options, QStringList *generatedFiles)
{
    // Reset UniqueIdMapper cache so different imports do not affect each other's ids
    UniqueIdMapper::instance()->reset();

    m_sourceFile = sourceFile;
    m_exportPath = savePath;
    m_options = options;

    processOptions(options);

    // Verify sourceFile and savePath
    QFileInfo source(sourceFile);
    if (!source.exists())
        return QStringLiteral("Source File: %s does not exist").arg(sourceFile);
    if (!savePath.exists())
        return QStringLiteral("Export Directory Invalid");

    QString uiaComponentName;
    QSize uiaComponentSize;

    // If sourceFile is a UIA file
    if (sourceFile.endsWith(QStringLiteral(".uia"), Qt::CaseInsensitive)) {
        auto uia = m_uiaParser.parse(sourceFile);
        uiaComponentName = uia.initialPresentationId;
        for (auto presentation : uia.presentations) {
            if (presentation.type == UiaParser::Uia::Presentation::Qml) {
                m_hasQMLSubPresentations = true;
                QFileInfo qmlFile(source.absolutePath() + QDir::separator() + presentation.source);
                if (!m_qmlDirs.contains(qmlFile.dir()))
                    m_qmlDirs.append(qmlFile.dir());
                generateQmlComponent(presentation.id, qmlFile.baseName());
            }
        }

        for (auto presentation : uia.presentations) {
            if (presentation.type == UiaParser::Uia::Presentation::Uip) {
                // UIP
                auto uip = m_uipParser.parse(source.absolutePath() + QDir::separator()
                                             + presentation.source, presentation.id);
                processUipPresentation(uip, savePath.absolutePath() + QDir::separator());
                if (presentation.id == uiaComponentName)
                    uiaComponentSize = QSize(uip->presentationWidth(), uip->presentationHeight());
            }
        }
        if (m_hasQMLSubPresentations) {
            // If there is any QML in the project at all, we have to copy the entire
            // qml folder over
            for (QDir dir : m_qmlDirs)
                copyRecursively(dir.absolutePath(), m_exportPath.absolutePath() + QDir::separator() + QStringLiteral("qml"));
        }

    } else if (sourceFile.endsWith(QStringLiteral(".uip"), Qt::CaseInsensitive)) {
        auto uip = m_uipParser.parse(sourceFile, QString());
        processUipPresentation(uip, savePath.absolutePath() + QDir::separator());
    }

    QString errorString;

    // Copy any resource files to export directory
    for (auto file : m_resourcesList) {
        QFileInfo sourceFile(source.absolutePath() + QDir::separator() + file);
        if (!sourceFile.exists()) {
            // Try again after stripping the parent directory
            sourceFile = QFileInfo(source.absolutePath() + QDir::separator() + QSSGQmlUtilities::stripParentDirectory(file));
            if (!sourceFile.exists()) {
                errorString += QStringLiteral("Resource file does not exist: ") + sourceFile.absoluteFilePath() + QChar('\n');
                continue;
            }
        }
        QFileInfo destFile(savePath.absoluteFilePath(QSSGQmlUtilities::stripParentDirectory(file)));
        QDir destDir(destFile.absolutePath());
        destDir.mkpath(".");

        if (QFile::copy(sourceFile.absoluteFilePath(), destFile.absoluteFilePath()))
            m_generatedFiles += destFile.absoluteFilePath();
    }

    // Generate UIA Component if we converted a uia
    if (m_createProjectWrapper && !uiaComponentName.isEmpty())
        generateApplicationComponent(QSSGQmlUtilities::qmlComponentName(uiaComponentName), uiaComponentSize);

    if (generatedFiles)
        generatedFiles = &m_generatedFiles;

    return errorString;
}

void UipImporter::processNode(GraphObject *object, QTextStream &output, int tabLevel, bool isInRootLevel, bool processSiblings)
{
    GraphObject *obj = object;
    while (obj) {
        if (obj->type() == GraphObject::Scene) {
            // Ignore Scene for now
            processNode(obj->firstChild(), output, tabLevel, isInRootLevel);
        } else if ( obj->type() == GraphObject::DefaultMaterial &&
                    obj->qmlId() == QStringLiteral("__Container")) {
            // UIP version > 5 which tries to be clever with reference materials
            // Instead of parsing these items as normal, instead we iterate the
            // materials container and generate new Components for each one and output them
            // to the "materials" folder
            GraphObject *materialObject = obj->firstChild();
            while(materialObject) {
                generateMaterialComponent(materialObject);
                materialObject = materialObject->nextSibling();
            }
        } else {
            // Output QML
            output << Qt::endl;
            obj->writeQmlHeader(output, tabLevel);
            obj->writeQmlProperties(output, tabLevel + 1, isInRootLevel);

            if (obj->type() != GraphObject::Component && obj->type() != GraphObject::Layer)
                processNode(obj->firstChild(), output, tabLevel + 1, isInRootLevel);

            if (obj->type() == GraphObject::Layer) {
//                // effects array
//                // get all children that are effects, and add their id's to effects: array
//                QString effects;
//                GraphObject *effectObject = obj->firstChild();
//                while (effectObject) {
//                    if (effectObject->type() == GraphObject::Effect)
//                        effects += effectObject->qmlId() + QStringLiteral(", ");
//                    effectObject = effectObject->nextSibling();
//                }
//                if (!effects.isEmpty()) {
//                    // remove final ", "
//                    effects.chop(2);
//                    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("effects: [") << effects << "]\n";
//                }


                // Check if layer has a sub-presentation set, in which case we can skip children processing and timeline generation
                auto layer = static_cast<LayerNode*>(obj);
                if (layer->m_sourcePath.isEmpty()) {
                    // Process children nodes
                    processNode(obj->firstChild(), output, tabLevel + 1, isInRootLevel);
//                    // Generate Animation Timeline
//                    generateAnimationTimeLine(obj, m_presentation->masterSlide(), output, tabLevel + 1);
//                    // Generate States from Slides
//                    generateStatesFromSlides(obj, m_presentation->masterSlide(), output, tabLevel + 1);
                }


            } else if (obj->type() == GraphObject::Model) {
                // materials array
                // get all children that are materials, and add their id's to materials: array
                QString materials;
                GraphObject *materialObject = obj->firstChild();
                while (materialObject) {
                    if (materialObject->type() == GraphObject::DefaultMaterial ||
                        materialObject->type() == GraphObject::CustomMaterial ||
                        materialObject->type() == GraphObject::ReferencedMaterial)
                        materials += materialObject->qmlId() + QStringLiteral(", ");
                    materialObject = materialObject->nextSibling();
                }
                if (!materials.isEmpty()) {
                    // remove final ", "
                    materials.chop(2);
                    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << "materials: [" << materials << "]\n";
                }
            } else if (obj->type() == GraphObject::ReferencedMaterial) {
                m_referencedMaterials.append(static_cast<ReferencedMaterial *>(obj));
            } else if (obj->type() == GraphObject::Alias) {
                m_aliasNodes.append(static_cast<AliasNode*>(obj));
            } else if (obj->type() == GraphObject::Component) {
                m_componentNodes.append(static_cast<ComponentNode*>(obj));
            }

            checkForResourceFiles(obj);

            obj->writeQmlFooter(output, tabLevel);
        }
        if (processSiblings)
            obj = obj->nextSibling();
        else
            break;
    }
}

void UipImporter::checkForResourceFiles(GraphObject *object)
{
    if (!object)
        return;
    if (object->type() == GraphObject::Image) {
        Image *image = static_cast<Image*>(object);
        if (image->m_subPresentation.isEmpty() && !m_resourcesList.contains(image->m_sourcePath))
            m_resourcesList.append(image->m_sourcePath);
    } else if (object->type() == GraphObject::Model) {
        ModelNode *model = static_cast<ModelNode*>(object);
        QString meshLocation = model->m_mesh_unresolved;
        // Remove trailing # directive
        int hashLocation = meshLocation.indexOf("#");
        // if mesh source starts with #, it's a primitive, so skip
        if (hashLocation == 1)
            return;
        if (hashLocation != -1) {
            meshLocation.chop(meshLocation.length() - hashLocation);
        }
        if (!m_resourcesList.contains(meshLocation))
            m_resourcesList.append(meshLocation);
    } else if (object->type() == GraphObject::Effect) {
        // ### maybe remove #
        //EffectInstance *effect = static_cast<EffectInstance*>(object);
        //if (!m_resourcesList.contains(effect->m_effect_unresolved))
        //    m_resourcesList.append(effect->m_effect_unresolved);
    } else if (object->type() == GraphObject::CustomMaterial) {
        // ### maybe remove #
        // CustomMaterialInstance *material = static_cast<CustomMaterialInstance*>(object);
        //if (!m_resourcesList.contains(material->m_material_unresolved))
        //    m_resourcesList.append(material->m_material_unresolved);
    }
}

void UipImporter::generateMaterialComponent(GraphObject *object)
{
    QDir materialPath = m_exportPath.absolutePath() + QDir::separator() + QStringLiteral("materials");

    QString id = object->qmlId();
    if (id.startsWith("materials_"))
        id = id.remove(QStringLiteral("materials_"));
    // Default matterial has two //'s
    if (id.startsWith("_"))
        id.remove(0, 1);

    QString materialComponent = QSSGQmlUtilities::qmlComponentName(id);
    QString targetFile = materialPath.absolutePath() + QDir::separator() + materialComponent + QStringLiteral(".qml");
    QFile materialComponentFile(targetFile);

    if (m_generatedFiles.contains(targetFile)) {
        // if we already generated this material
        return;
    }

    if (!materialComponentFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not write to file : " << materialComponentFile;
        return;
    }

    QTextStream output(&materialComponentFile);
    output << "import QtQuick 2.15\n";
    output << "import QtQuick3D 1.15\n";
    if (object->type() == GraphObject::ReferencedMaterial)
        output << "import \"./\"\n";
    processNode(object, output, 0, false, false);

    materialComponentFile.close();
    m_generatedFiles += targetFile;
}

void UipImporter::generateAliasComponent(GraphObject *reference)
{
    // create materials folder
    QDir aliasPath = m_exportPath.absolutePath() + QDir::separator() + QStringLiteral("aliases");

    QString aliasComponentName = QSSGQmlUtilities::qmlComponentName(reference->qmlId());
    QString targetFile = aliasPath.absolutePath() + QDir::separator() + aliasComponentName + QStringLiteral(".qml");
    QFile aliasComponentFile(targetFile);

    if (m_generatedFiles.contains(targetFile))
        return;

    if (!aliasComponentFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not write to file: " << aliasComponentFile;
        return;
    }

    QTextStream output(&aliasComponentFile);
    output << "import QtQuick 2.15\n";
    output << "import QtQuick3D 1.15\n";
    processNode(reference, output, 0, false, false);

    aliasComponentFile.close();
    m_generatedFiles += targetFile;
}

namespace {

#if 0
QSet<GraphObject*> getSubtreeItems(GraphObject *node)
{
    QSet<GraphObject *> items;
    if (!node)
        return items;

    std::function<void(GraphObject *, QSet<GraphObject *> &)> treeWalker;
    treeWalker = [&treeWalker](GraphObject *obj, QSet<GraphObject *> &items) {
        while (obj) {
            items.insert(obj);
            treeWalker(obj->firstChild(), items);
            obj = obj->nextSibling();
        }
    };

    treeWalker(node->firstChild(), items);

    return items;
}
#endif

void generateTimelineAnimation(Slide *slide, int startFrame, int endFrame, int duration,
                               bool isRunning, QTextStream &output, int tabLevel,
                               const QString &componentName)
{
    QString looping = QStringLiteral("1");
    QString pingPong = QStringLiteral("false");
    if (slide->m_playMode == Slide::Looping) {
        looping = QStringLiteral("-1");
    } else if (slide->m_playMode == Slide::PingPong) {
        looping = QStringLiteral("-1");
        pingPong = QStringLiteral("true");
    }

    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("TimelineAnimation {")
           << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("id: ")
           << QSSGQmlUtilities::sanitizeQmlId(slide->m_name + QStringLiteral("TimelineAnimation"))
           << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("duration: ")
           << duration << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("from: ")
           << startFrame << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("to: ")
           << endFrame << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("running: " )
           << (isRunning ? QStringLiteral("true") : QStringLiteral("false")) << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("loops: ")
           << looping << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("pingPong: ")
           << pingPong << Qt::endl;
    if (slide->m_playMode == Slide::PlayThroughTo) {
        // when the animation is done playing, change to the state defined by PlayThrough
        // onFinished: item1.state = "firsthalf"
        QString slideName;
        if (slide->m_playThrough == Slide::Next) {
            if (slide->nextSibling())
                slideName = slide->nextSibling()->m_name;
        } else if (slide->m_playThrough == Slide::Previous) {
            if (slide->nextSibling())
                slideName = slide->previousSibling()->m_name;
        } else {
            // value
            if (slide->m_playThroughValue.type() == QVariant::String) {
                slideName = slide->m_playThroughValue.toString();
            } else {
                int slideIndex = slide->m_playThroughValue.toInt();
                Slide *newSlide = static_cast<Slide*>(slide->parent()->childAtIndex(slideIndex));
                slideName = newSlide->m_name;
            }
        }
        if (!slideName.isEmpty()) {
            output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("onFinished: ")
                   << componentName << QStringLiteral(".state = \"") << slideName
                   << QStringLiteral("\"") << Qt::endl;
        }
    }
    output << QSSGQmlUtilities::insertTabs(tabLevel) << QStringLiteral("}");
}

QVector<AnimationTrack> combineAnimationTracks(const QVector<AnimationTrack> &master, const QVector<AnimationTrack> &slide) {
    // We can't have animations that target the same object and property,
    // so slides overwrite master animations
    QVector<AnimationTrack> animations;
    for (auto masterAnimation : master) {
        bool skip = false;
        for (auto slideAnimation : slide) {
            if (masterAnimation.m_target == slideAnimation.m_target &&
                    masterAnimation.m_property == slideAnimation.m_property) {
                skip = true;
                break;
            }
        }
        if (!skip)
            animations.append(masterAnimation);
    }
    animations.append(slide);
    return animations;
}

void calculateStartAndEndFrames(Slide *currentSlide, UipPresentation *presentation,
                                ComponentNode *component, float fps, int &startFrame,
                                int &endFrame, int &duration) {

    GraphObject *object = nullptr;
    if (presentation) {
        //presentation
        presentation->applySlidePropertyChanges(currentSlide);
        object = presentation->scene()->firstChild();
    } else {
        //component
        const auto &changeList = currentSlide->propertyChanges();
        for (auto it = changeList.cbegin(), ite = changeList.cend(); it != ite; ++it) {
            it.key()->applyPropertyChanges(*it.value());
        }
        object = component->firstChild();
    }

    float startTime = std::numeric_limits<float>::max();
    float endTime = std::numeric_limits<float>::min();
    while (object) {
        startTime = qMin(object->startTime(), startTime);
        endTime = qMax(object->endTime(), endTime);
        object = object->nextSibling();
    }

    startFrame = qRound(startTime * fps);
    endFrame = qRound(endTime * fps);
    duration = qRound((endTime - startTime) * 1000.f);
}

}

void UipImporter::generateAnimationTimeLine(QTextStream &output, int tabLevel, UipPresentation *presentation, ComponentNode *component)
{
    // Either presentation or component should be valid
    Slide *masterSlide;
    QString componentName;
    if (presentation) {
        masterSlide = presentation->masterSlide();
        componentName = presentation->name();
    } else if (component) {
        masterSlide = component->m_masterSlide;
        componentName = component->qmlId();
    } else {
        Q_UNREACHABLE();
        qDebug("something went wrong");
        return;
    }

    // Generate a 1 Timeline and 1 TimelineAnimation for each slide
    // Each combines the master slide and current slide

    // For each slide, create a TimelineAnimation
    auto slide = static_cast<Slide*>(masterSlide->firstChild());
    while (slide) {
        int startFrame = 0;
        int endFrame = 0;
        int duration = 0;
        calculateStartAndEndFrames(slide, presentation, component, m_fps,
                                   startFrame, endFrame, duration);
        output << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel) << "Timeline {\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("id: ")
               << QSSGQmlUtilities::sanitizeQmlId(slide->m_name + QStringLiteral("Timeline"))
               << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("startFrame: ")
               << startFrame << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("endFrame: ")
               << endFrame << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("currentFrame: ")
               << startFrame << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("enabled: false")
               << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << QStringLiteral("animations: [")
               << Qt::endl;
        generateTimelineAnimation(slide, startFrame, endFrame, duration, false, output,
                                  tabLevel + 2, componentName);
        output << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel + 1) << "]\n";

        // Keyframe groups for master + current slide
        // Get a list off all animations for the master and first slide
        const auto animations = combineAnimationTracks(masterSlide->animations(),
                                                       slide->animations());
        // Create a list of KeyframeGroups
        KeyframeGroupGenerator generator(m_fps);

        for (auto animation : animations)
            generator.addAnimation(animation);

        generator.generateKeyframeGroups(output, tabLevel + 1);

        output << QSSGQmlUtilities::insertTabs(tabLevel) << "}\n";
        slide = static_cast<Slide*>(slide->nextSibling());
    }
}

void UipImporter::generateStatesFromSlides(Slide *masterSlide, QTextStream &output, int tabLevel)
{
    output << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "states: [\n";

    auto slide = static_cast<Slide*>(masterSlide->firstChild());
    bool isFirst = true;
    QString firstStateName;
    while (slide) {
        if (isFirst) {
            isFirst = false;
            firstStateName = slide->m_name;
        } else {
            output << ",\n";
        }

        output << QSSGQmlUtilities::insertTabs(tabLevel+1) << "State {\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel+2) << "name: \"" << slide->m_name << "\"\n";

        // Add property changes here
        // First enable the Timeline for this state
        output << QSSGQmlUtilities::insertTabs(tabLevel+2) << "PropertyChanges {\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel+3) << QStringLiteral("target: ") << QSSGQmlUtilities::sanitizeQmlId(slide->m_name + QStringLiteral("Timeline")) << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel+3) << "enabled: true\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel+3) << "currentFrame: 0\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel+2) << "}\n";

        output << QSSGQmlUtilities::insertTabs(tabLevel+2) << "PropertyChanges {\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel+3) << QStringLiteral("target: ") << QSSGQmlUtilities::sanitizeQmlId(slide->m_name + QStringLiteral("TimelineAnimation")) << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(tabLevel+3) << "running: true\n";
        output << QSSGQmlUtilities::insertTabs(tabLevel+2) << "}\n";

        // Now all other properties changed by the slide
        auto changeList = slide->propertyChanges();
        for (auto it = changeList.cbegin(), ite = changeList.cend(); it != ite; ++it) {
            // First see if there are any property changes we actually care about
            QBuffer propertyChangesBuffer;
            propertyChangesBuffer.open(QBuffer::WriteOnly);
            QTextStream propertyChangesOutput(&propertyChangesBuffer);
            // output each of the properties changed
            it.key()->writeQmlProperties(*it.value(), propertyChangesOutput, tabLevel + 3);
            propertyChangesBuffer.close();

            if (!propertyChangesBuffer.data().isEmpty()) {

                output << QSSGQmlUtilities::insertTabs(tabLevel+2) << "PropertyChanges {\n";
                output << QSSGQmlUtilities::insertTabs(tabLevel+3) << QStringLiteral("target: ") << it.key()->qmlId() << Qt::endl;

                output << propertyChangesBuffer.data();

                output << QSSGQmlUtilities::insertTabs(tabLevel+2) << "}\n";
            }
        }

        output << QSSGQmlUtilities::insertTabs(tabLevel+1) << QStringLiteral("}");

        slide = static_cast<Slide*>(slide->nextSibling());
    }

    output << Qt::endl;

    output << QSSGQmlUtilities::insertTabs(tabLevel) << "]\n";

    // Set the initial state (works correctly even when empty)
    output << QSSGQmlUtilities::insertTabs(tabLevel) << "state: \""
        << firstStateName << "\"\n";
 }

void UipImporter::generateComponent(GraphObject *component)
{
    QDir componentPath = m_exportPath.absolutePath() + QDir::separator() + QStringLiteral("components");

    QString componentName = QSSGQmlUtilities::qmlComponentName(component->qmlId());
    QString targetFileName = componentPath.absolutePath() + QDir::separator() + componentName + QStringLiteral(".qml");
    QFile componentFile(targetFileName);
    if (!componentFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not write to file: " << componentFile;
        return;
    }

    QTextStream output(&componentFile);
    writeHeader(output);

    output << "Node {\n";
    component->writeQmlProperties(output, 1);

    processNode(component->firstChild(), output, 1, false);

    // Generate Animation Timeline
    auto componentNode = static_cast<ComponentNode*>(component);
    generateAnimationTimeLine(output, 1, nullptr, componentNode);

    // Generate States from Slides
    generateStatesFromSlides(componentNode->m_masterSlide, output, 1);

    // Footer
    component->writeQmlFooter(output, 0);

    componentFile.close();
    m_generatedFiles += targetFileName;
}

void UipImporter::writeHeader(QTextStream &output, bool isRootLevel)
{
    output << "import QtQuick 2.15\n";
    output << "import QtQuick3D 1.15\n";
    output << "import QtQuick.Timeline 1.0\n";

    QString relativePath = isRootLevel ? "./" : "../";

    if (m_referencedMaterials.count() > 0)
        output << "import \"" << relativePath << "materials\"\n";

    if (m_aliasNodes.count() > 0)
        output << "import \"" << relativePath << "aliases\"\n";

    if (m_componentNodes.count() > 0 || m_qmlDirs.count() > 0)
        output << "import \"" << relativePath << "components\"\n";

    output << Qt::endl;
}

void UipImporter::generateApplicationComponent(const QString &initialPresentationComponent, const QSize &size)
{
    // Create File
    QString targetFileName = m_exportPath.absolutePath() + QDir::separator() + initialPresentationComponent + QStringLiteral("Window.qml");
    QFile applicationComponentFile(targetFileName);
    if (!applicationComponentFile.open(QIODevice::WriteOnly)) {
        qDebug() << "couldn't open " << targetFileName << " for writing";
        return;
    }

    QTextStream output(&applicationComponentFile);

    // Header
    output << "import QtQuick 2.15\n";
    output << "import QtQuick.Window 2.15\n";
    output << Qt::endl;

    // Window
    output << "Window {\n";
    output << QSSGQmlUtilities::insertTabs(1) << "width: " << size.width() << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(1) << "height: " << size.height() << Qt::endl;
    output << QSSGQmlUtilities::insertTabs(1) << "title: " << "\"" << initialPresentationComponent << "\"\n";
    output << QSSGQmlUtilities::insertTabs(1) << "visible: true\n";
    output << Qt::endl;

    // Component
    output << QSSGQmlUtilities::insertTabs(1) << initialPresentationComponent << " {\n";
    output << QSSGQmlUtilities::insertTabs(2) << "anchors.fill: parent\n";
    output << QSSGQmlUtilities::insertTabs(1) << "}\n";

    output << "}\n";

    applicationComponentFile.close();
    m_generatedFiles += targetFileName;
}

void UipImporter::generateQmlComponent(const QString componentName, const QString componentSource)
{
    QDir componentPath = m_exportPath.absolutePath() + QDir::separator() + QStringLiteral("components");
    componentPath.mkdir(".");

    // Basically creates an alias component to a QML one
    QString qmlComponentName = QSSGQmlUtilities::qmlComponentName(componentName);
    QString targetFileName = componentPath.absolutePath() + QDir::separator() + qmlComponentName + QStringLiteral(".qml");
    QFile componentFile(targetFileName);
    if (!componentFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not write to file: " << componentFile;
        return;
    }

    QTextStream output(&componentFile);

    output << "import QtQuick 2.15\n";
    output << "import \"../qml\"\n" << Qt::endl;

    output << componentSource << QStringLiteral(" { }");

    componentFile.close();
    m_generatedFiles.append(targetFileName);
}

void UipImporter::processOptions(const QVariantMap &options)
{
    // Setup import settings based given options
    // You can either pass the whole options object, or just the "options" object
    // so get the right scope.
    QJsonObject optionsObject = QJsonObject::fromVariantMap(options);
    if (optionsObject.contains(QStringLiteral("options")))
        optionsObject = optionsObject.value(QStringLiteral("options")).toObject();

    if (optionsObject.isEmpty())
        return;

    // parse the options list for values
    m_createProjectWrapper = checkBooleanOption(QStringLiteral("createProjectWrapper"),
                                                optionsObject);
    m_createIndividualLayers = checkBooleanOption(QStringLiteral("createIndividualLayers"),
                                                  optionsObject);
    m_fps = float(getRealOption(QStringLiteral("framesPerSecond"), optionsObject));
}

bool UipImporter::checkBooleanOption(const QString &optionName, const QJsonObject &options)
{
    if (!options.contains(optionName))
        return false;

    QJsonObject option = options.value(optionName).toObject();
    return option.value(QStringLiteral("value")).toBool();
}

double UipImporter::getRealOption(const QString &optionName, const QJsonObject &options)
{
    if (!options.contains(optionName))
        return false;

    QJsonObject option = options.value(optionName).toObject();
    return option.value(QStringLiteral("value")).toDouble();
}

QString UipImporter::processUipPresentation(UipPresentation *presentation, const QString &ouputFilePath)
{
    m_referencedMaterials.clear();
    m_aliasNodes.clear();
    m_componentNodes.clear();
    m_presentation = presentation;

    // Apply the properties of the first slide before running generator
//    Slide *firstSlide = static_cast<Slide*>(presentation->masterSlide()->firstChild());
//    if (firstSlide)
//        presentation->applySlidePropertyChanges(firstSlide);
    QString errorString;

    // create one component per layer
    GraphObject *layer = presentation->scene()->lastChild();
    QHash<QString, QBuffer *> layerComponentsMap;
    while (layer) {
        if (layer->type() == GraphObject::Layer) {
            // Create qml component from .uip presentation
            QString targetFile = ouputFilePath + QSSGQmlUtilities::qmlComponentName(presentation->name()) + QSSGQmlUtilities::qmlComponentName(layer->qmlId());
            QBuffer *qmlBuffer = new QBuffer();
            qmlBuffer->open(QIODevice::WriteOnly);
            QTextStream output(qmlBuffer);
            processNode(layer, output, 0, true, false);
            qmlBuffer->close();
            layerComponentsMap.insert(targetFile, qmlBuffer);

        } else if ( layer->type() == GraphObject::DefaultMaterial &&
                    layer->qmlId() == QStringLiteral("__Container")) {
            // UIP version > 5 which tries to be clever with reference materials
            // Instead of parsing these items as normal, instead we iterate the
            // materials container and generate new Components for each one and output them
            // to the "materials" folder
            m_exportPath.mkdir("materials");

            GraphObject *object = layer->firstChild();
            while (object) {
                generateMaterialComponent(object);
                object = object->nextSibling();
            }
        }

        layer = layer->previousSibling();
    }

   // create aliases folder
    if (m_referencedMaterials.count() > 0) {
        m_exportPath.mkdir("materials");
    }

    if (m_aliasNodes.count() > 0) {
        m_exportPath.mkdir("aliases");
    }

    if(m_componentNodes.count() > 0) {
        m_exportPath.mkdir("components");
    }

    // Generate Alias, Components, and ReferenceMaterials (2nd pass)
    // Use iterators because generateComponent can contain additional nested components
    QVector<ComponentNode *>::iterator componentIterator;
    for (componentIterator = m_componentNodes.begin(); componentIterator != m_componentNodes.end(); ++componentIterator)
        generateComponent(*componentIterator);

    for (auto material : m_referencedMaterials) {
        QString id = material->m_referencedMaterial_unresolved;
        if (id.startsWith("#"))
            id.remove(0, 1);
        auto obj = presentation->object(UniqueIdMapper::instance()->queryId(id.toUtf8()));
        if (!obj) {
            qWarning("Couldn't find object with id: %s", qPrintable(id));
            continue;
        }
        generateMaterialComponent(obj);
    }

    for (auto alias : m_aliasNodes) {
        QString id = alias->m_referencedNode_unresolved;
        if (id.startsWith("#"))
            id.remove(0, 1);
        generateAliasComponent(presentation->object(UniqueIdMapper::instance()->queryId(id.toUtf8())));
    }

    // Generate actual files from the buffers we created
    if (m_createIndividualLayers) {
        // Create a file for each component buffer
        for (auto targetName : layerComponentsMap.keys()) {
            QString targetFileName = targetName + QStringLiteral(".qml");
            QFile targetFile(targetFileName);
            if (!targetFile.open(QIODevice::WriteOnly)) {
                errorString += QString("Could not write to file: ") + targetFileName;
            } else {
                QTextStream output(&targetFile);
                writeHeader(output, true);
                QBuffer *componentBuffer = layerComponentsMap.value(targetName);
                componentBuffer->open(QIODevice::ReadOnly);
                output << componentBuffer->readAll();
                componentBuffer->close();
                targetFile.close();
                m_generatedFiles += targetFileName;
            }
        }
    }

    QString outputFileName = ouputFilePath
            + QSSGQmlUtilities::qmlComponentName(presentation->name()) + QStringLiteral(".qml");
    QFile outputFile(outputFileName);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        errorString += QString(QStringLiteral("Could not write to file: ") + outputFileName);
    } else {
        QTextStream output(&outputFile);
        // Write header
        writeHeader(output, true);

        // Window header
        if (m_presentation->scene()->m_useClearColor)
            output << "Rectangle {\n";
        else
            output << "Item {\n";
        output << QSSGQmlUtilities::insertTabs(1) << QStringLiteral("id: ")
               << QSSGQmlUtilities::sanitizeQmlId(m_presentation->name()) << Qt::endl;
        output << QSSGQmlUtilities::insertTabs(1) << QStringLiteral("width: ")
               << m_presentation->presentationWidth()<< Qt::endl;
        output << QSSGQmlUtilities::insertTabs(1) << QStringLiteral("height: ")
               << m_presentation->presentationHeight() << Qt::endl;
        if (m_presentation->scene()->m_useClearColor) {
            output << QSSGQmlUtilities::insertTabs(1) << QStringLiteral("color: ")
                   << QSSGQmlUtilities::colorToQml(m_presentation->scene()->m_clearColor) << Qt::endl;
        }

        // For each component buffer paste in each line with tablevel +1
        if (m_createIndividualLayers) {
            const auto layerFiles = layerComponentsMap.keys();
            output << Qt::endl;
            for (const auto &layerFile : layerFiles) {
                output << QSSGQmlUtilities::insertTabs(1)
                       << QFileInfo(layerFile).baseName() << " {}\n\n";
            }
        } else {
            for (auto buffer : layerComponentsMap) {
                buffer->open(QIODevice::ReadOnly);
                buffer->seek(0);
                while (!buffer->atEnd()) {
                    QByteArray line = buffer->readLine();
                    output << QSSGQmlUtilities::insertTabs(1) << line;
                }
                buffer->close();
            }
        }
        // Do States and AnimationTimelines here (same for all layers of the presentation)
        // Generate Animation Timeline
        generateAnimationTimeLine(output, 1, m_presentation, nullptr);
        // Generate States from Slides
        generateStatesFromSlides(m_presentation->masterSlide(), output, 1);

        // Window footer
        output << "}\n";
        outputFile.close();
        m_generatedFiles += outputFileName;
    }

    // Cleanup
    for (auto buffer : layerComponentsMap.values())
        delete buffer;

    return errorString;
}

QT_END_NAMESPACE
