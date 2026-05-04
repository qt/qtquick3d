// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#pragma once

#include <QtQml/private/qqmlabstractprofileradapter_p.h>
#include <QtQuick/private/qquickprofiler_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DProfilerAdapterFactory : public QQmlAbstractProfilerAdapterFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlAbstractProfilerAdapterFactory_iid FILE "qquick3dprofileradapter.json")
public:
    QQmlAbstractProfilerAdapter *create(const QString &key) override;
};

QT_END_NAMESPACE
