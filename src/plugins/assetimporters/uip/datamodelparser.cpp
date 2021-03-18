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

#include "datamodelparser.h"

QT_BEGIN_NAMESPACE

QDebug operator<<(QDebug dbg, const DataModelParser::Property &prop)
{
    QDebugStateSaver saver(dbg);
    dbg.space() << "Property(" << prop.name << prop.type << prop.defaultValue;
    if (prop.type == Q3DS::Enum)
        dbg.space() << prop.enumValues;
    dbg << ")";
    return dbg;
}

DataModelParser *DataModelParser::instance()
{
    static DataModelParser p;
    return p.m_valid ? &p : nullptr;
}

DataModelParser::DataModelParser()
{
    QString fileName = QLatin1String(":/uipimporter/MetaData.xml");
    m_valid = setSource(fileName);
    if (!m_valid) {
        qWarning() << QObject::tr("Failed to create parser for %1").arg(fileName);
        return;
    }

    QXmlStreamReader *r = reader();
    if (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("MetaData"))
            parseMetaData();
        else
            r->raiseError(QObject::tr("Not a valid data model metadata: %1").arg(fileName));
    }

    if (r->hasError()) {
        m_valid = false;
        qWarning() << readerErrorString();
        return;
    }
}

void DataModelParser::parseMetaData()
{
    QXmlStreamReader *r = reader();
    while (r->readNextStartElement()) {
        if (r->name() == QStringLiteral("Category")) {
            r->skipCurrentElement();
        } else {
            QString typeName = r->name().toString();
            QVector<Property> props;
            while (r->readNextStartElement()) {
                if (r->name() == QStringLiteral("Property"))
                    parseProperty(&props);
                r->skipCurrentElement();
            }
            m_props.insert(typeName, props);
        }
    }
}

void DataModelParser::parseProperty(QVector<Property> *props)
{
    QXmlStreamReader *r = reader();
    Property prop;
    QStringRef defaultValue;
    Q3DS::PropertyType candidateType = Q3DS::Float;
    QString candidateTypeStr = QStringLiteral("Float");

    for (const QXmlStreamAttribute &a : r->attributes()) {
        if (a.name() == QStringLiteral("name")) {
            prop.name = a.value().toString();
        } else if (a.name() == QStringLiteral("type")) {
            if (!Q3DS::convertToPropertyType(a.value(), &prop.type, &prop.componentCount, "property type", r))
                return;
            prop.typeStr = a.value().toString();
        } else if (a.name() == QStringLiteral("list")) {
            prop.enumValues = a.value().toString().split(':');
            candidateType = Q3DS::Enum; // there may be a 'type' even when 'list' is present
            candidateTypeStr = QStringLiteral("Enum");
        } else if (a.name() == QStringLiteral("default")) {
            prop.defaultValue = a.value().toString();
        } else if (a.name() == QStringLiteral("animatable")) {
            if (!Q3DS::convertToBool(a.value(), &prop.animatable, "property type", r))
                return;
        }
    }

    if (prop.type == Q3DS::Unknown) {
        prop.type = candidateType;
        prop.typeStr = candidateTypeStr;
    }

    props->append(prop);
}

// this function must be non-const to be able to return a ptr to the value
const QVector<DataModelParser::Property> *DataModelParser::propertiesForType(const QString &typeName)
{
    return m_props.contains(typeName) ? &m_props[typeName] : nullptr;
}

QT_END_NAMESPACE
