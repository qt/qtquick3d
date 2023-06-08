// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3drenderextensions_p.h"

#include <QtQuick3D/private/qquick3dobject_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuick3DRenderExtension
    \inmodule QtQuick3D
    \since 6.6

    \brief Abstract class for implementing user side render extensions.

    //! \sa QSSGRenderExtension
*/

QQuick3DRenderExtension::QQuick3DRenderExtension(QQuick3DObject *parent)
    : QQuick3DObject(*new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::RenderExtension), parent)
{

}

QQuick3DRenderExtension::~QQuick3DRenderExtension()
{

}

QT_END_NAMESPACE
