// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3drenderextensions.h"

#include <QtQuick3D/private/qquick3dobject_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuick3DRenderExtension
    \inmodule QtQuick3D
    \since 6.7

    \brief Abstract class for implementing user side render extensions.

    \sa QSSGRenderExtension
*/

/*!
    \qmltype RenderExtension
    \instantiates QQuick3DRenderExtension
    \inqmlmodule QtQuick3D
    \inherits Object3D
    \since 6.7
    \brief An uncreatable abstract base type for render extensions.

    \sa QQuick3DRenderExtension, QSSGRenderExtension, QQuick3DViewport::extensions()
*/


QQuick3DRenderExtension::QQuick3DRenderExtension(QQuick3DObject *parent)
    : QQuick3DObject(*new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::RenderExtension), parent)
{

}

QQuick3DRenderExtension::~QQuick3DRenderExtension()
{

}

QT_END_NAMESPACE
