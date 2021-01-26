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

#ifndef PROPERTYMAP_H
#define PROPERTYMAP_H

#include "uippresentation.h"

QT_BEGIN_NAMESPACE

class PropertyMap
{    
public:

    struct Property
    {
        QString name;
        Q3DS::PropertyType type;
        QVariant defaultValue;
        bool animatable = true;
        Property() = default;
        Property(const QString &n, Q3DS::PropertyType t, const QVariant &v)
            : name(n)
            , type(t)
            , defaultValue(v)
        {}
    };

    typedef QHash<QString, Property> PropertiesMap;

    static PropertyMap *instance();

    PropertiesMap *propertiesForType(GraphObject::Type type);
    QVariant getDefaultValue(GraphObject::Type type, const QString &property);
    bool isDefaultValue(GraphObject::Type type, const QString &property, const QVariant &value);


private:
    PropertyMap();
    ~PropertyMap();

    QHash<GraphObject::Type, PropertiesMap *> m_properties;

};

QT_END_NAMESPACE

#endif // PROPERTYMAP_H
