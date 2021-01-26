/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef UIPPARSER_H
#define UIPPARSER_H

#include <abstractxmlparser.h>
#include <functional>
#include <QSet>


QT_BEGIN_NAMESPACE

class UipPresentation;
class GraphObject;
class Slide;
class AnimationTrack;
class UipParser : public AbstractXmlParser
{
public:
    UipPresentation *parse(const QString &filename, const QString &presentationName);
    UipPresentation *parseData(const QByteArray &data, const QString &presentationName);

private:
    UipPresentation *createPresentation(const QString &presentationName);
    void parseUIP();
    void parseProject();
    void parseProjectSettings();
    void parseClasses();
    void parseBufferData();
    void parseImageBuffer();
    void parseGraph();
    void parseScene();
    void parseObjects(GraphObject *parent);
    void parseLogic();
    Slide *parseSlide(Slide *parent = nullptr, const QByteArray &idPrefix = QByteArray());
    void parseAddSet(Slide *slide, bool isSet, bool isMaster);
    void parseAnimationKeyFrames(const QString &data, AnimationTrack *animTrack);

    QByteArray getId(const QStringRef &desc, bool required = true);
    void resolveReferences(GraphObject *obj);

    typedef std::function<bool(const QByteArray &, const QString &)> ExternalFileLoadCallback;
    void parseExternalFileRef(ExternalFileLoadCallback callback);

    UipPresentation *m_presentation = nullptr;
};

QT_END_NAMESPACE

#endif // UIPPARSER_H
