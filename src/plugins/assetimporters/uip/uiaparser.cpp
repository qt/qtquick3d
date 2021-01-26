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

#include "uiaparser.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

UiaParser::Uia UiaParser::parse(const QString &filename)
{
    UiaParser::Uia invalidUia;

    if (!setSource(filename))
        return invalidUia;

    m_uia = UiaParser::Uia();

    QXmlStreamReader *r = reader();
    if (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("application"))
            parseApplication();
        else
            r->raiseError(QObject::tr("Not a valid uia document: %1").arg(filename));
    }
    if (r->hasError()) {
        qWarning() << readerErrorString();
        return invalidUia;
    }

    m_uia.loadTimeMsecs = elapsedSinceSetSource();
    qDebug("%s loaded in %lld ms", qPrintable(filename), m_uia.loadTimeMsecs);
    return m_uia;
}

void UiaParser::parseApplication()
{
    QXmlStreamReader *r = reader();
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("assets")) {
            QStringRef initialId = r->attributes().value(QLatin1String("initial"));
            if (!initialId.isEmpty())
                m_uia.initialPresentationId = initialId.toString();
            parsePresentations();
        } else {
            // statemachine not supported
            r->skipCurrentElement();
        }
    }
}

void UiaParser::parsePresentations()
{
    QXmlStreamReader *r = reader();
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("presentation")) {
            QXmlStreamAttributes attrs = r->attributes();
            QStringRef id = attrs.value(QLatin1String("id"));
            QStringRef src = attrs.value(QLatin1String("src"));
            if (!id.isEmpty() && !src.isEmpty()) {
                Uia::Presentation pres;
                pres.type = Uia::Presentation::Uip;
                pres.id = id.toString();
                pres.source = src.toString();
                m_uia.presentations.append(pres);
                if (m_uia.initialPresentationId.isEmpty())
                    m_uia.initialPresentationId = pres.id;
            } else {
                r->raiseError(QObject::tr("Malformed presentation element"));
                m_uia.presentations.clear();
            }
        } else if (r->name() == QStringLiteral("presentation-qml")) {
            QXmlStreamAttributes attrs = r->attributes();
            QStringRef id = attrs.value(QLatin1String("id"));
            QStringRef args = attrs.value(QLatin1String("args"));
            if (!id.isEmpty()) {
                Uia::Presentation pres;
                pres.type = Uia::Presentation::Qml;
                pres.id = id.toString();
                pres.source = args.toString();
                m_uia.presentations.append(pres);
            }
        } else if (r->name() == QStringLiteral("dataInput")) {
            QXmlStreamAttributes attrs = r->attributes();
            QStringRef name = attrs.value(QLatin1String("name"));
            QStringRef type = attrs.value(QLatin1String("type"));
            QStringRef minValue = attrs.value(QLatin1String("min"));
            QStringRef maxValue = attrs.value(QLatin1String("max"));
            QStringRef metaDataKey = attrs.value(QLatin1String("metadatakey"));
            QStringRef metaData = attrs.value(QLatin1String("metadata"));
            if (name.isEmpty() || type.isEmpty()) {
                r->raiseError(QObject::tr("Malformed dataInput element"));
                m_uia.presentations.clear();
            } else {
                DataInputEntry e;
                e.name = name.toString();

                if (!metaData.isEmpty())
                    e.metaData = QVariant::fromValue(metaData.toString());
                if (!metaDataKey.isEmpty())
                    e.metaDataKey = QVariant::fromValue(metaDataKey.toString());

                if (type == QStringLiteral("String")) {
                    e.type = DataInputEntry::TypeString;
                } else if (type == QLatin1String("Float")) {
                    e.type = DataInputEntry::TypeFloat;
                } else if (type == QLatin1String("Ranged Number")) {
                    e.type = DataInputEntry::TypeRangedNumber;
                } else if (type == QLatin1String("Vector2")) {
                    e.type = DataInputEntry::TypeVec2;
                } else if (type == QLatin1String("Vector3")) {
                    e.type = DataInputEntry::TypeVec3;
                } else if (type == QLatin1String("Variant")) {
                    e.type = DataInputEntry::TypeVariant;
                } else if (type == QLatin1String("Boolean")) {
                    e.type = DataInputEntry::TypeBoolean;
                } else {
                    r->raiseError(QObject::tr("Unknown type in dataInput element"));
                    m_uia.presentations.clear();
                }
                if (!minValue.isEmpty())
                    e.minValue = minValue.toFloat();
                if (!maxValue.isEmpty())
                    e.maxValue = maxValue.toFloat();
                m_uia.dataInputEntries.insert(e.name, e);
            }
        }
        r->skipCurrentElement();
    }
}

QT_END_NAMESPACE
