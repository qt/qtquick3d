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

#include "uipparser.h"
#include "uippresentation.h"
#include "enummaps.h"
#include "uniqueidmapper.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

UipPresentation *UipParser::parse(const QString &filename, const QString &presentationName)
{
    if (!setSource(filename))
        return nullptr;

    return createPresentation(presentationName);
}

UipPresentation *UipParser::parseData(const QByteArray &data, const QString &presentationName)
{
    if (!setSourceData(data))
        return nullptr;

    return createPresentation(presentationName);
}

namespace  {
QString sanitizeFilename(const QString &filename) {
    //remove ending

    QStringList parts = filename.split(".");
    QString name = parts[0];
    name.replace(" ", "_");
    return name;
}
}

UipPresentation *UipParser::createPresentation(const QString &presentationName)
{
    if (m_presentation)
        delete m_presentation;
    m_presentation = new UipPresentation;

    m_presentation->setSourceFile(sourceInfo()->absoluteFilePath());
    m_presentation->setName(presentationName.isEmpty() ? sanitizeFilename(sourceInfo()->fileName()) : presentationName);

    QXmlStreamReader *r = reader();
    if (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("UIP")
                && (r->attributes().value(QLatin1String("version")) == QStringLiteral("6")
                    || r->attributes().value(QLatin1String("version")) == QStringLiteral("5")
                    || r->attributes().value(QLatin1String("version")) == QStringLiteral("4")
                    || r->attributes().value(QLatin1String("version")) == QStringLiteral("3"))) {
            parseUIP();
        } else {
            r->raiseError(QObject::tr("UIP version is too low, and is no longer supported."));
        }
    }

    if (r->hasError()) {
        qWarning() << readerErrorString();
        return nullptr;
    }

    const quint64 loadTime = elapsedSinceSetSource();
    qDebug("Presentation %s loaded in %lld ms", qPrintable(m_presentation->sourceFile()), loadTime);


    return m_presentation;
}

void UipParser::parseUIP()
{
    QXmlStreamReader *r = reader();
    int projectCount = 0;
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("Project")) {
            ++projectCount;
            if (projectCount == 1)
                parseProject();
            else
                r->raiseError(QObject::tr("Multiple Project elements found."));
        } else {
            r->skipCurrentElement();
        }
    }
}

void UipParser::parseProject()
{
    QXmlStreamReader *r = reader();
    bool foundGraph = false;
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("ProjectSettings"))
            parseProjectSettings();
        else if (r->name() == QStringLiteral("Classes"))
            parseClasses();
        else if (r->name() == QStringLiteral("BufferData"))
            parseBufferData();
        else if (r->name() == QStringLiteral("Graph")) {
            if (!foundGraph) {
                foundGraph = true;
                parseGraph();
            } else {
                r->raiseError(QObject::tr("Multiple Graph elements found."));
            }
        } else if (r->name() == QStringLiteral("Logic")) {
            if (foundGraph)
                parseLogic();
            else
                r->raiseError(QObject::tr("Encountered Logic element before Graph."));
        } else {
            r->skipCurrentElement();
        }
    }
}

void UipParser::parseProjectSettings()
{
    QXmlStreamReader *r = reader();
    for (const QXmlStreamAttribute &attr : r->attributes()) {
        if (attr.name() == QStringLiteral("author")) {
            m_presentation->setAuthor(attr.value().toString());
        } else if (attr.name() == QStringLiteral("company")) {
            m_presentation->setCompany(attr.value().toString());
        } else if (attr.name() == QStringLiteral("presentationWidth")) {
            int w;
            if (Q3DS::convertToInt(attr.value(), &w, "presentation width", r))
                m_presentation->setPresentationWidth(w);
        } else if (attr.name() == QStringLiteral("presentationHeight")) {
            int h;
            if (Q3DS::convertToInt(attr.value(), &h, "presentation height", r))
                m_presentation->setPresentationHeight(h);
        } else if (attr.name() == QStringLiteral("maintainAspect")) {
            bool v;
            if (Q3DS::convertToBool(attr.value(), &v, "maintainAspect value", r))
                m_presentation->setMaintainAspectRatio(v);
        }
    }
    r->skipCurrentElement();
}


void UipParser::parseClasses()
{
    QXmlStreamReader *r = reader();
    while (r->readNextStartElement()) {
//        if (r->name() == QStringLiteral("CustomMaterial")) {
//            parseExternalFileRef(std::bind(&UipPresentation::loadCustomMaterial, m_presentation.data(),
//                                           std::placeholders::_1, std::placeholders::_2));
//        } else if (r->name() == QStringLiteral("Effect")) {
//            parseExternalFileRef(std::bind(&UipPresentation::loadEffect, m_presentation.data(),
//                                           std::placeholders::_1, std::placeholders::_2));
//        } else if (r->name() == QStringLiteral("Behavior")) {
//            parseExternalFileRef(std::bind(&UipPresentation::loadBehavior, m_presentation.data(),
//                                           std::placeholders::_1, std::placeholders::_2));
//        } else if (r->name() == QStringLiteral("RenderPlugin")) {
//            r->raiseError(QObject::tr("RenderPlugin not supported"));
//        } else {
            r->skipCurrentElement();
//        }
    }
}

void UipParser::parseBufferData()
{
    QXmlStreamReader *r = reader();
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("ImageBuffer"))
            parseImageBuffer();
        else
            r->skipCurrentElement();
    }
}

void UipParser::parseImageBuffer()
{
    QXmlStreamReader *r = reader();
    auto a = r->attributes();
    const QStringRef &sourcePath = a.value(QStringLiteral("sourcepath"));
    const QStringRef &hasTransparency = a.value(QStringLiteral("hasTransparency"));

    if (!sourcePath.isEmpty() && !hasTransparency.isEmpty())
        m_presentation->registerImageBuffer(sourcePath.toString(), hasTransparency.compare(QStringLiteral("True")) == 0);

    r->skipCurrentElement();
}

void UipParser::parseGraph()
{
    QXmlStreamReader *r = reader();
    int sceneCount = 0;
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("Scene")) {
            ++sceneCount;
            if (sceneCount == 1)
                parseScene();
            else
                r->raiseError(QObject::tr("Multiple Scene elements found."));
        } else {
            r->skipCurrentElement();
        }
    }
}

void UipParser::parseScene()
{
    QXmlStreamReader *r = reader();
    QByteArray id = getId(r->name());
    if (id.isEmpty())
        return;

    auto scene = new Scene;
    scene->setProperties(r->attributes(), GraphObject::PropSetDefaults);
    m_presentation->registerObject(id, scene);
    m_presentation->setScene(scene);

    // Parse elements below Layer element
    while (r->readNextStartElement())
        parseObjects(scene);
}

void UipParser::parseObjects(GraphObject *parent)
{
    QXmlStreamReader *r = reader();
    Q_ASSERT(parent);
    QByteArray id = getId(r->name());
    if (id.isEmpty())
        return;

    GraphObject *obj = nullptr;

    if (r->name() == QStringLiteral("Layer"))
        obj = new LayerNode;
    else if (r->name() == QStringLiteral("Camera"))
        obj = new CameraNode;
    else if (r->name() == QStringLiteral("Light"))
        obj = new LightNode;
    else if (r->name() == QStringLiteral("Model"))
        obj = new ModelNode;
    else if (r->name() == QStringLiteral("Group"))
        obj = new GroupNode;
    else if (r->name() == QStringLiteral("Component"))
        obj = new ComponentNode;
    else if (r->name() == QStringLiteral("Text"))
        obj = new TextNode;
    else if (r->name() == QStringLiteral("Material"))
        obj = new DefaultMaterial;
    else if (r->name() == QStringLiteral("ReferencedMaterial"))
        obj = new ReferencedMaterial;
    else if (r->name() == QStringLiteral("CustomMaterial"))
        obj = new CustomMaterialInstance;
    else if (r->name() == QStringLiteral("Effect"))
        obj = new EffectInstance;
    else if (r->name() == QStringLiteral("Behavior"))
        obj = new BehaviorInstance;
    else if (r->name() == QStringLiteral("Image"))
        obj = new Image;
    else if (r->name() == QStringLiteral("Alias"))
        obj = new AliasNode;

    // Skip unknown elements in the XML
    if (!obj) {
        r->skipCurrentElement();
        return;
    }

    obj->setProperties(r->attributes(), GraphObject::PropSetDefaults);
    m_presentation->registerObject(id, obj);
    parent->appendChildNode(obj);

    while (r->readNextStartElement())
        parseObjects(obj);
}

void UipParser::parseLogic()
{
    QXmlStreamReader *r = reader();
    int masterCount = 0;
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("State")) {
            QStringRef compRef = r->attributes().value(QLatin1String("component"));
            if (!compRef.startsWith('#')) {
                r->raiseError(QObject::tr("Invalid ref '%1' in State").arg(compRef.toString()));
                return;
            }

            QByteArray refId = UniqueIdMapper::instance()->queryId(compRef.mid(1).toUtf8());
            GraphObject *slideTarget = m_presentation->object(refId);
            if (!slideTarget) {
                r->raiseError(
                            QObject::tr(
                                "State references unknown object '%1'").arg(
                                compRef.mid(1).toString()));
                return;
            }
            const QByteArray idPrefix = compRef.mid(1).toUtf8();
            if (slideTarget->type() == GraphObject::Scene) {
                if (++masterCount == 1) {
                    auto masterSlide = parseSlide(nullptr, idPrefix);
                    Q_ASSERT(masterSlide);
                    m_presentation->setMasterSlide(masterSlide);
                } else {
                    r->raiseError(QObject::tr("Multiple State (master slide) elements found."));
                }
            } else {
                Slide *componentMasterSlide = parseSlide(nullptr, idPrefix);
                Q_ASSERT(componentMasterSlide);
                static_cast<ComponentNode *>(slideTarget)->m_masterSlide = componentMasterSlide; // transfer ownership
            }
        } else {
            r->raiseError(QObject::tr("Logic can only have State children."));
        }
    }
}

Slide *UipParser::parseSlide(Slide *parent, const QByteArray &idPrefix)
{
    QXmlStreamReader *r = reader();
    QByteArray id = getId(r->name(), false);
    const bool isMaster = !parent;
    if (isMaster) {
        // The master slide may not have an id.
        if (id.isEmpty())
            id = idPrefix + QByteArrayLiteral("-Master");
    }
    if (id.isEmpty())
        return nullptr;

    Slide *slide = new Slide;
    slide->setProperties(r->attributes(), GraphObject::PropSetDefaults);
    m_presentation->registerObject(id, slide);
    if (parent)
        parent->appendChildNode(slide);

    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("State")) {
            if (isMaster)
                parseSlide(slide);
            else
                r->raiseError(QObject::tr("Encountered sub-slide in sub-slide."));
        } else if (r->name() == QStringLiteral("Add")) {
            parseAddSet(slide, false, isMaster);
        } else if (r->name() == QStringLiteral("Set")) {
            parseAddSet(slide, true, isMaster);
        } else {
            r->skipCurrentElement();
        }
    }

    return slide;
}

void UipParser::parseAddSet(Slide *slide, bool isSet, bool isMaster)
{
    QXmlStreamReader *r = reader();
    QStringRef ref = r->attributes().value(QLatin1String("ref"));
    if (!ref.startsWith('#')) {
        r->raiseError(QObject::tr("Invalid ref '%1' in Add/Set").arg(ref.toString()));
        return;
    }

    QByteArray refId = UniqueIdMapper::instance()->queryId(ref.mid(1).toUtf8());
    GraphObject *obj = m_presentation->object(refId);
    if (!obj) {
        r->raiseError(
                    QObject::tr("Add/Set references unknown object '%1'").arg(
                        ref.mid(1).toString()));
        return;
    }

    if (!isSet) {
        // Add: register the object for this slide
        slide->addObject(obj);
        // and set the properties on the object right away.
        GraphObject::PropSetFlags flags;
        if (isMaster)
            flags |= GraphObject::PropSetOnMaster;
        obj->setProperties(r->attributes(), flags);
    } else {
        // Set: store the property changes
        QScopedPointer<PropertyChangeList> changeList(new PropertyChangeList);
        for (const QXmlStreamAttribute &attr : r->attributes()) {
            if (attr.name() == QStringLiteral("ref"))
                continue;
            if (attr.name() == QStringLiteral("sourcepath")) {
                QString absoluteFileName =
                        m_presentation->assetFileName(attr.value().toString(), nullptr);
                changeList->append(PropertyChange(attr.name().toString(), absoluteFileName));
            } else {
                changeList->append(PropertyChange(attr.name().toString(), attr.value().toString()));
            }
        }
        if (!changeList->isEmpty())
            slide->addPropertyChanges(obj, changeList.take());
    }

    // Store animations and actions.
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("AnimationTrack")) {
            AnimationTrack animTrack;
            animTrack.m_target = obj;
            for (const QXmlStreamAttribute &attr : r->attributes()) {
                if (attr.name() == QStringLiteral("property")) {
                    animTrack.m_property = attr.value().toString().trimmed();
                } else if (attr.name() == QStringLiteral("type")) {
                    if (!EnumMap::enumFromStr(attr.value(), &animTrack.m_type))
                        r->raiseError(QObject::tr("Unknown animation type %1").arg(attr.value().toString()));
                } else if (attr.name() == QStringLiteral("dynamic")) {
                    Q3DS::convertToBool(attr.value(), &animTrack.m_dynamic, "'dynamic' attribute value", r);
                }
            }
            parseAnimationKeyFrames(r->readElementText(QXmlStreamReader::SkipChildElements).trimmed(), &animTrack);
            if (!animTrack.m_keyFrames.isEmpty())
                slide->addAnimation(animTrack);
        } else {
            r->skipCurrentElement();
        }
    }
}

void UipParser::parseAnimationKeyFrames(const QString &data, AnimationTrack *animTrack)
{
    QXmlStreamReader *r = reader();
    QString spaceOnlyData = data;
    spaceOnlyData.replace('\n', ' ');
    const QStringList values = spaceOnlyData.split(' ', Qt::SkipEmptyParts);
    if (values.isEmpty() || values.first().isEmpty())
        return;

    const float handednessAdjustment = ((animTrack->m_property == QStringLiteral("position.z"))
                                || (animTrack->m_property == QStringLiteral("rotation.x"))
                                || (animTrack->m_property == QStringLiteral("rotation.y")))
                            ? -1.0f : 1.0f;

    switch (animTrack->m_type) {
    case AnimationTrack::Linear:
        if (!(values.count() % 2)) {
            for (int i = 0; i < values.count() / 2; ++i) {
                AnimationTrack::KeyFrame kf;
                if (!Q3DS::convertToFloat(&values[i * 2], &kf.time, "keyframe time", r))
                    continue;
                if (!Q3DS::convertToFloat(&values[i * 2 + 1], &kf.value, "keyframe value", r))
                    continue;
                kf.value *= handednessAdjustment;
                animTrack->m_keyFrames.append(kf);
            }
        } else {
            r->raiseError(QObject::tr("Invalid Linear animation track: %1").arg(spaceOnlyData));
            return;
        }
        break;
    case AnimationTrack::EaseInOut:
        if (!(values.count() % 4)) {
            for (int i = 0; i < values.count() / 4; ++i) {
                AnimationTrack::KeyFrame kf;
                if (!Q3DS::convertToFloat(&values[i * 4], &kf.time, "keyframe time", r))
                    continue;
                if (!Q3DS::convertToFloat(&values[i * 4 + 1], &kf.value, "keyframe value", r))
                    continue;
                if (!Q3DS::convertToFloat(&values[i * 4 + 2], &kf.easeIn, "keyframe EaseIn", r))
                    continue;
                if (!Q3DS::convertToFloat(&values[i * 4 + 3], &kf.easeOut, "keyframe EaseOut", r))
                    continue;
                kf.value *= handednessAdjustment;
                animTrack->m_keyFrames.append(kf);
            }
        } else {
            r->raiseError(QObject::tr("Invalid EaseInOut animation track: %1").arg(spaceOnlyData));
            return;
        }
        break;
    case AnimationTrack::Bezier:
        if (!(values.count() % 6)) {
            for (int i = 0; i < values.count() / 6; ++i) {
                AnimationTrack::KeyFrame kf;
                if (!Q3DS::convertToFloat(&values[i * 6], &kf.time, "keyframe time", r))
                    continue;
                kf.time *= 1000.0f;
                if (!Q3DS::convertToFloat(&values[i * 6 + 1], &kf.value, "keyframe value", r))
                    continue;
                if (i < values.count() / 6 - 1) {
                    if (!Q3DS::convertToFloat(&values[i * 6 + 2], &kf.c2time, "keyframe C2 time", r))
                        continue;
                    kf.c2time *= 1000.0f;
                    if (!Q3DS::convertToFloat(&values[i * 6 + 3], &kf.c2value, "keyframe C2 value", r))
                        continue;
                } else { // last keyframe
                    kf.c2time = kf.c2value = 0.0f;
                }
                if (!Q3DS::convertToFloat(&values[i * 6 + 4], &kf.c1time, "keyframe C1 time", r))
                    continue;
                kf.c1time *= 1000.0f;
                if (!Q3DS::convertToFloat(&values[i * 6 + 5], &kf.c1value, "keyframe C1 value", r))
                    continue;
                kf.value *= handednessAdjustment;
                animTrack->m_keyFrames.append(kf);
            }
        } else {
            r->raiseError(QObject::tr("Invalid Bezier animation track: %1").arg(spaceOnlyData));
            return;
        }
        break;
    default:
        break;
    }
}

QByteArray UipParser::getId(const QStringRef &desc, bool required)
{
    QByteArray id = reader()->attributes().value(QLatin1String("id")).toUtf8();
    if (id.isEmpty() && required)
        reader()->raiseError(QObject::tr("Missing %1 id.").arg(desc.toString()));
    return UniqueIdMapper::instance()->generateUniqueId(id);
}


void UipParser::parseExternalFileRef(UipParser::ExternalFileLoadCallback callback)
{
    QXmlStreamReader *r = reader();
    auto a = r->attributes();

    const QStringRef id = a.value(QStringLiteral("id"));
    const QStringRef sourcePath = a.value(QStringLiteral("sourcepath"));

    // custommaterial/effect/behavior all expect ids to be prefixed with #
    const QByteArray decoratedId = QByteArrayLiteral("#") + UniqueIdMapper::instance()->queryId(id.toUtf8());
    const QString src = m_presentation->assetFileName(sourcePath.toString(), nullptr);
    if (!callback(decoratedId, src))
        r->raiseError(QObject::tr("Failed to load external file %1").arg(src));

    r->skipCurrentElement();
}

QT_END_NAMESPACE
