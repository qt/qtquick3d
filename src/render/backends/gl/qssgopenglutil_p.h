/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QSSGOPENGLUTIL_H
#define QSSGOPENGLUTIL_H

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

#include <QtQuick3DRender/private/qssgopengltokens_p.h>
#include <QtQuick3DRender/private/qssgrenderbasetypes_p.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLExtraFunctions>

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

QT_BEGIN_NAMESPACE

#if !defined(QT_OPENGL_ES)
static bool IsGlEsContext(QSSGRenderContextType inContextType)
{
    QSSGRenderContextTypes esContextTypes(QSSGRenderContextType::GLES2 |
                                          QSSGRenderContextType::GLES3 |
                                          QSSGRenderContextType::GLES3PLUS);
    if (esContextTypes & inContextType)
        return true;

    return false;
}
#endif

struct GLConversion
{
    GLConversion() {}

    static const char *processGLError(GLenum error)
    {
        const char *errorString;
        switch (error) {
        case GL_NO_ERROR:
            errorString = "GL_NO_ERROR";
            break;
        case GL_INVALID_ENUM:
            errorString = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorString = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorString = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "GL_OUT_OF_MEMORY";
            break;
        default:
            errorString = "Unknown GL error";
            break;
        }
        return errorString;
    }

    static QSSGRenderSrcBlendFunc fromGLToSrcBlendFunc(qint32 value)
    {
        switch (value) {
        case GL_ZERO:
            return QSSGRenderSrcBlendFunc::Zero;
        case GL_ONE:
            return QSSGRenderSrcBlendFunc::One;
        case GL_SRC_COLOR:
            return QSSGRenderSrcBlendFunc::SrcColor;
        case GL_ONE_MINUS_SRC_COLOR:
            return QSSGRenderSrcBlendFunc::OneMinusSrcColor;
        case GL_DST_COLOR:
            return QSSGRenderSrcBlendFunc::DstColor;
        case GL_ONE_MINUS_DST_COLOR:
            return QSSGRenderSrcBlendFunc::OneMinusDstColor;
        case GL_SRC_ALPHA:
            return QSSGRenderSrcBlendFunc::SrcAlpha;
        case GL_ONE_MINUS_SRC_ALPHA:
            return QSSGRenderSrcBlendFunc::OneMinusSrcAlpha;
        case GL_DST_ALPHA:
            return QSSGRenderSrcBlendFunc::DstAlpha;
        case GL_ONE_MINUS_DST_ALPHA:
            return QSSGRenderSrcBlendFunc::OneMinusDstAlpha;
        case GL_CONSTANT_COLOR:
            return QSSGRenderSrcBlendFunc::ConstantColor;
        case GL_ONE_MINUS_CONSTANT_COLOR:
            return QSSGRenderSrcBlendFunc::OneMinusConstantColor;
        case GL_CONSTANT_ALPHA:
            return QSSGRenderSrcBlendFunc::ConstantAlpha;
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return QSSGRenderSrcBlendFunc::OneMinusConstantAlpha;
        case GL_SRC_ALPHA_SATURATE:
            return QSSGRenderSrcBlendFunc::SrcAlphaSaturate;
        default:
            Q_ASSERT(false);
            return QSSGRenderSrcBlendFunc::Unknown;
        }
    }

    static GLenum fromSrcBlendFuncToGL(QSSGRenderSrcBlendFunc value)
    {
        switch (value) {
        case QSSGRenderSrcBlendFunc::Zero:
            return GL_ZERO;
        case QSSGRenderSrcBlendFunc::One:
            return GL_ONE;
        case QSSGRenderSrcBlendFunc::SrcColor:
            return GL_SRC_COLOR;
        case QSSGRenderSrcBlendFunc::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case QSSGRenderSrcBlendFunc::DstColor:
            return GL_DST_COLOR;
        case QSSGRenderSrcBlendFunc::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case QSSGRenderSrcBlendFunc::SrcAlpha:
            return GL_SRC_ALPHA;
        case QSSGRenderSrcBlendFunc::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case QSSGRenderSrcBlendFunc::DstAlpha:
            return GL_DST_ALPHA;
        case QSSGRenderSrcBlendFunc::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case QSSGRenderSrcBlendFunc::ConstantColor:
            return GL_CONSTANT_COLOR;
        case QSSGRenderSrcBlendFunc::OneMinusConstantColor:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case QSSGRenderSrcBlendFunc::ConstantAlpha:
            return GL_CONSTANT_ALPHA;
        case QSSGRenderSrcBlendFunc::OneMinusConstantAlpha:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        case QSSGRenderSrcBlendFunc::SrcAlphaSaturate:
            return GL_SRC_ALPHA_SATURATE;
        default:
            Q_ASSERT(false);
            return 0;
        }
    }

    static QSSGRenderDstBlendFunc fromGLToDstBlendFunc(qint32 value)
    {
        switch (value) {
        case GL_ZERO:
            return QSSGRenderDstBlendFunc::Zero;
        case GL_ONE:
            return QSSGRenderDstBlendFunc::One;
        case GL_SRC_COLOR:
            return QSSGRenderDstBlendFunc::SrcColor;
        case GL_ONE_MINUS_SRC_COLOR:
            return QSSGRenderDstBlendFunc::OneMinusSrcColor;
        case GL_DST_COLOR:
            return QSSGRenderDstBlendFunc::DstColor;
        case GL_ONE_MINUS_DST_COLOR:
            return QSSGRenderDstBlendFunc::OneMinusDstColor;
        case GL_SRC_ALPHA:
            return QSSGRenderDstBlendFunc::SrcAlpha;
        case GL_ONE_MINUS_SRC_ALPHA:
            return QSSGRenderDstBlendFunc::OneMinusSrcAlpha;
        case GL_DST_ALPHA:
            return QSSGRenderDstBlendFunc::DstAlpha;
        case GL_ONE_MINUS_DST_ALPHA:
            return QSSGRenderDstBlendFunc::OneMinusDstAlpha;
        case GL_CONSTANT_COLOR:
            return QSSGRenderDstBlendFunc::ConstantColor;
        case GL_ONE_MINUS_CONSTANT_COLOR:
            return QSSGRenderDstBlendFunc::OneMinusConstantColor;
        case GL_CONSTANT_ALPHA:
            return QSSGRenderDstBlendFunc::ConstantAlpha;
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return QSSGRenderDstBlendFunc::OneMinusConstantAlpha;
        default:
            Q_ASSERT(false);
            return QSSGRenderDstBlendFunc::Unknown;
        }
    }

    static GLenum fromDstBlendFuncToGL(QSSGRenderDstBlendFunc value)
    {
        switch (value) {
        case QSSGRenderDstBlendFunc::Zero:
            return GL_ZERO;
        case QSSGRenderDstBlendFunc::One:
            return GL_ONE;
        case QSSGRenderDstBlendFunc::SrcColor:
            return GL_SRC_COLOR;
        case QSSGRenderDstBlendFunc::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case QSSGRenderDstBlendFunc::DstColor:
            return GL_DST_COLOR;
        case QSSGRenderDstBlendFunc::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case QSSGRenderDstBlendFunc::SrcAlpha:
            return GL_SRC_ALPHA;
        case QSSGRenderDstBlendFunc::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case QSSGRenderDstBlendFunc::DstAlpha:
            return GL_DST_ALPHA;
        case QSSGRenderDstBlendFunc::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case QSSGRenderDstBlendFunc::ConstantColor:
            return GL_CONSTANT_COLOR;
        case QSSGRenderDstBlendFunc::OneMinusConstantColor:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case QSSGRenderDstBlendFunc::ConstantAlpha:
            return GL_CONSTANT_ALPHA;
        case QSSGRenderDstBlendFunc::OneMinusConstantAlpha:
            return GL_ONE_MINUS_CONSTANT_ALPHA;
        default:
            Q_ASSERT(false);
            return 0;
        }
    }

    static GLenum fromBlendEquationToGL(QSSGRenderBlendEquation value, bool nvAdvancedBlendSupported, bool khrAdvancedBlendSupported)
    {
        switch (value) {
        case QSSGRenderBlendEquation::Add:
            return GL_FUNC_ADD;
        case QSSGRenderBlendEquation::Subtract:
            return GL_FUNC_SUBTRACT;
        case QSSGRenderBlendEquation::ReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        default:
            Q_ASSERT(nvAdvancedBlendSupported || khrAdvancedBlendSupported);
            break;
        }

        if (nvAdvancedBlendSupported) {
            switch (value) {
            case QSSGRenderBlendEquation::Overlay:
                return GL_OVERLAY_NV;
            case QSSGRenderBlendEquation::ColorBurn:
                return GL_COLORBURN_NV;
            case QSSGRenderBlendEquation::ColorDodge:
                return GL_COLORDODGE_NV;
            default:
                break;
            }
        }

#if defined(GL_KHR_blend_equation_advanced)
        if (khrAdvancedBlendSupported) {
            switch (value) {
            case QSSGRenderBlendEquation::Overlay:
                return GL_OVERLAY_KHR;
            case QSSGRenderBlendEquation::ColorBurn:
                return GL_COLORBURN_KHR;
            case QSSGRenderBlendEquation::ColorDodge:
                return GL_COLORDODGE_KHR;
            default:
                break;
            }
        }
#endif
        Q_ASSERT(false);
        return GL_FUNC_ADD;
    }

    static QSSGRenderFace fromGLToFaces(GLenum value)
    {
        switch (value) {
        case GL_FRONT:
            return QSSGRenderFace::Front;
        case GL_BACK:
            return QSSGRenderFace::Back;
        case GL_FRONT_AND_BACK:
            return QSSGRenderFace::FrontAndBack;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderFace::Unknown;
    }

    static GLenum fromFacesToGL(QSSGRenderFace value)
    {
        switch (value) {
        case QSSGRenderFace::Front:
            return GL_FRONT;
        case QSSGRenderFace::Back:
            return GL_BACK;
        case QSSGRenderFace::FrontAndBack:
            return GL_FRONT_AND_BACK;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGReadFace fromGLToReadFaces(GLenum value)
    {
        switch (value) {
        case GL_FRONT:
            return QSSGReadFace::Front;
        case GL_BACK:
            return QSSGReadFace::Back;
        case GL_COLOR_ATTACHMENT0:
            return QSSGReadFace::Color0;
        case GL_COLOR_ATTACHMENT1:
            return QSSGReadFace::Color1;
        case GL_COLOR_ATTACHMENT2:
            return QSSGReadFace::Color2;
        case GL_COLOR_ATTACHMENT3:
            return QSSGReadFace::Color3;
        case GL_COLOR_ATTACHMENT4:
            return QSSGReadFace::Color4;
        case GL_COLOR_ATTACHMENT5:
            return QSSGReadFace::Color5;
        case GL_COLOR_ATTACHMENT6:
            return QSSGReadFace::Color6;
        case GL_COLOR_ATTACHMENT7:
            return QSSGReadFace::Color7;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGReadFace::Unknown;
    }

    static GLenum fromReadFacesToGL(QSSGReadFace value)
    {
        switch (value) {
        case QSSGReadFace::Front:
            return GL_FRONT;
        case QSSGReadFace::Back:
            return GL_BACK;
        case QSSGReadFace::Color0:
            return GL_COLOR_ATTACHMENT0;
        case QSSGReadFace::Color1:
            return GL_COLOR_ATTACHMENT1;
        case QSSGReadFace::Color2:
            return GL_COLOR_ATTACHMENT2;
        case QSSGReadFace::Color3:
            return GL_COLOR_ATTACHMENT3;
        case QSSGReadFace::Color4:
            return GL_COLOR_ATTACHMENT4;
        case QSSGReadFace::Color5:
            return GL_COLOR_ATTACHMENT5;
        case QSSGReadFace::Color6:
            return GL_COLOR_ATTACHMENT6;
        case QSSGReadFace::Color7:
            return GL_COLOR_ATTACHMENT7;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderWinding fromGLToWinding(GLenum value)
    {
        switch (value) {
        case GL_CW:
            return QSSGRenderWinding::Clockwise;
        case GL_CCW:
            return QSSGRenderWinding::CounterClockwise;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderWinding::Unknown;
    }

    static GLenum fromWindingToGL(QSSGRenderWinding value)
    {
        switch (value) {
        case QSSGRenderWinding::Clockwise:
            return GL_CW;
        case QSSGRenderWinding::CounterClockwise:
            return GL_CCW;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderBoolOp fromGLToBoolOp(GLenum value)
    {
        switch (value) {
        case GL_NEVER:
            return QSSGRenderBoolOp::Never;
        case GL_LESS:
            return QSSGRenderBoolOp::Less;
        case GL_EQUAL:
            return QSSGRenderBoolOp::Equal;
        case GL_LEQUAL:
            return QSSGRenderBoolOp::LessThanOrEqual;
        case GL_GREATER:
            return QSSGRenderBoolOp::Greater;
        case GL_NOTEQUAL:
            return QSSGRenderBoolOp::NotEqual;
        case GL_GEQUAL:
            return QSSGRenderBoolOp::GreaterThanOrEqual;
        case GL_ALWAYS:
            return QSSGRenderBoolOp::AlwaysTrue;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderBoolOp::Unknown;
    }

    static GLenum fromBoolOpToGL(QSSGRenderBoolOp value)
    {
        switch (value) {
        case QSSGRenderBoolOp::Never:
            return GL_NEVER;
        case QSSGRenderBoolOp::Less:
            return GL_LESS;
        case QSSGRenderBoolOp::Equal:
            return GL_EQUAL;
        case QSSGRenderBoolOp::LessThanOrEqual:
            return GL_LEQUAL;
        case QSSGRenderBoolOp::Greater:
            return GL_GREATER;
        case QSSGRenderBoolOp::NotEqual:
            return GL_NOTEQUAL;
        case QSSGRenderBoolOp::GreaterThanOrEqual:
            return GL_GEQUAL;
        case QSSGRenderBoolOp::AlwaysTrue:
            return GL_ALWAYS;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderHint fromGLToHint(GLenum value)
    {
        switch (value) {
        case GL_FASTEST:
            return QSSGRenderHint::Fastest;
        case GL_NICEST:
            return QSSGRenderHint::Nicest;
        case GL_DONT_CARE:
            return QSSGRenderHint::Unspecified;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderHint::Unknown;
    }

    static GLenum fromHintToGL(QSSGRenderHint value)
    {
        switch (value) {
        case QSSGRenderHint::Fastest:
            return GL_FASTEST;
        case QSSGRenderHint::Nicest:
            return GL_NICEST;
        case QSSGRenderHint::Unspecified:
            return GL_DONT_CARE;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderStencilOp fromGLToStencilOp(GLenum value)
    {
        switch (value) {
        case GL_KEEP:
            return QSSGRenderStencilOp::Keep;
        case GL_ZERO:
            return QSSGRenderStencilOp::Zero;
        case GL_REPLACE:
            return QSSGRenderStencilOp::Replace;
        case GL_INCR:
            return QSSGRenderStencilOp::Increment;
        case GL_INCR_WRAP:
            return QSSGRenderStencilOp::IncrementWrap;
        case GL_DECR:
            return QSSGRenderStencilOp::Decrement;
        case GL_DECR_WRAP:
            return QSSGRenderStencilOp::DecrementWrap;
        case GL_INVERT:
            return QSSGRenderStencilOp::Invert;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderStencilOp::Unknown;
    }

    static GLenum fromStencilOpToGL(QSSGRenderStencilOp value)
    {
        switch (value) {
        case QSSGRenderStencilOp::Keep:
            return GL_KEEP;
        case QSSGRenderStencilOp::Zero:
            return GL_ZERO;
        case QSSGRenderStencilOp::Replace:
            return GL_REPLACE;
        case QSSGRenderStencilOp::Increment:
            return GL_INCR;
        case QSSGRenderStencilOp::IncrementWrap:
            return GL_INCR_WRAP;
        case QSSGRenderStencilOp::Decrement:
            return GL_DECR;
        case QSSGRenderStencilOp::DecrementWrap:
            return GL_DECR_WRAP;
        case QSSGRenderStencilOp::Invert:
            return GL_INVERT;
        default:
            break;
        }

        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderComponentType fromGLToBufferComponentTypes(GLenum value)
    {
        switch (value) {
        case GL_UNSIGNED_BYTE:
            return QSSGRenderComponentType::UnsignedInteger8;
        case GL_BYTE:
            return QSSGRenderComponentType::Integer8;
        case GL_UNSIGNED_SHORT:
            return QSSGRenderComponentType::UnsignedInteger16;
        case GL_SHORT:
            return QSSGRenderComponentType::Integer16;
        case GL_UNSIGNED_INT:
            return QSSGRenderComponentType::UnsignedInteger32;
        case GL_FLOAT:
            return QSSGRenderComponentType::Float32;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderComponentType::Unknown;
    }

    static GLenum fromBufferComponentTypesToGL(QSSGRenderComponentType value)
    {
        switch (value) {
        case QSSGRenderComponentType::UnsignedInteger8:
            return GL_UNSIGNED_BYTE;
        case QSSGRenderComponentType::Integer8:
            return GL_BYTE;
        case QSSGRenderComponentType::UnsignedInteger16:
            return GL_UNSIGNED_SHORT;
        case QSSGRenderComponentType::Integer16:
            return GL_SHORT;
        case QSSGRenderComponentType::UnsignedInteger32:
            return GL_UNSIGNED_INT;
        case QSSGRenderComponentType::Integer32:
            return GL_INT;
        case QSSGRenderComponentType::Float32:
            return GL_FLOAT;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static GLenum fromIndexBufferComponentsTypesToGL(QSSGRenderComponentType value)
    {
        switch (value) {
        case QSSGRenderComponentType::UnsignedInteger8:
            return GL_UNSIGNED_BYTE;
        case QSSGRenderComponentType::UnsignedInteger16:
            return GL_UNSIGNED_SHORT;
        case QSSGRenderComponentType::UnsignedInteger32:
            return GL_UNSIGNED_INT;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static GLenum fromBindBufferFlagsToGL(QSSGRenderBufferType type)
    {
        switch(type) {
        case QSSGRenderBufferType::Vertex:
            return GL_ARRAY_BUFFER;
        case QSSGRenderBufferType::Index:
            return GL_ELEMENT_ARRAY_BUFFER;
        case QSSGRenderBufferType::Constant:
            return GL_UNIFORM_BUFFER;
        case QSSGRenderBufferType::Storage:
            return GL_SHADER_STORAGE_BUFFER;
        case QSSGRenderBufferType::AtomicCounter:
            return GL_ATOMIC_COUNTER_BUFFER;
        case QSSGRenderBufferType::DrawIndirect:
            return GL_DRAW_INDIRECT_BUFFER;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderBufferType fromGLToBindBufferFlags(GLenum value)
    {
        if (value == GL_ARRAY_BUFFER)
            return QSSGRenderBufferType::Vertex;
        else if (value == GL_ELEMENT_ARRAY_BUFFER)
            return QSSGRenderBufferType::Index;
        else if (value == GL_UNIFORM_BUFFER)
            return QSSGRenderBufferType::Constant;
        else if (value == GL_SHADER_STORAGE_BUFFER)
            return QSSGRenderBufferType::Storage;
        else if (value == GL_ATOMIC_COUNTER_BUFFER)
            return QSSGRenderBufferType::AtomicCounter;
        else if (value == GL_DRAW_INDIRECT_BUFFER)
            return QSSGRenderBufferType::DrawIndirect;
        else
            Q_ASSERT(false);

        return QSSGRenderBufferType(0);
    }

    static QSSGRenderBufferUsageType fromGLToBufferUsageType(GLenum value)
    {
        switch (value) {
        case GL_STATIC_DRAW:
            return QSSGRenderBufferUsageType::Static;
        case GL_DYNAMIC_DRAW:
            return QSSGRenderBufferUsageType::Dynamic;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderBufferUsageType::Unknown;
    }

    static GLenum fromBufferUsageTypeToGL(QSSGRenderBufferUsageType value)
    {
        switch (value) {
        case QSSGRenderBufferUsageType::Static:
            return GL_STATIC_DRAW;
        case QSSGRenderBufferUsageType::Dynamic:
            return GL_DYNAMIC_DRAW;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static GLenum fromQueryTypeToGL(QSSGRenderQueryType type)
    {
        GLenum retval = GL_INVALID_ENUM;
        if (type == QSSGRenderQueryType::Samples)
            retval = GL_ANY_SAMPLES_PASSED;
#if defined(GL_TIME_ELAPSED)
        else if (type == QSSGRenderQueryType::Timer)
            retval = GL_TIME_ELAPSED;
#elif defined(GL_TIME_ELAPSED_EXT)
        else if (type == QSSGRenderQueryType::Timer)
            retval = GL_TIME_ELAPSED_EXT;
#endif
        else
            Q_ASSERT(false);

        return retval;
    }

    static GLenum fromQueryResultTypeToGL(QSSGRenderQueryResultType type)
    {
        GLenum retval = GL_INVALID_ENUM;
        if (type == QSSGRenderQueryResultType::ResultAvailable)
            retval = GL_QUERY_RESULT_AVAILABLE;
        else if (type == QSSGRenderQueryResultType::Result)
            retval = GL_QUERY_RESULT;
        else
            Q_ASSERT(false);

        return retval;
    }

    static GLenum fromSyncTypeToGL(QSSGRenderSyncType type)
    {
        GLenum retval = GL_INVALID_ENUM;
        if (type == QSSGRenderSyncType::GpuCommandsComplete)
            retval = GL_SYNC_GPU_COMMANDS_COMPLETE;
        else
            Q_ASSERT(false);

        return retval;
    }

    static QSSGRenderTextureFormat replaceDeprecatedTextureFormat(QSSGRenderContextType type,
                                                                  QSSGRenderTextureFormat value,
                                                                  QSSGRenderTextureSwizzleMode &swizzleMode)
    {
        QSSGRenderContextTypes deprecatedContextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);
        QSSGRenderTextureFormat newValue = value;
        swizzleMode = QSSGRenderTextureSwizzleMode::NoSwizzle;

        if (!(deprecatedContextFlags & type)) {
            switch (value.format) {
            case QSSGRenderTextureFormat::Luminance8:
                newValue = QSSGRenderTextureFormat::R8;
                swizzleMode = QSSGRenderTextureSwizzleMode::L8toR8;
                break;
            case QSSGRenderTextureFormat::LuminanceAlpha8:
                newValue = QSSGRenderTextureFormat::RG8;
                swizzleMode = QSSGRenderTextureSwizzleMode::L8A8toRG8;
                break;
            case QSSGRenderTextureFormat::Alpha8:
                newValue = QSSGRenderTextureFormat::R8;
                swizzleMode = QSSGRenderTextureSwizzleMode::A8toR8;
                break;
            case QSSGRenderTextureFormat::Luminance16:
                newValue = QSSGRenderTextureFormat::R16;
                swizzleMode = QSSGRenderTextureSwizzleMode::L16toR16;
                break;
            default:
                break;
            }
        }

        return newValue;
    }

    static void NVRenderConvertSwizzleModeToGL(const QSSGRenderTextureSwizzleMode swizzleMode, GLint glSwizzle[4])
    {
        switch (swizzleMode) {
        case QSSGRenderTextureSwizzleMode::L16toR16:
        case QSSGRenderTextureSwizzleMode::L8toR8:
            glSwizzle[0] = GL_RED;
            glSwizzle[1] = GL_RED;
            glSwizzle[2] = GL_RED;
            glSwizzle[3] = GL_ONE;
            break;
        case QSSGRenderTextureSwizzleMode::L8A8toRG8:
            glSwizzle[0] = GL_RED;
            glSwizzle[1] = GL_RED;
            glSwizzle[2] = GL_RED;
            glSwizzle[3] = GL_GREEN;
            break;
        case QSSGRenderTextureSwizzleMode::A8toR8:
            glSwizzle[0] = GL_ZERO;
            glSwizzle[1] = GL_ZERO;
            glSwizzle[2] = GL_ZERO;
            glSwizzle[3] = GL_RED;
            break;
        case QSSGRenderTextureSwizzleMode::NoSwizzle:
        default:
            glSwizzle[0] = GL_RED;
            glSwizzle[1] = GL_GREEN;
            glSwizzle[2] = GL_BLUE;
            glSwizzle[3] = GL_ALPHA;
            break;
        }
    }

    static bool fromUncompressedTextureFormatToGL(QSSGRenderContextType type,
                                                  QSSGRenderTextureFormat value,
                                                  GLenum &outFormat,
                                                  GLenum &outDataType,
                                                  GLenum &outInternalFormat)
    {
        switch (value.format) {
        case QSSGRenderTextureFormat::R8:
            if (type == QSSGRenderContextType::GLES2) {
                outFormat = GL_ALPHA;
                outInternalFormat = GL_ALPHA;
            } else {
                outFormat = GL_RED;
                outInternalFormat = GL_R8;
            }
            outDataType = GL_UNSIGNED_BYTE;
            return true;
        case QSSGRenderTextureFormat::RG8:
            outFormat = GL_RG;
            outInternalFormat = GL_RG8;
            outDataType = GL_UNSIGNED_BYTE;
            return true;
        case QSSGRenderTextureFormat::RGBA8:
            outFormat = GL_RGBA;
            outInternalFormat = GL_RGBA8;
            outDataType = GL_UNSIGNED_BYTE;
            return true;
        case QSSGRenderTextureFormat::RGB8:
            outFormat = GL_RGB;
            outInternalFormat = GL_RGB8;
            outDataType = GL_UNSIGNED_BYTE;
            return true;
        case QSSGRenderTextureFormat::RGB565:
            outFormat = GL_RGB;
            outInternalFormat = GL_RGB8;
            outDataType = GL_UNSIGNED_SHORT_5_6_5;
            return true;
        case QSSGRenderTextureFormat::RGBA5551:
            outFormat = GL_RGBA;
            outInternalFormat = GL_RGBA8;
            outDataType = GL_UNSIGNED_SHORT_5_5_5_1;
            return true;
        case QSSGRenderTextureFormat::Alpha8:
            outFormat = GL_ALPHA;
            outInternalFormat = GL_ALPHA;
            outDataType = GL_UNSIGNED_BYTE;
            return true;
        case QSSGRenderTextureFormat::Luminance8:
            outFormat = GL_LUMINANCE;
            outInternalFormat = GL_LUMINANCE;
            outDataType = GL_UNSIGNED_BYTE;
            return true;
        case QSSGRenderTextureFormat::LuminanceAlpha8:
            outFormat = GL_LUMINANCE_ALPHA;
            outInternalFormat = GL_LUMINANCE_ALPHA;
            outDataType = GL_UNSIGNED_BYTE;
            return true;
        case QSSGRenderTextureFormat::Luminance16:
#if defined(QT_OPENGL_ES)
            outFormat = GL_LUMINANCE16F_EXT;
            outInternalFormat = GL_LUMINANCE16F_EXT;
#else
            outFormat = GL_LUMINANCE;
            outInternalFormat = GL_LUMINANCE;
#endif
            outDataType = GL_UNSIGNED_INT;
            return true;
        default:
            break;
        }

        QSSGRenderContextTypes contextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);
        // check extented texture formats
        if (!(contextFlags & type)) {
            switch (value.format) {
#if !defined(QT_OPENGL_ES)
            case QSSGRenderTextureFormat::R16: {
                if (IsGlEsContext(type)) {
                    outFormat = GL_RED_INTEGER;
                    outInternalFormat = GL_R16UI;
                } else {
                    outFormat = GL_RED;
                    outInternalFormat = GL_R16;
                }
                outDataType = GL_UNSIGNED_SHORT;
                return true;
            }
#endif
            case QSSGRenderTextureFormat::R16F:
                outFormat = GL_RED;
                outInternalFormat = GL_R16F;
                outDataType = GL_HALF_FLOAT;
                return true;
            case QSSGRenderTextureFormat::R32UI:
                outFormat = GL_RED_INTEGER;
                outInternalFormat = GL_R32UI;
                outDataType = GL_UNSIGNED_INT;
                return true;
            case QSSGRenderTextureFormat::R32F:
                outFormat = GL_RED;
                outInternalFormat = GL_R32F;
                outDataType = GL_FLOAT;
                return true;
            case QSSGRenderTextureFormat::RGBA16F:
                outFormat = GL_RGBA;
                outInternalFormat = GL_RGBA16F;
                outDataType = GL_HALF_FLOAT;
                return true;
            case QSSGRenderTextureFormat::RG16F:
                outFormat = GL_RG;
                outInternalFormat = GL_RG16F;
                outDataType = GL_HALF_FLOAT;
                return true;
            case QSSGRenderTextureFormat::RG32F:
                outFormat = GL_RG;
                outInternalFormat = GL_RG32F;
                outDataType = GL_FLOAT;
                return true;
            case QSSGRenderTextureFormat::RGBA32F:
                outFormat = GL_RGBA;
                outInternalFormat = GL_RGBA32F;
                outDataType = GL_FLOAT;
                return true;
            case QSSGRenderTextureFormat::RGB32F:
                outFormat = GL_RGB;
                outInternalFormat = GL_RGB32F;
                outDataType = GL_FLOAT;
                return true;
            case QSSGRenderTextureFormat::R11G11B10:
                outFormat = GL_RGB;
                outInternalFormat = GL_R11F_G11F_B10F;
                outDataType = GL_UNSIGNED_INT_10F_11F_11F_REV;
                return true;
            case QSSGRenderTextureFormat::RGB9E5:
                outFormat = GL_RGB;
                outInternalFormat = GL_RGB9_E5;
                outDataType = GL_UNSIGNED_INT_5_9_9_9_REV;
                return true;
            case QSSGRenderTextureFormat::SRGB8:
                outFormat = GL_RGB;
                outInternalFormat = GL_SRGB8;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            case QSSGRenderTextureFormat::SRGB8A8:
                outFormat = GL_RGBA;
                outInternalFormat = GL_SRGB8_ALPHA8;
                outDataType = GL_UNSIGNED_BYTE;
                return true;
            default:
                break;
            }
        }

        Q_ASSERT(false);
        return false;
    }

    static GLenum fromCompressedTextureFormatToGL(QSSGRenderTextureFormat value)
    {
        switch (value.format) {
        case QSSGRenderTextureFormat::RGBA_DXT1:
            return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case QSSGRenderTextureFormat::RGB_DXT1:
            return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        case QSSGRenderTextureFormat::RGBA_DXT3:
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case QSSGRenderTextureFormat::RGBA_DXT5:
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        default:
            break;
        }

        Q_ASSERT(false);
        return 0;
    }

    static bool fromDepthTextureFormatToGL(QSSGRenderContextType type,
                                           QSSGRenderTextureFormat value,
                                           GLenum &outFormat,
                                           GLenum &outDataType,
                                           GLenum &outInternalFormat)
    {
        QSSGRenderContextTypes theContextFlags(QSSGRenderContextType::GLES2 | QSSGRenderContextType::GL2);

        bool supportDepth24 = !(theContextFlags & type);
        bool supportDepth32f = !(theContextFlags & type);
        bool supportDepth24Stencil8 = !(theContextFlags & type);

        switch (value.format) {
        case QSSGRenderTextureFormat::Depth16:
            outFormat = GL_DEPTH_COMPONENT;
            outInternalFormat = GL_DEPTH_COMPONENT16;
            outDataType = GL_UNSIGNED_SHORT;
            return true;
        case QSSGRenderTextureFormat::Depth24:
            outFormat = GL_DEPTH_COMPONENT;
            outInternalFormat = (supportDepth24) ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16;
            outDataType = (supportDepth24) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
            return true;
        case QSSGRenderTextureFormat::Depth32:
            outFormat = GL_DEPTH_COMPONENT;
            outInternalFormat = (supportDepth32f) ? GL_DEPTH_COMPONENT32F : GL_DEPTH_COMPONENT16;
            outDataType = (supportDepth32f) ? GL_FLOAT : GL_UNSIGNED_SHORT;
            return true;
        case QSSGRenderTextureFormat::Depth24Stencil8:
            outFormat = (supportDepth24Stencil8) ? GL_DEPTH_STENCIL : GL_DEPTH_COMPONENT;
            outInternalFormat = (supportDepth24Stencil8) ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT16;
            outDataType = (supportDepth24Stencil8) ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_SHORT;
            return true;
        default:
            break;
        }

        Q_ASSERT(false);
        return false;
    }

    static GLenum fromTextureTargetToGL(QSSGRenderTextureTargetType value)
    {
        GLenum retval = 0;
        if (value == QSSGRenderTextureTargetType::Texture2D)
            retval = GL_TEXTURE_2D;
        else if (value == QSSGRenderTextureTargetType::Texture2D_MS)
            retval = GL_TEXTURE_2D_MULTISAMPLE;
        else if (value == QSSGRenderTextureTargetType::Texture2D_Array)
            retval = GL_TEXTURE_2D_ARRAY;
        else if (value == QSSGRenderTextureTargetType::TextureCube)
            retval = GL_TEXTURE_CUBE_MAP;
        else if (value == QSSGRenderTextureTargetType::TextureCubeNegX)
            retval = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
        else if (value == QSSGRenderTextureTargetType::TextureCubePosX)
            retval = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        else if (value == QSSGRenderTextureTargetType::TextureCubeNegY)
            retval = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
        else if (value == QSSGRenderTextureTargetType::TextureCubePosY)
            retval = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
        else if (value == QSSGRenderTextureTargetType::TextureCubeNegZ)
            retval = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
        else if (value == QSSGRenderTextureTargetType::TextureCubePosZ)
            retval = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
        else
            Q_ASSERT(false);

        return retval;
    }

    static QSSGRenderTextureTargetType fromGLToTextureTarget(GLenum value)
    {
        QSSGRenderTextureTargetType retval = QSSGRenderTextureTargetType::Unknown;

        if (value == GL_TEXTURE_2D)
            retval = QSSGRenderTextureTargetType::Texture2D;
        else if (value == GL_TEXTURE_2D_MULTISAMPLE)
            retval = QSSGRenderTextureTargetType::Texture2D_MS;
        else
            Q_ASSERT(false);

        return retval;
    }

    static GLenum fromTextureUnitToGL(QSSGRenderTextureUnit value)
    {
        quint32 v = static_cast<quint32>(value);
        GLenum retval = GL_TEXTURE0;
        retval = GL_TEXTURE0 + v;

        return retval;
    }

    static QSSGRenderTextureUnit fromGLToTextureUnit(GLenum value)
    {
        Q_ASSERT(value > GL_TEXTURE0);

        quint32 v = value - GL_TEXTURE0;
        QSSGRenderTextureUnit retval = QSSGRenderTextureUnit(v);

        return retval;
    }

    static GLenum fromTextureMinifyingOpToGL(QSSGRenderTextureMinifyingOp value)
    {
        switch (value) {
        case QSSGRenderTextureMinifyingOp::Nearest:
            return GL_NEAREST;
        case QSSGRenderTextureMinifyingOp::Linear:
            return GL_LINEAR;
        case QSSGRenderTextureMinifyingOp::NearestMipmapNearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case QSSGRenderTextureMinifyingOp::LinearMipmapNearest:
            return GL_LINEAR_MIPMAP_NEAREST;
        case QSSGRenderTextureMinifyingOp::NearestMipmapLinear:
            return GL_NEAREST_MIPMAP_LINEAR;
        case QSSGRenderTextureMinifyingOp::LinearMipmapLinear:
            return GL_LINEAR_MIPMAP_LINEAR;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderTextureMinifyingOp fromGLToTextureMinifyingOp(GLenum value)
    {
        switch (value) {
        case GL_NEAREST:
            return QSSGRenderTextureMinifyingOp::Nearest;
        case GL_LINEAR:
            return QSSGRenderTextureMinifyingOp::Linear;
        case GL_NEAREST_MIPMAP_NEAREST:
            return QSSGRenderTextureMinifyingOp::NearestMipmapNearest;
        case GL_LINEAR_MIPMAP_NEAREST:
            return QSSGRenderTextureMinifyingOp::LinearMipmapNearest;
        case GL_NEAREST_MIPMAP_LINEAR:
            return QSSGRenderTextureMinifyingOp::NearestMipmapLinear;
        case GL_LINEAR_MIPMAP_LINEAR:
            return QSSGRenderTextureMinifyingOp::LinearMipmapLinear;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderTextureMinifyingOp::Unknown;
    }

    static GLenum fromTextureMagnifyingOpToGL(QSSGRenderTextureMagnifyingOp value)
    {
        switch (value) {
        case QSSGRenderTextureMagnifyingOp::Nearest:
            return GL_NEAREST;
        case QSSGRenderTextureMagnifyingOp::Linear:
            return GL_LINEAR;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderTextureMagnifyingOp fromGLToTextureMagnifyingOp(GLenum value)
    {
        switch (value) {
        case GL_NEAREST:
            return QSSGRenderTextureMagnifyingOp::Nearest;
        case GL_LINEAR:
            return QSSGRenderTextureMagnifyingOp::Linear;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderTextureMagnifyingOp::Unknown;
    }

    static GLenum fromTextureCoordOpToGL(QSSGRenderTextureCoordOp value)
    {
        switch (value) {
        case QSSGRenderTextureCoordOp::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case QSSGRenderTextureCoordOp::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
        case QSSGRenderTextureCoordOp::Repeat:
            return GL_REPEAT;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderTextureCoordOp fromGLToTextureCoordOp(GLenum value)
    {
        switch (value) {
        case GL_CLAMP_TO_EDGE:
            return QSSGRenderTextureCoordOp::ClampToEdge;
        case GL_MIRRORED_REPEAT:
            return QSSGRenderTextureCoordOp::MirroredRepeat;
        case GL_REPEAT:
            return QSSGRenderTextureCoordOp::Repeat;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderTextureCoordOp::Unknown;
    }

    static GLenum fromTextureCompareModeToGL(QSSGRenderTextureCompareMode value)
    {
        switch (value) {
        case QSSGRenderTextureCompareMode::NoCompare:
            return GL_NONE;
        case QSSGRenderTextureCompareMode::CompareToRef:
            return GL_COMPARE_REF_TO_TEXTURE;
        default:
            break;
        }
        Q_ASSERT(false);
        return GL_INVALID_ENUM;
    }

    static QSSGRenderTextureCompareMode fromGLToTextureCompareMode(GLenum value)
    {
        switch (value) {
        case GL_NONE:
            return QSSGRenderTextureCompareMode::NoCompare;
        case GL_COMPARE_REF_TO_TEXTURE:
            return QSSGRenderTextureCompareMode::CompareToRef;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderTextureCompareMode::Unknown;
    }

    static GLenum fromTextureCompareFuncToGL(QSSGRenderTextureCompareOp value)
    {
        switch (value) {
        case QSSGRenderTextureCompareOp::Never:
            return GL_NEVER;
        case QSSGRenderTextureCompareOp::Less:
            return GL_LESS;
        case QSSGRenderTextureCompareOp::Equal:
            return GL_EQUAL;
        case QSSGRenderTextureCompareOp::LessThanOrEqual:
            return GL_LEQUAL;
        case QSSGRenderTextureCompareOp::Greater:
            return GL_GREATER;
        case QSSGRenderTextureCompareOp::NotEqual:
            return GL_NOTEQUAL;
        case QSSGRenderTextureCompareOp::GreaterThanOrEqual:
            return GL_GEQUAL;
        case QSSGRenderTextureCompareOp::AlwaysTrue:
            return GL_ALWAYS;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static GLenum fromImageFormatToGL(QSSGRenderTextureFormat value)
    {
        switch (value.format) {
        case QSSGRenderTextureFormat::R8:
            return GL_R8;
        case QSSGRenderTextureFormat::R32I:
            return GL_R32I;
        case QSSGRenderTextureFormat::R32UI:
            return GL_R32UI;
        case QSSGRenderTextureFormat::R32F:
            return GL_R32F;
        case QSSGRenderTextureFormat::RGBA8:
            return GL_RGBA8;
        case QSSGRenderTextureFormat::SRGB8A8:
            return GL_RGBA8_SNORM;
        case QSSGRenderTextureFormat::RG16F:
            return GL_RG16F;
        case QSSGRenderTextureFormat::RGBA16F:
            return GL_RGBA16F;
        case QSSGRenderTextureFormat::RGBA32F:
            return GL_RGBA32F;
        default:
            break;
        }

        Q_ASSERT(false);
        return GL_INVALID_ENUM;
    }

    static GLenum fromImageAccessToGL(QSSGRenderImageAccessType value)
    {
        switch (value) {
        case QSSGRenderImageAccessType::Read:
            return GL_READ_ONLY;
        case QSSGRenderImageAccessType::Write:
            return GL_WRITE_ONLY;
        case QSSGRenderImageAccessType::ReadWrite:
            return GL_READ_WRITE;
        default:
            break;
        }
        Q_ASSERT(false);
        return GL_INVALID_ENUM;
    }

    static GLbitfield fromBufferAccessBitToGL(QSSGRenderBufferAccessFlags flags)
    {
        GLbitfield retval = 0;

        if (flags & QSSGRenderBufferAccessTypeValues::Read)
            retval |= GL_MAP_READ_BIT;
        if (flags & QSSGRenderBufferAccessTypeValues::Write)
            retval |= GL_MAP_WRITE_BIT;
        if (flags & QSSGRenderBufferAccessTypeValues::Invalid)
            retval |= GL_MAP_INVALIDATE_BUFFER_BIT;
        if (flags & QSSGRenderBufferAccessTypeValues::InvalidRange)
            retval |= GL_MAP_INVALIDATE_RANGE_BIT;

        Q_ASSERT(retval);
        return retval;
    }

    static GLbitfield fromMemoryBarrierFlagsToGL(QSSGRenderBufferBarrierFlags flags)
    {
        GLbitfield retval = 0;
#if !defined(QT_OPENGL_ES)
        if (flags & QSSGRenderBufferBarrierValues::AtomicCounter)
            retval |= GL_ATOMIC_COUNTER_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::BufferUpdate)
            retval |= GL_BUFFER_UPDATE_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::CommandBuffer)
            retval |= GL_COMMAND_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::ElementArray)
            retval |= GL_ELEMENT_ARRAY_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::Framebuffer)
            retval |= GL_FRAMEBUFFER_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::PixelBuffer)
            retval |= GL_PIXEL_BUFFER_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::ShaderImageAccess)
            retval |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::ShaderStorage)
            retval |= GL_SHADER_STORAGE_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::TextureFetch)
            retval |= GL_TEXTURE_FETCH_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::TextureUpdate)
            retval |= GL_TEXTURE_UPDATE_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::TransformFeedback)
            retval |= GL_TRANSFORM_FEEDBACK_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::UniformBuffer)
            retval |= GL_UNIFORM_BARRIER_BIT;
        if (flags & QSSGRenderBufferBarrierValues::VertexAttribArray)
            retval |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
        Q_ASSERT(retval);
#else
        Q_UNUSED(flags);
#endif
        return retval;
    }

    static GLbitfield fromShaderTypeFlagsToGL(QSSGRenderShaderTypeFlags flags)
    {
        GLbitfield retval = 0;
        if (flags & QSSGRenderShaderTypeValue::Vertex)
            retval |= GL_VERTEX_SHADER_BIT;
        if (flags & QSSGRenderShaderTypeValue::Fragment)
            retval |= GL_FRAGMENT_SHADER_BIT;
        if (flags & QSSGRenderShaderTypeValue::TessControl)
            retval |= GL_TESS_CONTROL_SHADER_BIT;
        if (flags & QSSGRenderShaderTypeValue::TessEvaluation)
            retval |= GL_TESS_EVALUATION_SHADER_BIT;
        if (flags & QSSGRenderShaderTypeValue::Geometry)
#if defined(QT_OPENGL_ES_3_1)
            retval |= GL_GEOMETRY_SHADER_BIT_EXT;
#else
            retval |= GL_GEOMETRY_SHADER_BIT;
#endif
        Q_ASSERT(retval || !flags);
        return retval;
    }

    static GLenum fromPropertyDataTypesToShaderGL(QSSGRenderShaderDataType value)
    {
        // cube Sampler and mat22 unsupported
        switch (value) {
        case QSSGRenderShaderDataType::Float:
            return GL_FLOAT;
        case QSSGRenderShaderDataType::Vec2:
            return GL_FLOAT_VEC2;
        case QSSGRenderShaderDataType::Vec3:
            return GL_FLOAT_VEC3;
        case QSSGRenderShaderDataType::Vec4:
            return GL_FLOAT_VEC4;
        case QSSGRenderShaderDataType::Integer:
            return GL_INT;
        case QSSGRenderShaderDataType::IntegerVec2:
            return GL_INT_VEC2;
        case QSSGRenderShaderDataType::IntegerVec3:
            return GL_INT_VEC3;
        case QSSGRenderShaderDataType::IntegerVec4:
            return GL_INT_VEC4;
        case QSSGRenderShaderDataType::Boolean:
            return GL_BOOL;
        case QSSGRenderShaderDataType::BooleanVec2:
            return GL_BOOL_VEC2;
        case QSSGRenderShaderDataType::BooleanVec3:
            return GL_BOOL_VEC3;
        case QSSGRenderShaderDataType::BooleanVec4:
            return GL_BOOL_VEC4;
        case QSSGRenderShaderDataType::UnsignedInteger:
            return GL_UNSIGNED_INT;
        case QSSGRenderShaderDataType::UnsignedIntegerVec2:
            return GL_UNSIGNED_INT_VEC2;
        case QSSGRenderShaderDataType::UnsignedIntegerVec3:
            return GL_UNSIGNED_INT_VEC3;
        case QSSGRenderShaderDataType::UnsignedIntegerVec4:
            return GL_UNSIGNED_INT_VEC4;
        case QSSGRenderShaderDataType::Matrix3x3:
            return GL_FLOAT_MAT3;
        case QSSGRenderShaderDataType::Matrix4x4:
            return GL_FLOAT_MAT4;
        case QSSGRenderShaderDataType::Texture2D:
            return GL_SAMPLER_2D;
        case QSSGRenderShaderDataType::Texture2DArray:
            return GL_SAMPLER_2D_ARRAY;
        case QSSGRenderShaderDataType::TextureCube:
            return GL_SAMPLER_CUBE;
        case QSSGRenderShaderDataType::Image2D:
            return GL_IMAGE_2D;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderShaderDataType fromShaderGLToPropertyDataTypes(GLenum value)
    {
        // cube Sampler and mat22 unsupported
        switch (value) {
        case GL_FLOAT:
            return QSSGRenderShaderDataType::Float;
        case GL_FLOAT_VEC2:
            return QSSGRenderShaderDataType::Vec2;
        case GL_FLOAT_VEC3:
            return QSSGRenderShaderDataType::Vec3;
        case GL_FLOAT_VEC4:
            return QSSGRenderShaderDataType::Vec4;
        case GL_INT:
            return QSSGRenderShaderDataType::Integer;
        case GL_INT_VEC2:
            return QSSGRenderShaderDataType::IntegerVec2;
        case GL_INT_VEC3:
            return QSSGRenderShaderDataType::IntegerVec3;
        case GL_INT_VEC4:
            return QSSGRenderShaderDataType::IntegerVec4;
        case GL_BOOL:
            return QSSGRenderShaderDataType::Boolean;
        case GL_BOOL_VEC2:
            return QSSGRenderShaderDataType::BooleanVec2;
        case GL_BOOL_VEC3:
            return QSSGRenderShaderDataType::BooleanVec3;
        case GL_BOOL_VEC4:
            return QSSGRenderShaderDataType::BooleanVec4;
        case GL_UNSIGNED_INT:
            return QSSGRenderShaderDataType::UnsignedInteger;
        case GL_UNSIGNED_INT_VEC2:
            return QSSGRenderShaderDataType::UnsignedIntegerVec2;
        case GL_UNSIGNED_INT_VEC3:
            return QSSGRenderShaderDataType::UnsignedIntegerVec3;
        case GL_UNSIGNED_INT_VEC4:
            return QSSGRenderShaderDataType::UnsignedIntegerVec4;
        case GL_FLOAT_MAT3:
            return QSSGRenderShaderDataType::Matrix3x3;
        case GL_FLOAT_MAT4:
            return QSSGRenderShaderDataType::Matrix4x4;
        case GL_SAMPLER_2D:
            return QSSGRenderShaderDataType::Texture2D;
        case GL_SAMPLER_2D_ARRAY:
            return QSSGRenderShaderDataType::Texture2DArray;
        case GL_SAMPLER_CUBE:
            return QSSGRenderShaderDataType::TextureCube;
        case GL_IMAGE_2D:
            return QSSGRenderShaderDataType::Image2D;
        case GL_SAMPLER_2D_SHADOW:
            return QSSGRenderShaderDataType::Texture2D;
#if !defined(QT_OPENGL_ES)
        case GL_UNSIGNED_INT_ATOMIC_COUNTER:
            return QSSGRenderShaderDataType::Integer;
        case GL_UNSIGNED_INT_IMAGE_2D:
            return QSSGRenderShaderDataType::Image2D;
#endif
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderShaderDataType::Unknown;
    }

    static GLenum fromComponentTypeAndNumCompsToAttribGL(QSSGRenderComponentType compType, quint32 numComps)
    {
        if (compType == QSSGRenderComponentType::Float32) {
            switch (numComps) {
            case 1:
                return GL_FLOAT;
            case 2:
                return GL_FLOAT_VEC2;
            case 3:
                return GL_FLOAT_VEC3;
            case 4:
                return GL_FLOAT_VEC4;
            case 9:
                return GL_FLOAT_MAT3;
            case 16:
                return GL_FLOAT_MAT4;
            default:
                break;
            }
        }
        Q_ASSERT(false);
        return 0;
    }

    static void fromAttribGLToComponentTypeAndNumComps(GLenum enumVal, QSSGRenderComponentType &outCompType, quint32 &outNumComps)
    {
        switch (enumVal) {
        case GL_FLOAT:
            outCompType = QSSGRenderComponentType::Float32;
            outNumComps = 1;
            return;
        case GL_FLOAT_VEC2:
            outCompType = QSSGRenderComponentType::Float32;
            outNumComps = 2;
            return;
        case GL_FLOAT_VEC3:
            outCompType = QSSGRenderComponentType::Float32;
            outNumComps = 3;
            return;
        case GL_FLOAT_VEC4:
            outCompType = QSSGRenderComponentType::Float32;
            outNumComps = 4;
            return;
        case GL_FLOAT_MAT2:
            outCompType = QSSGRenderComponentType::Float32;
            outNumComps = 4;
            return;
        case GL_FLOAT_MAT3:
            outCompType = QSSGRenderComponentType::Float32;
            outNumComps = 9;
            return;
        case GL_FLOAT_MAT4:
            outCompType = QSSGRenderComponentType::Float32;
            outNumComps = 16;
            return;
        default:
            break;
        }
        Q_ASSERT(false);
        outCompType = QSSGRenderComponentType::Unknown;
        outNumComps = 0;
    }

    static GLenum fromRenderBufferFormatsToRenderBufferGL(QSSGRenderRenderBufferFormat value)
    {
        switch (value) {
#if defined(GL_DEPTH_COMPONENT32)
        case QSSGRenderRenderBufferFormat::RGBA4:
            return GL_RGBA4;
        case QSSGRenderRenderBufferFormat::RGB565:
            return GL_RGB565;
        case QSSGRenderRenderBufferFormat::RGBA5551:
            return GL_RGB5_A1;
        case QSSGRenderRenderBufferFormat::Depth16:
            return GL_DEPTH_COMPONENT16;
        case QSSGRenderRenderBufferFormat::Depth24:
            return GL_DEPTH_COMPONENT24;
        case QSSGRenderRenderBufferFormat::Depth32:
            return GL_DEPTH_COMPONENT32;
        case QSSGRenderRenderBufferFormat::StencilIndex8:
            return GL_STENCIL_INDEX8;
#else
        case QSSGRenderRenderBufferFormat::RGBA4:
            return GL_RGBA4;
        case QSSGRenderRenderBufferFormat::RGB565:
            return GL_RGB565;
        case QSSGRenderRenderBufferFormat::RGBA5551:
            return GL_RGB5_A1;
        case QSSGRenderRenderBufferFormat::Depth16:
            return GL_DEPTH_COMPONENT16;
        case QSSGRenderRenderBufferFormat::Depth24:
            return GL_DEPTH_COMPONENT24;
        case QSSGRenderRenderBufferFormat::StencilIndex8:
            return GL_STENCIL_INDEX8;
#endif
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderRenderBufferFormat fromRenderBufferGLToRenderBufferFormats(GLenum value)
    {
#if defined(GL_DEPTH_COMPONENT32)
        switch (value) {
        case GL_RGBA4:
            return QSSGRenderRenderBufferFormat::RGBA4;
        case GL_RGB565:
            return QSSGRenderRenderBufferFormat::RGB565;
        case GL_RGB5_A1:
            return QSSGRenderRenderBufferFormat::RGBA5551;
        case GL_DEPTH_COMPONENT16:
            return QSSGRenderRenderBufferFormat::Depth16;
        case GL_DEPTH_COMPONENT24:
            return QSSGRenderRenderBufferFormat::Depth24;
        case GL_DEPTH_COMPONENT32:
            return QSSGRenderRenderBufferFormat::Depth32;
        case GL_STENCIL_INDEX8:
            return QSSGRenderRenderBufferFormat::StencilIndex8;
#else
        switch (value) {
        case GL_RGBA4:
            return QSSGRenderRenderBufferFormat::RGBA4;
        case GL_RGB565:
            return QSSGRenderRenderBufferFormat::RGB565;
        case GL_RGB5_A1:
            return QSSGRenderRenderBufferFormat::RGBA5551;
        case GL_DEPTH_COMPONENT16:
            return QSSGRenderRenderBufferFormat::Depth16;
        case GL_DEPTH_COMPONENT24:
            return QSSGRenderRenderBufferFormat::Depth24;
        case GL_STENCIL_INDEX8:
            return QSSGRenderRenderBufferFormat::StencilIndex8;
#endif
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderRenderBufferFormat::Unknown;
    }

    static GLenum fromFramebufferAttachmentsToGL(QSSGRenderFrameBufferAttachment value)
    {
        switch (value) {
        case QSSGRenderFrameBufferAttachment::Color0:
            return GL_COLOR_ATTACHMENT0;
        case QSSGRenderFrameBufferAttachment::Color1:
            return GL_COLOR_ATTACHMENT1;
        case QSSGRenderFrameBufferAttachment::Color2:
            return GL_COLOR_ATTACHMENT2;
        case QSSGRenderFrameBufferAttachment::Color3:
            return GL_COLOR_ATTACHMENT3;
        case QSSGRenderFrameBufferAttachment::Color4:
            return GL_COLOR_ATTACHMENT4;
        case QSSGRenderFrameBufferAttachment::Color5:
            return GL_COLOR_ATTACHMENT5;
        case QSSGRenderFrameBufferAttachment::Color6:
            return GL_COLOR_ATTACHMENT6;
        case QSSGRenderFrameBufferAttachment::Color7:
            return GL_COLOR_ATTACHMENT7;
        case QSSGRenderFrameBufferAttachment::Depth:
            return GL_DEPTH_ATTACHMENT;
        case QSSGRenderFrameBufferAttachment::Stencil:
            return GL_STENCIL_ATTACHMENT;
        case QSSGRenderFrameBufferAttachment::DepthStencil:
            return GL_DEPTH_STENCIL_ATTACHMENT;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderFrameBufferAttachment fromGLToFramebufferAttachments(GLenum value)
    {
        switch (value) {
        case GL_COLOR_ATTACHMENT0:
            return QSSGRenderFrameBufferAttachment::Color0;
        case GL_COLOR_ATTACHMENT1:
            return QSSGRenderFrameBufferAttachment::Color1;
        case GL_COLOR_ATTACHMENT2:
            return QSSGRenderFrameBufferAttachment::Color2;
        case GL_COLOR_ATTACHMENT3:
            return QSSGRenderFrameBufferAttachment::Color3;
        case GL_COLOR_ATTACHMENT4:
            return QSSGRenderFrameBufferAttachment::Color4;
        case GL_COLOR_ATTACHMENT5:
            return QSSGRenderFrameBufferAttachment::Color5;
        case GL_COLOR_ATTACHMENT6:
            return QSSGRenderFrameBufferAttachment::Color6;
        case GL_COLOR_ATTACHMENT7:
            return QSSGRenderFrameBufferAttachment::Color7;
        case GL_DEPTH_ATTACHMENT:
            return QSSGRenderFrameBufferAttachment::Depth;
        case GL_STENCIL_ATTACHMENT:
            return QSSGRenderFrameBufferAttachment::Stencil;
        case GL_DEPTH_STENCIL_ATTACHMENT:
            return QSSGRenderFrameBufferAttachment::DepthStencil;
        }
        Q_ASSERT(false);
        return QSSGRenderFrameBufferAttachment::Unknown;
    }

    static GLbitfield fromClearFlagsToGL(QSSGRenderClearFlags flags)
    {
        GLbitfield retval = 0;
        if ((flags & QSSGRenderClearValues::Color))
            retval |= GL_COLOR_BUFFER_BIT;
        if ((flags & QSSGRenderClearValues::Depth))
            retval |= GL_DEPTH_BUFFER_BIT;
        if ((flags & QSSGRenderClearValues::Stencil))
            retval |= GL_STENCIL_BUFFER_BIT;

        return retval;
    }

    static QSSGRenderClearFlags fromGLToClearFlags(GLbitfield value)
    {
        QSSGRenderClearFlags retval;
        if ((value & GL_COLOR_BUFFER_BIT))
            retval |= QSSGRenderClearValues::Color;
        if ((value & GL_DEPTH_BUFFER_BIT))
            retval |= QSSGRenderClearValues::Depth;
        if ((value & GL_STENCIL_BUFFER_BIT))
            retval |= QSSGRenderClearValues::Stencil;

        return retval;
    }

    static GLenum fromDrawModeToGL(QSSGRenderDrawMode value, bool inTesselationSupported)
    {
        switch (value) {
        case QSSGRenderDrawMode::Points:
            return GL_POINTS;
        case QSSGRenderDrawMode::Lines:
            return GL_LINES;
        case QSSGRenderDrawMode::LineStrip:
            return GL_LINE_STRIP;
        case QSSGRenderDrawMode::LineLoop:
            return GL_LINE_LOOP;
        case QSSGRenderDrawMode::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case QSSGRenderDrawMode::TriangleFan:
            return GL_TRIANGLE_FAN;
        case QSSGRenderDrawMode::Triangles:
            return GL_TRIANGLES;
        case QSSGRenderDrawMode::Patches:
            return (inTesselationSupported) ? GL_PATCHES : GL_TRIANGLES;
        default:
            break;
        }
        Q_ASSERT(false);
        return GL_INVALID_ENUM;
    }

    static QSSGRenderDrawMode fromGLToDrawMode(GLenum value)
    {
        switch (value) {
        case GL_POINTS:
            return QSSGRenderDrawMode::Points;
        case GL_LINES:
            return QSSGRenderDrawMode::Lines;
        case GL_LINE_STRIP:
            return QSSGRenderDrawMode::LineStrip;
        case GL_LINE_LOOP:
            return QSSGRenderDrawMode::LineLoop;
        case GL_TRIANGLE_STRIP:
            return QSSGRenderDrawMode::TriangleStrip;
        case GL_TRIANGLE_FAN:
            return QSSGRenderDrawMode::TriangleFan;
        case GL_TRIANGLES:
            return QSSGRenderDrawMode::Triangles;
        case GL_PATCHES:
            return QSSGRenderDrawMode::Patches;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderDrawMode::Unknown;
    }

    static GLenum fromRenderStateToGL(QSSGRenderState value)
    {
        switch (value) {
        case QSSGRenderState::Blend:
            return GL_BLEND;
        case QSSGRenderState::CullFace:
            return GL_CULL_FACE;
        case QSSGRenderState::DepthTest:
            return GL_DEPTH_TEST;
        case QSSGRenderState::Multisample:
            return GL_MULTISAMPLE;
        case QSSGRenderState::StencilTest:
            return GL_STENCIL_TEST;
        case QSSGRenderState::ScissorTest:
            return GL_SCISSOR_TEST;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    static QSSGRenderState fromGLToRenderState(GLenum value)
    {
        switch (value) {
        case GL_BLEND:
            return QSSGRenderState::Blend;
        case GL_CULL_FACE:
            return QSSGRenderState::CullFace;
        case GL_DEPTH_TEST:
            return QSSGRenderState::DepthTest;
        case GL_MULTISAMPLE:
            return QSSGRenderState::Multisample;
        case GL_STENCIL_TEST:
            return QSSGRenderState::StencilTest;
        case GL_SCISSOR_TEST:
            return QSSGRenderState::ScissorTest;
        default:
            break;
        }
        Q_ASSERT(false);
        return QSSGRenderState::Unknown;
    }

    static bool fromReadPixelsToGlFormatAndType(QSSGRenderReadPixelFormat inReadPixels, GLuint *outFormat, GLuint *outType)
    {
        switch (inReadPixels) {
        case QSSGRenderReadPixelFormat::Alpha8:
            *outFormat = GL_ALPHA;
            *outType = GL_UNSIGNED_BYTE;
            break;
        case QSSGRenderReadPixelFormat::RGB565:
            *outFormat = GL_RGB;
            *outType = GL_UNSIGNED_SHORT_5_6_5;
            break;
        case QSSGRenderReadPixelFormat::RGB8:
            *outFormat = GL_RGB;
            *outType = GL_UNSIGNED_BYTE;
            break;
        case QSSGRenderReadPixelFormat::RGBA4444:
            *outFormat = GL_RGBA;
            *outType = GL_UNSIGNED_SHORT_4_4_4_4;
            break;
        case QSSGRenderReadPixelFormat::RGBA5551:
            *outFormat = GL_RGBA;
            *outType = GL_UNSIGNED_SHORT_5_5_5_1;
            break;
        case QSSGRenderReadPixelFormat::RGBA8:
            *outFormat = GL_RGBA;
            *outType = GL_UNSIGNED_BYTE;
            break;
        default:
            *outFormat = 0;
            *outType = 0;
            Q_ASSERT(false);
            return false;
        };

        return true;
    }

    static GLenum fromPathFillModeToGL(QSSGRenderPathFillMode inMode)
    {
        GLenum glFillMode = 0;

#if !defined(QT_OPENGL_ES)
        switch (inMode) {
        case QSSGRenderPathFillMode::Fill:
            glFillMode = GL_PATH_FILL_MODE_NV;
            break;
        case QSSGRenderPathFillMode::CountUp:
            glFillMode = GL_COUNT_UP_NV;
            break;
        case QSSGRenderPathFillMode::CountDown:
            glFillMode = GL_COUNT_DOWN_NV;
            break;
        case QSSGRenderPathFillMode::Invert:
            glFillMode = GL_INVERT;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
#else
        Q_UNUSED(inMode);
#endif
        return glFillMode;
    }

    static GLenum fromPathFontTargetToGL(QSSGRenderPathFontTarget inFontTarget)
    {
        GLenum glFontTarget = 0;

#if !defined(QT_OPENGL_ES)
        switch (inFontTarget) {
        case QSSGRenderPathFontTarget::StandardFont:
            glFontTarget = GL_STANDARD_FONT_NAME_NV;
            break;
        case QSSGRenderPathFontTarget::SystemFont:
            glFontTarget = GL_SYSTEM_FONT_NAME_NV;
            break;
        case QSSGRenderPathFontTarget::FileFont:
            glFontTarget = GL_FILE_NAME_NV;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
#else
        Q_UNUSED(inFontTarget)
#endif
        return glFontTarget;
    }

    static QSSGRenderPathReturnValues fromGLToPathFontReturn(GLenum inReturnValue)
    {
        QSSGRenderPathReturnValues returnValue;

        switch (inReturnValue) {
#if !defined(QT_OPENGL_ES)
        case GL_FONT_GLYPHS_AVAILABLE_NV:
            returnValue = QSSGRenderPathReturnValues::FontGlypsAvailable;
            break;
        case GL_FONT_TARGET_UNAVAILABLE_NV:
            returnValue = QSSGRenderPathReturnValues::FontTargetUnavailable;
            break;
        case GL_FONT_UNAVAILABLE_NV:
            returnValue = QSSGRenderPathReturnValues::FontUnavailable;
            break;
        case GL_FONT_UNINTELLIGIBLE_NV:
            returnValue = QSSGRenderPathReturnValues::FontUnintelligible;
            break;
#endif
        case GL_INVALID_ENUM:
        case GL_INVALID_VALUE:
            returnValue = QSSGRenderPathReturnValues::InvalidEnum;
            break;
        case GL_OUT_OF_MEMORY:
            returnValue = QSSGRenderPathReturnValues::OutOfMemory;
            break;
        default:
            Q_ASSERT(false);
            returnValue = QSSGRenderPathReturnValues::FontTargetUnavailable;
            break;
        }

        return returnValue;
    }

    static GLenum fromPathMissingGlyphsToGL(QSSGRenderPathMissingGlyphs inHandleGlyphs)
    {
        GLenum glMissingGlyphs = 0;

#if !defined(QT_OPENGL_ES)
        switch (inHandleGlyphs) {
        case QSSGRenderPathMissingGlyphs::SkipMissing:
            glMissingGlyphs = GL_SKIP_MISSING_GLYPH_NV;
            break;
        case QSSGRenderPathMissingGlyphs::UseMissing:
            glMissingGlyphs = GL_USE_MISSING_GLYPH_NV;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
#else
        Q_UNUSED(inHandleGlyphs);
#endif

        return glMissingGlyphs;
    }

    static GLenum fromPathListModeToGL(QSSGRenderPathListMode inListMode)
    {
        GLenum glListMode = 0;

#if !defined(QT_OPENGL_ES)
        switch (inListMode) {
        case QSSGRenderPathListMode::AccumAdjacentPairs:
            glListMode = GL_ACCUM_ADJACENT_PAIRS_NV;
            break;
        case QSSGRenderPathListMode::AdjacentPairs:
            glListMode = GL_ADJACENT_PAIRS_NV;
            break;
        case QSSGRenderPathListMode::FirstToRest:
            glListMode = GL_FIRST_TO_REST_NV;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
#else
        Q_UNUSED(inListMode);
#endif

        return glListMode;
    }

    static GLenum fromPathCoverModeToGL(QSSGRenderPathCoverMode inMode)
    {
        GLenum glCoverMode = 0;

#if !defined(QT_OPENGL_ES)
        switch (inMode) {
        case QSSGRenderPathCoverMode::ConvexHull:
            glCoverMode = GL_CONVEX_HULL_NV;
            break;
        case QSSGRenderPathCoverMode::BoundingBox:
            glCoverMode = GL_BOUNDING_BOX_NV;
            break;
        case QSSGRenderPathCoverMode::BoundingBoxOfBoundingBox:
            glCoverMode = GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV;
            break;
        case QSSGRenderPathCoverMode::PathFillCover:
            glCoverMode = GL_PATH_FILL_COVER_MODE_NV;
            break;
        case QSSGRenderPathCoverMode::PathStrokeCover:
            glCoverMode = GL_PATH_STROKE_COVER_MODE_NV;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
#else
        Q_UNUSED(inMode);
#endif

        return glCoverMode;
    }

    static GLenum fromPathTypeToGL(QSSGRenderPathFormatType value)
    {
        switch (value) {
        case QSSGRenderPathFormatType::Byte:
            return GL_BYTE;
        case QSSGRenderPathFormatType::UByte:
            return GL_UNSIGNED_BYTE;
        case QSSGRenderPathFormatType::Short:
            return GL_SHORT;
        case QSSGRenderPathFormatType::UShort:
            return GL_UNSIGNED_SHORT;
        case QSSGRenderPathFormatType::Int:
            return GL_INT;
        case QSSGRenderPathFormatType::Uint:
            return GL_UNSIGNED_INT;
#if !defined(QT_OPENGL_ES)
        case QSSGRenderPathFormatType::Bytes2:
            return GL_2_BYTES_NV;
        case QSSGRenderPathFormatType::Bytes3:
            return GL_3_BYTES_NV;
        case QSSGRenderPathFormatType::Bytes4:
            return GL_4_BYTES_NV;
        case QSSGRenderPathFormatType::Utf8:
            return GL_UTF8_NV;
        case QSSGRenderPathFormatType::Utf16:
            return GL_UTF16_NV;
#endif
        default:
            break;
        }
        Q_ASSERT(false);
        return GL_UNSIGNED_BYTE;
    }

    static GLbitfield fromPathFontStyleToGL(QSSGRenderPathFontStyleFlags flags)
    {
        GLbitfield retval = 0;
#if !defined(QT_OPENGL_ES)
        if (flags & QSSGRenderPathFontStyleValue::Bold)
            retval |= GL_BOLD_BIT_NV;
        if (flags & QSSGRenderPathFontStyleValue::Italic)
            retval |= GL_ITALIC_BIT_NV;
#else
        Q_UNUSED(flags);
#endif
        Q_ASSERT(retval || !flags);
        return retval;
    }

    static GLenum fromPathTransformToGL(QSSGRenderPathTransformType value)
    {
        switch (value) {
        case QSSGRenderPathTransformType::NoTransform:
            return GL_NONE;
#if !defined(QT_OPENGL_ES)
        case QSSGRenderPathTransformType::TranslateX:
            return GL_TRANSLATE_X_NV;
        case QSSGRenderPathTransformType::TranslateY:
            return GL_TRANSLATE_Y_NV;
        case QSSGRenderPathTransformType::Translate2D:
            return GL_TRANSLATE_2D_NV;
        case QSSGRenderPathTransformType::Translate3D:
            return GL_TRANSLATE_3D_NV;
        case QSSGRenderPathTransformType::Affine2D:
            return GL_AFFINE_2D_NV;
        case QSSGRenderPathTransformType::Affine3D:
            return GL_AFFINE_3D_NV;
        case QSSGRenderPathTransformType::TransposeAffine2D:
            return GL_TRANSPOSE_AFFINE_2D_NV;
        case QSSGRenderPathTransformType::TransposeAffine3D:
            return GL_TRANSPOSE_AFFINE_3D_NV;
#endif
        default:
            break;
        }
        Q_ASSERT(false);
        return GL_UNSIGNED_BYTE;
    }

    static GLbitfield fromPathMetricQueryFlagsToGL(QSSGRenderPathGlyphFontMetricFlags flags)
    {
        GLbitfield retval = 0;
#if !defined(QT_OPENGL_ES)
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphWidth)
            retval |= GL_GLYPH_WIDTH_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphHeight)
            retval |= GL_GLYPH_HEIGHT_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphHorizontalBearingX)
            retval |= GL_GLYPH_HORIZONTAL_BEARING_X_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphHorizontalBearingY)
            retval |= GL_GLYPH_HORIZONTAL_BEARING_Y_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphHorizontalBearingAdvance)
            retval |= GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphVerticalBearingX)
            retval |= GL_GLYPH_VERTICAL_BEARING_X_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphVerticalBearingY)
            retval |= GL_GLYPH_VERTICAL_BEARING_Y_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphVerticalBearingAdvance)
            retval |= GL_GLYPH_VERTICAL_BEARING_ADVANCE_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::GlyphHasKerning)
            retval |= GL_GLYPH_HAS_KERNING_BIT_NV;

        if (flags & QSSGRenderPathGlyphFontMetricValues::FontXMinBounds)
            retval |= GL_FONT_X_MIN_BOUNDS_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontYMinBounds)
            retval |= GL_FONT_Y_MIN_BOUNDS_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontXMaxBounds)
            retval |= GL_FONT_X_MAX_BOUNDS_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontYMaxBounds)
            retval |= GL_FONT_Y_MAX_BOUNDS_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontUnitsPerEm)
            retval |= GL_FONT_UNITS_PER_EM_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontAscender)
            retval |= GL_FONT_ASCENDER_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontDescender)
            retval |= GL_FONT_DESCENDER_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontHeight)
            retval |= GL_FONT_HEIGHT_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontMaxAdvanceWidth)
            retval |= GL_FONT_MAX_ADVANCE_WIDTH_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontMaxAdvanceHeight)
            retval |= GL_FONT_MAX_ADVANCE_HEIGHT_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontUnderlinePosition)
            retval |= GL_FONT_UNDERLINE_POSITION_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontMaxAdvanceWidth)
            retval |= GL_FONT_UNDERLINE_THICKNESS_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontHasKerning)
            retval |= GL_FONT_HAS_KERNING_BIT_NV;
        if (flags & QSSGRenderPathGlyphFontMetricValues::FontNumGlyphIndices)
            retval |= GL_FONT_NUM_GLYPH_INDICES_BIT_NV;
#else
        Q_UNUSED(flags);
#endif
        Q_ASSERT(retval || !flags);
        return retval;
    }
};

QT_END_NAMESPACE

#endif // QSSGOPENGLUTIL_H
