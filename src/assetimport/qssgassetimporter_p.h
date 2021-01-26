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

#ifndef QSSGASSETIMPORTER_H
#define QSSGASSETIMPORTER_H

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

#include <QtCore/QVariantMap>
#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtCore/QFile>

QT_BEGIN_NAMESPACE

class Q_QUICK3DASSETIMPORT_EXPORT QSSGAssetImporter : public QObject
{
    Q_OBJECT
public:
    virtual const QString name() const = 0;
    virtual const QStringList inputExtensions() const = 0;
    virtual const QString outputExtension() const = 0;
    virtual const QString type() const = 0;
    virtual const QVariantMap importOptions() const = 0;
    virtual const QString typeDescription() const = 0;
    virtual const QString import(const QString &sourceFile,
                                 const QDir &savePath,
                                 const QVariantMap &options,
                                 QStringList *generatedFiles = nullptr) = 0;
};

QT_END_NAMESPACE

#endif // QSSGASSETIMPORTER_H
