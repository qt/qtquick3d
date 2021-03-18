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

#ifndef UIAPARSER_H
#define UIAPARSER_H

#include "abstractxmlparser.h"

#include <QVariant>
#include <QHash>
#include <QMultiHash>

QT_BEGIN_NAMESPACE

struct DataInputEntry
{
    typedef QHash<QString, DataInputEntry> Map;
    // Metadata key - Datainput name mapping. Internally
    // key is type QString, as QVariant does not work that well
    // as keys to QMultiHash. Accesses to this map are therefore
    // done using QVariant automatic cast to QString (UIA format
    // anyway stores metadata as strings, so that is the usual use case).
    // TODO: use case of keys that are of different internal type but
    // equivalent after auto cast to QString eg. "9.0" type string vs "9.0" type float?
    typedef QMultiHash<QString, QString> MetadataMap;

    enum Type {
        TypeString,
        TypeFloat,
        TypeRangedNumber,
        TypeVec2,
        TypeVec3,
        TypeVariant,
        TypeBoolean
    };

    QString name;
    Type type = TypeString;
    float minValue = 0;
    float maxValue = 0;
    // As per QT3DS-2205 we currently need only a single key-value pair per datainput.
    // For efficiency we use separate items for both, as there is no need for more
    // elaborate containers.
    QVariant metaDataKey;
    QVariant metaData;

    // Just check that relative minmax values are sane, which also
    // covers the case when both are at default value of 0.0 i.e. unset.
    bool hasMinMax() const { return maxValue > minValue; }
};

Q_DECLARE_TYPEINFO(DataInputEntry, Q_MOVABLE_TYPE);

class UiaParser : AbstractXmlParser
{
public:
    struct Uia {
        struct Presentation {
            enum Type {
                Uip,
                Qml
            };
            Type type;
            QString id;
            QString source; // or preview for qml
        };

        QVector<Presentation> presentations;
        QString initialPresentationId;
        DataInputEntry::Map dataInputEntries;
        qint64 loadTimeMsecs = 0;

        bool isValid() const { return !presentations.isEmpty(); }
    };

    Uia parse(const QString &filename);

private:
    void parseApplication();
    void parsePresentations();

    Uia m_uia;
};

QT_END_NAMESPACE

#endif // UIAPARSER_H
