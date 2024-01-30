// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRRUNTIMEINFO_H
#define QOPENXRRUNTIMEINFO_H

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

#include <QtQuick3DXr/qtquick3dxrglobal.h>
#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QOpenXRManager;

class Q_QUICK3DXR_EXPORT QOpenXRRuntimeInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList enabledExtensions READ enabledExtensions CONSTANT)
    Q_PROPERTY(QString runtimeName READ runtimeName CONSTANT)
    Q_PROPERTY(QString runtimeVersion READ runtimeVersion CONSTANT)
    Q_PROPERTY(QString graphicsApiName READ graphicsApiName CONSTANT)
    Q_PROPERTY(bool multiViewRendering READ multiViewRendering CONSTANT)

    QML_NAMED_ELEMENT(XrRuntimeInfo)
    QML_UNCREATABLE("Created by XrView")

public:
    QOpenXRRuntimeInfo(QOpenXRManager *manager, QObject *parent = nullptr);
    QStringList enabledExtensions() const;
    QString runtimeName() const;
    QString runtimeVersion() const;
    QString graphicsApiName() const;
    bool multiViewRendering() const;

private:
    QOpenXRManager *m_openXRManager;
};

QT_END_NAMESPACE

#endif // QOPENXRRUNTIMEINFO_H
