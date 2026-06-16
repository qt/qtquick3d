// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include <QtQuick3DRuntimeRender/private/qssgrenderbuffer_p.h>

QT_BEGIN_NAMESPACE

QSSGRenderBuffer::QSSGRenderBuffer(QSSGRenderGraphObject::Type type)
    : QSSGRenderGraphObject(type, FlagT(QSSGRenderGraphObject::Flags::HasGraphicsResources))
{
    m_flags.setFlag(Flag::Dirty);
}
QSSGRenderBuffer::~QSSGRenderBuffer()
{
}

QT_END_NAMESPACE
