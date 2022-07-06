// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DRuntimeRender/private/qssgrendereffect_p.h>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

QT_BEGIN_NAMESPACE

QSSGRenderEffect::QSSGRenderEffect() : QSSGRenderGraphObject(Type::Effect) {}

QSSGRenderEffect::~QSSGRenderEffect()
{

}

void QSSGRenderEffect::markDirty()
{
    flags |= FlagT(Flags::Dirty);
}

void QSSGRenderEffect::clearDirty()
{
    flags &= ~FlagT(Flags::Dirty);
}

QT_END_NAMESPACE
