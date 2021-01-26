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

#ifndef QSSGQMLUTILITIES_P_H
#define QSSGQMLUTILITIES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DAssetImport/private/qtquick3dassetimportglobal_p.h>

#include <QString>
#include <QColor>
#include <QVariant>
#include <QTextStream>

QT_BEGIN_NAMESPACE

namespace QSSGQmlUtilities {

class PropertyMap
{
public:
    enum Type {
        Node,
        Model,
        Camera,
        DirectionalLight,
        PointLight,
        AreaLight,
        SpotLight,
        DefaultMaterial,
        PrincipledMaterial,
        Texture,
    };

    typedef QHash<QString, QVariant> PropertiesMap;

    static PropertyMap *instance();

    PropertiesMap *propertiesForType(Type type);
    QVariant getDefaultValue(Type type, const QString &property);
    bool isDefaultValue(Type type, const QString &property, const QVariant &value);


private:
    PropertyMap();
    ~PropertyMap();

    QHash<Type, PropertiesMap *> m_properties;

};

QString Q_QUICK3DASSETIMPORT_EXPORT insertTabs(int n);
QString Q_QUICK3DASSETIMPORT_EXPORT qmlComponentName(const QString &name);
QString Q_QUICK3DASSETIMPORT_EXPORT colorToQml(const QColor &color);
QString Q_QUICK3DASSETIMPORT_EXPORT variantToQml(const QVariant &variant);
QString Q_QUICK3DASSETIMPORT_EXPORT sanitizeQmlId(const QString &id);
QString Q_QUICK3DASSETIMPORT_EXPORT sanitizeQmlSourcePath(const QString &source, bool removeParentDirectory = false);
QString Q_QUICK3DASSETIMPORT_EXPORT stripParentDirectory(const QString &filePath);

void Q_QUICK3DASSETIMPORT_EXPORT writeQmlPropertyHelper(QTextStream &output, int tabLevel, PropertyMap::Type type, const QString &propertyName, const QVariant &value);

}


QT_END_NAMESPACE

#endif // QSSGQMLUTILITIES_P_H
