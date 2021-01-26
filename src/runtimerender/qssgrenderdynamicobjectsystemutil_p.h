/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSG_RENDER_DYNAMIC_OBJECT_SYSTEM_UTIL_H
#define QSSG_RENDER_DYNAMIC_OBJECT_SYSTEM_UTIL_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
namespace dynamic {

constexpr inline quint32 align(quint32 inValue) Q_DECL_NOTHROW
{
    return (inValue % 4) ? (inValue + (4 - (inValue % 4))) : inValue;
}

constexpr inline quint32 align8(quint32 inValue) Q_DECL_NOTHROW
{
    return (inValue % 8) ? (inValue + (8 - (inValue % 8))) : inValue;
}

inline quint32 getSizeofShaderDataType(QSSGRenderShaderDataType value)
{
    switch (value) {
    case QSSGRenderShaderDataType::Unknown:
        return 0;
    case QSSGRenderShaderDataType::Integer: // qint32,
        return sizeof(qint32);
    case QSSGRenderShaderDataType::IntegerVec2: // qint32_2,
        return sizeof(qint32_2);
    case QSSGRenderShaderDataType::IntegerVec3: // qint32_3,
        return sizeof(qint32_3);
    case QSSGRenderShaderDataType::IntegerVec4: // qint32_4,
        return sizeof(qint32_4);
    case QSSGRenderShaderDataType::Boolean: // bool
        return sizeof(bool);
    case QSSGRenderShaderDataType::BooleanVec2: // bool_2,
        return sizeof(bool_2);
    case QSSGRenderShaderDataType::BooleanVec3: // bool_3,
        return sizeof(bool_3);
    case QSSGRenderShaderDataType::BooleanVec4: // bool_4,
        return sizeof(bool_4);
    case QSSGRenderShaderDataType::Float: // float,
        return sizeof(float);
    case QSSGRenderShaderDataType::Vec2: // QVector2D,
        return sizeof(QVector2D);
    case QSSGRenderShaderDataType::Vec3: // QVector3D,
        return sizeof(QVector3D);
    case QSSGRenderShaderDataType::Vec4: // QVector4D,
        Q_FALLTHROUGH();
    case QSSGRenderShaderDataType::Rgba: // QColor, which will be converted to QVector4D,
        return sizeof(QVector4D);
    case QSSGRenderShaderDataType::UnsignedInteger: // quint32,
        return sizeof(quint32);
    case QSSGRenderShaderDataType::UnsignedIntegerVec2: // quint32_2,
        return sizeof(quint32_2);
    case QSSGRenderShaderDataType::UnsignedIntegerVec3: // quint32_3,
        return sizeof(quint32_3);
    case QSSGRenderShaderDataType::UnsignedIntegerVec4: // quint32_4,
        return sizeof(quint32_4);
    case QSSGRenderShaderDataType::Matrix3x3: // QMatrix3x3,
        return sizeof(QMatrix3x3);
    case QSSGRenderShaderDataType::Matrix4x4: // QMatrix4x4,
        return sizeof(QMatrix4x4);
    case QSSGRenderShaderDataType::Texture2D: // QSSGRenderTexture2D *,
        Q_FALLTHROUGH();
    case QSSGRenderShaderDataType::Texture2DHandle: // QSSGRenderTexture2D **,
        Q_FALLTHROUGH();
    case QSSGRenderShaderDataType::TextureCube: // QSSGRenderTextureCube *,
        Q_FALLTHROUGH();
    case QSSGRenderShaderDataType::TextureCubeHandle: // QSSGRenderTextureCube **,
        Q_FALLTHROUGH();
    case QSSGRenderShaderDataType::Image2D: // QSSGRenderImage2D *,
        Q_FALLTHROUGH();
    case QSSGRenderShaderDataType::DataBuffer: // QSSGRenderDataBufferPtr
        return sizeof (void *);
    }
    Q_UNREACHABLE();
    return 0;
}
}
QT_END_NAMESPACE

#endif
