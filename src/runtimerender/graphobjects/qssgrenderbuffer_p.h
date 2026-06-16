// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSG_RENDER_BUFFER_H
#define QSSG_RENDER_BUFFER_H


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

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>

QT_BEGIN_NAMESPACE

class QSSGRenderExtension;
class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderBuffer : public QSSGRenderGraphObject
{
public:
    enum class Flag
    {
        Dirty = 1 << 0,
        DataDirty = 1 << 1
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    Q_DISABLE_COPY(QSSGRenderBuffer)

    QSSGRenderGraphObject *m_parent = nullptr;

    QSSGRenderExtension *m_extensionsSource = nullptr;

    Flags m_flags;
    quint32 m_size = 0;
    QByteArray m_data;

    QSSGRenderBuffer(QSSGRenderGraphObject::Type type = QSSGRenderGraphObject::Type::ShaderStorageBuffer);
    ~QSSGRenderBuffer();
};

class QSSGShaderBuffer
{
public:
    QSSGRenderBuffer *buffer = nullptr;
    QByteArray dataType;
    QSSGMemoryQualifiers qualifiers;
    QSSGShaderBuffer() = default;
    QSSGShaderBuffer(QSSGRenderBuffer *buffer, const QByteArray &dataType, QSSGMemoryQualifiers qualifiers)
        : buffer(buffer), dataType(dataType), qualifiers(qualifiers) {}
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGRenderBuffer::Flags)

QT_END_NAMESPACE

#endif
