// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CLOUDNOISETEXTUREDATA_H
#define CLOUDNOISETEXTUREDATA_H

#include <QtQuick3D/QQuick3DTextureData>
#include <QtQml/qqml.h>

class CloudNoiseTextureData : public QQuick3DTextureData
{
    Q_OBJECT
    QML_ELEMENT
public:
    CloudNoiseTextureData(QQuick3DObject *parent = nullptr);
};

#endif // CLOUDNOISETEXTUREDATA_H
