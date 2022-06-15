// Copyright (C) 2008-2012 NVIDIA Corporation.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGN_RENDERQSSGDER_TYPES_H
#define QSSGN_RENDERQSSGDER_TYPES_H

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

#include <QtQuick3DUtils/private/qssgdataref_p.h>

#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>
#include <QtGui/QMatrix4x4>
#include <QtGui/QMatrix3x3>
#include <QFloat16>

#include <cmath>

QT_BEGIN_NAMESPACE

enum class QSSGRenderComponentType // stored in mesh files, the values must not change, must match Mesh::ComponentType
{
    UnsignedInteger8 = 1,
    Integer8,
    UnsignedInteger16,
    Integer16,
    UnsignedInteger32,
    Integer32,
    UnsignedInteger64,
    Integer64,
    Float16,
    Float32,
    Float64
};

inline const char *toString(QSSGRenderComponentType value)
{
    switch (value) {
    case QSSGRenderComponentType::UnsignedInteger8:
        return "UnsignedInteger8";
    case QSSGRenderComponentType::Integer8:
        return "Integer8";
    case QSSGRenderComponentType::UnsignedInteger16:
        return "UnsignedInteger16";
    case QSSGRenderComponentType::Integer16:
        return "Integer16";
    case QSSGRenderComponentType::UnsignedInteger32:
        return "UnsignedInteger32";
    case QSSGRenderComponentType::Integer32:
        return "Integer32";
    case QSSGRenderComponentType::UnsignedInteger64:
        return "UnsignedInteger64";
    case QSSGRenderComponentType::Integer64:
        return "Integer64";
    case QSSGRenderComponentType::Float16:
        return "Float16";
    case QSSGRenderComponentType::Float32:
        return "Float32";
    case QSSGRenderComponentType::Float64:
        return "Float64";
    default:
        break;
    }
    return "Unknown";
}

inline quint32 getSizeOfType(QSSGRenderComponentType value)
{
    switch (value) {
    case QSSGRenderComponentType::UnsignedInteger8:
        return sizeof(quint8);
    case QSSGRenderComponentType::Integer8:
        return sizeof(qint8);
    case QSSGRenderComponentType::UnsignedInteger16:
        return sizeof(quint16);
    case QSSGRenderComponentType::Integer16:
        return sizeof(qint16);
    case QSSGRenderComponentType::UnsignedInteger32:
        return sizeof(quint32);
    case QSSGRenderComponentType::Integer32:
        return sizeof(qint32);
    case QSSGRenderComponentType::UnsignedInteger64:
        return sizeof(quint64);
    case QSSGRenderComponentType::Integer64:
        return sizeof(qint64);
    case QSSGRenderComponentType::Float16:
        return sizeof(qfloat16);
    case QSSGRenderComponentType::Float32:
        return sizeof(float);
    case QSSGRenderComponentType::Float64:
        return sizeof(double);
    default:
        break;
    }
    Q_ASSERT(false);
    return 0;
}

enum class QSSGRenderInputAttribute : quint32
{
    Position                = 0x00000001,
    Normal                  = 0x00000002,
    Tangent                 = 0x00000004,
    Binormal                = 0x00000008,
    TexCoords0              = 0x00000010,
    TexCoords1              = 0x00000020,
    Color                   = 0x00000040,
    TargetPosition0         = 0x00000080,
    TargetPosition1         = 0x00000100,
    TargetPosition2         = 0x00000200,
    TargetPosition3         = 0x00000400,
    TargetPosition4         = 0x00000800,
    TargetPosition5         = 0x00001000,
    TargetPosition6         = 0x00002000,
    TargetPosition7         = 0x00004000,
    TargetNormal0           = 0x00008000,
    TargetNormal1           = 0x00010000,
    TargetNormal2           = 0x00020000,
    TargetNormal3           = 0x00040000,
    TargetTangent0          = 0x00080000,
    TargetTangent1          = 0x00100000,
    TargetBinormal0         = 0x00800000,
    TargetBinormal1         = 0x01000000
};
Q_DECLARE_FLAGS(QSSGRenderInputAttributes, QSSGRenderInputAttribute);

struct QSSGRenderTextureFormat
{
    enum Format : quint8 {
        Unknown = 0,
        R8,
        R16,
        R16F,
        R32I,
        R32UI,
        R32F,
        RG8,
        RGBA8,
        RGB8,
        SRGB8,
        SRGB8A8,
        RGB565,
        RGBA5551,
        Alpha8,
        Luminance8,
        Luminance16,
        LuminanceAlpha8,
        RGBA16F,
        RG16F,
        RG32F,
        RGB32F,
        RGBA32F,
        R11G11B10,
        RGB9E5,
        RGB10_A2,
        RGB16F,
        RGBA32UI,
        RGB32UI,
        RGBA16UI,
        RGB16UI,
        RGBA8UI,
        RGB8UI,
        RGBA32I,
        RGB32I,
        RGBA16I,
        RGB16I,
        RGBA8I,
        RGB8I,
        RGBE8,
        RGBA_DXT1,
        RGB_DXT1,
        RGBA_DXT3,
        RGBA_DXT5,
        R11_EAC_UNorm,
        R11_EAC_SNorm,
        RG11_EAC_UNorm,
        RG11_EAC_SNorm,
        RGB8_ETC2,
        SRGB8_ETC2,
        RGB8_PunchThrough_Alpha1_ETC2,
        SRGB8_PunchThrough_Alpha1_ETC2,
        RGBA8_ETC2_EAC,
        SRGB8_Alpha8_ETC2_EAC,
        RGBA_ASTC_4x4,
        RGBA_ASTC_5x4,
        RGBA_ASTC_5x5,
        RGBA_ASTC_6x5,
        RGBA_ASTC_6x6,
        RGBA_ASTC_8x5,
        RGBA_ASTC_8x6,
        RGBA_ASTC_8x8,
        RGBA_ASTC_10x5,
        RGBA_ASTC_10x6,
        RGBA_ASTC_10x8,
        RGBA_ASTC_10x10,
        RGBA_ASTC_12x10,
        RGBA_ASTC_12x12,
        SRGB8_Alpha8_ASTC_4x4,
        SRGB8_Alpha8_ASTC_5x4,
        SRGB8_Alpha8_ASTC_5x5,
        SRGB8_Alpha8_ASTC_6x5,
        SRGB8_Alpha8_ASTC_6x6,
        SRGB8_Alpha8_ASTC_8x5,
        SRGB8_Alpha8_ASTC_8x6,
        SRGB8_Alpha8_ASTC_8x8,
        SRGB8_Alpha8_ASTC_10x5,
        SRGB8_Alpha8_ASTC_10x6,
        SRGB8_Alpha8_ASTC_10x8,
        SRGB8_Alpha8_ASTC_10x10,
        SRGB8_Alpha8_ASTC_12x10,
        SRGB8_Alpha8_ASTC_12x12,
        BC1,
        BC2,
        BC3,
        BC4,
        BC5,
        BC6H,
        BC7,
        Depth16,
        Depth24,
        Depth32,
        Depth24Stencil8
    };
    Format format;

    constexpr QSSGRenderTextureFormat(Format f) : format(f) {}

    bool isUncompressedTextureFormat() const
    {
        switch (format) {
        case QSSGRenderTextureFormat::R8:
            return true;
        case QSSGRenderTextureFormat::R16:
            return true;
        case QSSGRenderTextureFormat::R16F:
            return true;
        case QSSGRenderTextureFormat::R32I:
            return true;
        case QSSGRenderTextureFormat::R32UI:
            return true;
        case QSSGRenderTextureFormat::R32F:
            return true;
        case QSSGRenderTextureFormat::RG8:
            return true;
        case QSSGRenderTextureFormat::RGBA8:
            return true;
        case QSSGRenderTextureFormat::RGB8:
            return true;
        case QSSGRenderTextureFormat::SRGB8:
            return true;
        case QSSGRenderTextureFormat::SRGB8A8:
            return true;
        case QSSGRenderTextureFormat::RGB565:
            return true;
        case QSSGRenderTextureFormat::RGBA5551:
            return true;
        case QSSGRenderTextureFormat::Alpha8:
            return true;
        case QSSGRenderTextureFormat::Luminance8:
            return true;
        case QSSGRenderTextureFormat::Luminance16:
            return true;
        case QSSGRenderTextureFormat::LuminanceAlpha8:
            return true;
        case QSSGRenderTextureFormat::RGBA16F:
            return true;
        case QSSGRenderTextureFormat::RG16F:
            return true;
        case QSSGRenderTextureFormat::RG32F:
            return true;
        case QSSGRenderTextureFormat::RGB32F:
            return true;
        case QSSGRenderTextureFormat::RGBA32F:
            return true;
        case QSSGRenderTextureFormat::R11G11B10:
            return true;
        case QSSGRenderTextureFormat::RGB9E5:
            return true;
        case QSSGRenderTextureFormat::RGB10_A2:
            return true;
        case QSSGRenderTextureFormat::RGB16F:
            return true;
        case QSSGRenderTextureFormat::RGBA32UI:
            return true;
        case QSSGRenderTextureFormat::RGB32UI:
            return true;
        case QSSGRenderTextureFormat::RGBA16UI:
            return true;
        case QSSGRenderTextureFormat::RGB16UI:
            return true;
        case QSSGRenderTextureFormat::RGBA8UI:
            return true;
        case QSSGRenderTextureFormat::RGB8UI:
            return true;
        case QSSGRenderTextureFormat::RGBA32I:
            return true;
        case QSSGRenderTextureFormat::RGB32I:
            return true;
        case QSSGRenderTextureFormat::RGBA16I:
            return true;
        case QSSGRenderTextureFormat::RGB16I:
            return true;
        case QSSGRenderTextureFormat::RGBA8I:
            return true;
        case QSSGRenderTextureFormat::RGB8I:
            return true;
        case QSSGRenderTextureFormat::RGBE8:
            return true;
        default:
            break;
        }
        return false;
    }

    bool isCompressedTextureFormat() const
    {
        switch (format) {
        case QSSGRenderTextureFormat::RGBA_DXT1:
        case QSSGRenderTextureFormat::RGB_DXT1:
        case QSSGRenderTextureFormat::RGBA_DXT3:
        case QSSGRenderTextureFormat::RGBA_DXT5:
        case QSSGRenderTextureFormat::R11_EAC_UNorm:
        case QSSGRenderTextureFormat::R11_EAC_SNorm:
        case QSSGRenderTextureFormat::RG11_EAC_UNorm:
        case QSSGRenderTextureFormat::RG11_EAC_SNorm:
        case QSSGRenderTextureFormat::RGB8_ETC2:
        case QSSGRenderTextureFormat::SRGB8_ETC2:
        case QSSGRenderTextureFormat::RGB8_PunchThrough_Alpha1_ETC2:
        case QSSGRenderTextureFormat::SRGB8_PunchThrough_Alpha1_ETC2:
        case QSSGRenderTextureFormat::RGBA8_ETC2_EAC:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ETC2_EAC:
        case QSSGRenderTextureFormat::RGBA_ASTC_4x4:
        case QSSGRenderTextureFormat::RGBA_ASTC_5x4:
        case QSSGRenderTextureFormat::RGBA_ASTC_5x5:
        case QSSGRenderTextureFormat::RGBA_ASTC_6x5:
        case QSSGRenderTextureFormat::RGBA_ASTC_6x6:
        case QSSGRenderTextureFormat::RGBA_ASTC_8x5:
        case QSSGRenderTextureFormat::RGBA_ASTC_8x6:
        case QSSGRenderTextureFormat::RGBA_ASTC_8x8:
        case QSSGRenderTextureFormat::RGBA_ASTC_10x5:
        case QSSGRenderTextureFormat::RGBA_ASTC_10x6:
        case QSSGRenderTextureFormat::RGBA_ASTC_10x8:
        case QSSGRenderTextureFormat::RGBA_ASTC_10x10:
        case QSSGRenderTextureFormat::RGBA_ASTC_12x10:
        case QSSGRenderTextureFormat::RGBA_ASTC_12x12:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_4x4:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_5x4:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_5x5:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_6x5:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_6x6:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x5:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x6:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x8:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x5:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x6:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x8:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x10:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_12x10:
        case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_12x12:
        case QSSGRenderTextureFormat::BC1:
        case QSSGRenderTextureFormat::BC2:
        case QSSGRenderTextureFormat::BC3:
        case QSSGRenderTextureFormat::BC4:
        case QSSGRenderTextureFormat::BC5:
        case QSSGRenderTextureFormat::BC6H:
        case QSSGRenderTextureFormat::BC7:
            return true;
        default:
            break;
        }
        return false;
    }

    bool isDepthTextureFormat() const
    {
        switch (format) {
        case QSSGRenderTextureFormat::Depth16:
            return true;
        case QSSGRenderTextureFormat::Depth24:
            return true;
        case QSSGRenderTextureFormat::Depth32:
            return true;
        case QSSGRenderTextureFormat::Depth24Stencil8:
            return true;
        default:
            break;
        }
        return false;
    }

    const char *toString() const
    {
        switch (format) {
        case R8:
            return "R8";
        case R16:
            return "R16";
        case R16F:
            return "R16F";
        case R32I:
            return "R32I";
        case R32UI:
            return "R32UI";
        case R32F:
            return "R32F";
        case RG8:
            return "RG8";
        case RGBA8:
            return "RGBA8";
        case RGB8:
            return "RGB8";
        case SRGB8:
            return "SRGB8";
        case SRGB8A8:
            return "SRGB8A8";
        case RGB565:
            return "RGB565";
        case RGBA5551:
            return "RGBA5551";
        case Alpha8:
            return "Alpha8";
        case Luminance8:
            return "Luminance8";
        case Luminance16:
            return "Luminance16";
        case LuminanceAlpha8:
            return "LuminanceAlpha8";
        case RGBA16F:
            return "RGBA16F";
        case RG16F:
            return "RG16F";
        case RG32F:
            return "RG32F";
        case RGB32F:
            return "RGB32F";
        case RGBA32F:
            return "RGBA32F";
        case R11G11B10:
            return "R11G11B10";
        case RGB9E5:
            return "RGB9E5";
        case RGBE8:
            return "RGBE8";
        case RGBA_DXT1:
            return "RGBA_DXT1";
        case RGB_DXT1:
            return "RGB_DXT1";
        case RGBA_DXT3:
            return "RGBA_DXT3";
        case RGBA_DXT5:
            return "RGBA_DXT5";
        case R11_EAC_UNorm:
            return "R11_EAC_UNorm";
        case R11_EAC_SNorm:
            return "R11_EAC_SNorm";
        case RG11_EAC_UNorm:
            return "RG11_EAC_UNorm";
        case RG11_EAC_SNorm:
            return "RG11_EAC_SNorm";
        case RGB8_ETC2:
            return "RGB8_ETC2";
        case SRGB8_ETC2:
            return "SRGB8_ETC2";
        case RGB8_PunchThrough_Alpha1_ETC2:
            return "RGB8_PunchThrough_Alpha1_ETC2";
        case SRGB8_PunchThrough_Alpha1_ETC2:
            return "SRGB8_PunchThrough_Alpha1_ETC2";
        case RGBA8_ETC2_EAC:
            return "RGBA8_ETC2_EAC";
        case SRGB8_Alpha8_ETC2_EAC:
            return "SRGB8_Alpha8_ETC2_EAC";
        case RGBA_ASTC_4x4:
            return "RGBA_ASTC_4x4";
        case RGBA_ASTC_5x4:
            return "RGBA_ASTC_5x4";
        case RGBA_ASTC_5x5:
            return "RGBA_ASTC_5x5";
        case RGBA_ASTC_6x5:
            return "RGBA_ASTC_6x5";
        case RGBA_ASTC_6x6:
            return "RGBA_ASTC_6x6";
        case RGBA_ASTC_8x5:
            return "RGBA_ASTC_8x5";
        case RGBA_ASTC_8x6:
            return "RGBA_ASTC_8x6";
        case RGBA_ASTC_8x8:
            return "RGBA_ASTC_8x8";
        case RGBA_ASTC_10x5:
            return "RGBA_ASTC_10x5";
        case RGBA_ASTC_10x6:
            return "RGBA_ASTC_10x6";
        case RGBA_ASTC_10x8:
            return "RGBA_ASTC_10x8";
        case RGBA_ASTC_10x10:
            return "RGBA_ASTC_10x10";
        case RGBA_ASTC_12x10:
            return "RGBA_ASTC_12x10";
        case RGBA_ASTC_12x12:
            return "RGBA_ASTC_12x12";
        case SRGB8_Alpha8_ASTC_4x4:
            return "SRGB8_Alpha8_ASTC_4x4";
        case SRGB8_Alpha8_ASTC_5x4:
            return "SRGB8_Alpha8_ASTC_5x4";
        case SRGB8_Alpha8_ASTC_5x5:
            return "SRGB8_Alpha8_ASTC_5x5";
        case SRGB8_Alpha8_ASTC_6x5:
            return "SRGB8_Alpha8_ASTC_6x5";
        case SRGB8_Alpha8_ASTC_6x6:
            return "SRGB8_Alpha8_ASTC_6x6";
        case SRGB8_Alpha8_ASTC_8x5:
            return "SRGB8_Alpha8_ASTC_8x5";
        case SRGB8_Alpha8_ASTC_8x6:
            return "SRGB8_Alpha8_ASTC_8x6";
        case SRGB8_Alpha8_ASTC_8x8:
            return "SRGB8_Alpha8_ASTC_8x8";
        case SRGB8_Alpha8_ASTC_10x5:
            return "SRGB8_Alpha8_ASTC_10x5";
        case SRGB8_Alpha8_ASTC_10x6:
            return "SRGB8_Alpha8_ASTC_10x6";
        case SRGB8_Alpha8_ASTC_10x8:
            return "SRGB8_Alpha8_ASTC_10x8";
        case SRGB8_Alpha8_ASTC_10x10:
            return "SRGB8_Alpha8_ASTC_10x10";
        case SRGB8_Alpha8_ASTC_12x10:
            return "SRGB8_Alpha8_ASTC_12x10";
        case SRGB8_Alpha8_ASTC_12x12:
            return "SRGB8_Alpha8_ASTC_12x12";
        case BC1:
            return "BC1";
        case BC2:
            return "BC2";
        case BC3:
            return "BC3";
        case BC4:
            return "BC4";
        case BC5:
            return "BC5";
        case BC6H:
            return "BC6H";
        case BC7:
            return "BC7";
        case Depth16:
            return "Depth16";
        case Depth24:
            return "Depth24";
        case Depth32:
            return "Depth32";
        case Depth24Stencil8:
            return "Depth24Stencil8";
        default:
            break;
        }
        return "Unknown";
    }

    qint32 getSizeofFormat() const
    {
        switch (format) {
        case R8:
            return 1;
        case R16F:
            return 2;
        case R16:
            return 2;
        case R32I:
            return 4;
        case R32F:
            return 4;
        case RGBE8:
        case RGBA8:
            return 4;
        case RGB8:
            return 3;
        case RGB565:
            return 2;
        case RGBA5551:
            return 2;
        case Alpha8:
            return 1;
        case Luminance8:
            return 1;
        case LuminanceAlpha8:
            return 1;
        case Depth16:
            return 2;
        case Depth24:
            return 3;
        case Depth32:
            return 4;
        case Depth24Stencil8:
            return 4;
        case RGB9E5:
            return 4;
        case SRGB8:
            return 3;
        case SRGB8A8:
            return 4;
        case RGBA16F:
            return 8;
        case RG16F:
            return 4;
        case RG32F:
            return 8;
        case RGBA32F:
            return 16;
        case RGB32F:
            return 12;
        case R11G11B10:
            return 4;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    qint32 getNumberOfComponent() const
    {
        switch (format) {
        case R8:
            return 1;
        case R16F:
            return 1;
        case R16:
            return 1;
        case R32I:
            return 1;
        case R32F:
            return 1;
        case RGBA8:
            return 4;
        case RGB8:
            return 3;
        case RGB565:
            return 3;
        case RGBA5551:
            return 4;
        case Alpha8:
            return 1;
        case Luminance8:
            return 1;
        case LuminanceAlpha8:
            return 2;
        case Depth16:
            return 1;
        case Depth24:
            return 1;
        case Depth32:
            return 1;
        case Depth24Stencil8:
            return 2;
        case RGB9E5:
            return 3;
        case SRGB8:
            return 3;
        case SRGB8A8:
            return 4;
        case RGBA16F:
            return 4;
        case RG16F:
            return 2;
        case RG32F:
            return 2;
        case RGBA32F:
            return 4;
        case RGB32F:
            return 3;
        case R11G11B10:
            return 3;
        case RGBE8:
            return 4;
        default:
            break;
        }
        Q_ASSERT(false);
        return 0;
    }

    struct M8E8
    {
        quint8 m;
        quint8 e;
        M8E8() : m(0), e(0){
        }
        M8E8(const float val) {
            float l2 = 1.f + std::floor(log2f(val));
            float mm = val / powf(2.f, l2);
            m = quint8(mm * 255.f);
            e = quint8(l2 + 128);
        }
        M8E8(const float val, quint8 exp) {
            if (val <= 0) {
                m = e = 0;
                return;
            }
            float mm = val / powf(2.f, exp - 128);
            m = quint8(mm * 255.f);
            e = exp;
        }
    };

    void decodeToFloat(void *inPtr, qint32 byteOfs, float *outPtr) const
    {
        Q_ASSERT(byteOfs >= 0);
           outPtr[0] = 0.0f;
           outPtr[1] = 0.0f;
           outPtr[2] = 0.0f;
           outPtr[3] = 0.0f;
           quint8 *src = reinterpret_cast<quint8 *>(inPtr);
           switch (format) {
           case Alpha8:
               outPtr[0] = (float(src[byteOfs])) / 255.0f;
               break;

           case Luminance8:
           case LuminanceAlpha8:
           case R8:
           case RG8:
           case RGB8:
           case RGBA8:
           case SRGB8:
           case SRGB8A8:
               for (qint32 i = 0; i < getSizeofFormat(); ++i) {
                   float val = (float(src[byteOfs + i])) / 255.0f;
                   outPtr[i] = (i < 3) ? std::pow(val, 0.4545454545f) : val;
               }
               break;
           case RGBE8:
               {
                   float pwd = powf(2.0f, int(src[byteOfs + 3]) - 128);
                   outPtr[0] = float(src[byteOfs + 0]) * pwd / 255.0;
                   outPtr[1] = float(src[byteOfs + 1]) * pwd / 255.0;
                   outPtr[2] = float(src[byteOfs + 2]) * pwd / 255.0;
                   outPtr[3] = 1.0f;
               } break;

           case R32F:
               outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
               break;
           case RG32F:
               outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
               outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
               break;
           case RGBA32F:
               outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
               outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
               outPtr[2] = reinterpret_cast<float *>(src + byteOfs)[2];
               outPtr[3] = reinterpret_cast<float *>(src + byteOfs)[3];
               break;
           case RGB32F:
               outPtr[0] = reinterpret_cast<float *>(src + byteOfs)[0];
               outPtr[1] = reinterpret_cast<float *>(src + byteOfs)[1];
               outPtr[2] = reinterpret_cast<float *>(src + byteOfs)[2];
               break;

           case R16F:
           case RG16F:
           case RGBA16F:
               for (qint32 i = 0; i < (getSizeofFormat() >> 1); ++i) {
                   // NOTE : This only works on the assumption that we don't have any denormals,
                   // Infs or NaNs.
                   // Every pixel in our source image should be "regular"
                   quint16 h = reinterpret_cast<quint16 *>(src + byteOfs)[i];
                   quint32 sign = (h & 0x8000u) << 16u;
                   quint32 exponent = (((((h & 0x7c00u) >> 10) - 15) + 127) << 23);
                   quint32 mantissa = ((h & 0x3ffu) << 13);
                   quint32 result = sign | exponent | mantissa;

                   if (h == 0 || h == 0x8000)
                       result = 0;
                   memcpy(outPtr + i, &result, 4);
               }
               break;

           case R11G11B10:
               // place holder
               Q_ASSERT(false);
               break;

           default:
               outPtr[0] = 0.0f;
               outPtr[1] = 0.0f;
               outPtr[2] = 0.0f;
               outPtr[3] = 0.0f;
               break;
           }
    }
    void encodeToPixel(float *inPtr, void *outPtr, qint32 byteOfs) const
    {
        Q_ASSERT(byteOfs >= 0);
            quint8 *dest = reinterpret_cast<quint8 *>(outPtr);
            switch (format) {
            case QSSGRenderTextureFormat::Alpha8:
                dest[byteOfs] = quint8(inPtr[0] * 255.0f);
                break;

            case Luminance8:
            case LuminanceAlpha8:
            case R8:
            case RG8:
            case RGB8:
            case RGBA8:
            case SRGB8:
            case SRGB8A8:
                for (qint32 i = 0; i < getSizeofFormat(); ++i) {
                    inPtr[i] = (inPtr[i] > 1.0f) ? 1.0f : inPtr[i];
                    if (i < 3)
                        dest[byteOfs + i] = quint8(powf(inPtr[i], 2.2f) * 255.0f);
                    else
                        dest[byteOfs + i] = quint8(inPtr[i] * 255.0f);
                }
                break;
            case RGBE8:
            {
                float max = qMax(inPtr[0], qMax(inPtr[1], inPtr[2]));
                M8E8 ex(max);
                M8E8 a(inPtr[0], ex.e);
                M8E8 b(inPtr[1], ex.e);
                M8E8 c(inPtr[2], ex.e);
                quint8 *dst = reinterpret_cast<quint8 *>(outPtr) + byteOfs;
                dst[0] = a.m;
                dst[1] = b.m;
                dst[2] = c.m;
                dst[3] = ex.e;
            } break;

            case R32F:
                reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
                break;
            case RG32F:
                reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
                reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
                break;
            case RGBA32F:
                reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
                reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
                reinterpret_cast<float *>(dest + byteOfs)[2] = inPtr[2];
                reinterpret_cast<float *>(dest + byteOfs)[3] = inPtr[3];
                break;
            case RGB32F:
                reinterpret_cast<float *>(dest + byteOfs)[0] = inPtr[0];
                reinterpret_cast<float *>(dest + byteOfs)[1] = inPtr[1];
                reinterpret_cast<float *>(dest + byteOfs)[2] = inPtr[2];
                break;

            case R16F:
            case RG16F:
            case RGBA16F:
                for (qint32 i = 0; i < (getSizeofFormat() >> 1); ++i) {
                    // NOTE : This also has the limitation of not handling  infs, NaNs and
                    // denormals, but it should be
                    // sufficient for our purposes.
                    if (inPtr[i] > 65519.0f)
                        inPtr[i] = 65519.0f;
                    if (std::fabs(inPtr[i]) < 6.10352E-5f)
                        inPtr[i] = 0.0f;
                    quint32 f = reinterpret_cast<quint32 *>(inPtr)[i];
                    quint32 sign = (f & 0x80000000) >> 16;
                    qint32 exponent = (f & 0x7f800000) >> 23;
                    quint32 mantissa = (f >> 13) & 0x3ff;
                    exponent = exponent - 112;
                    if (exponent > 31)
                        exponent = 31;
                    if (exponent < 0)
                        exponent = 0;
                    exponent = exponent << 10;
                    reinterpret_cast<quint16 *>(dest + byteOfs)[i] = quint16(sign | quint32(exponent) | mantissa);
                }
                break;

            case R11G11B10:
                // place holder
                Q_ASSERT(false);
                break;

            default:
                dest[byteOfs] = 0;
                dest[byteOfs + 1] = 0;
                dest[byteOfs + 2] = 0;
                dest[byteOfs + 3] = 0;
                break;
            }
    }

    bool operator==(const QSSGRenderTextureFormat &other) const { return format == other.format; }
    bool operator!=(const QSSGRenderTextureFormat &other) const { return format != other.format; }
};

inline const char *toString(QSSGRenderTextureFormat::Format value)
{
    return QSSGRenderTextureFormat(value).toString();
}

enum class QSSGRenderTextureSwizzleMode
{
    NoSwizzle = 0,
    L8toR8,
    A8toR8,
    L8A8toRG8,
    L16toR16
};

enum class QSSGRenderTextureFilterOp
{
    None = 0,
    Nearest,
    Linear
};

inline const char *toString(QSSGRenderTextureFilterOp value)
{
    switch (value) {
    case QSSGRenderTextureFilterOp::Nearest:
        return "Nearest";
    case QSSGRenderTextureFilterOp::Linear:
        return "Linear";
    default:
        break;
    }
    return "Unknown";
}

enum class QSSGRenderTextureCoordOp : quint8
{
    Unknown = 0,
    ClampToEdge,
    MirroredRepeat,
    Repeat
};

inline const char *toString(QSSGRenderTextureCoordOp value)
{
    switch (value) {
    case QSSGRenderTextureCoordOp::ClampToEdge:
        return "ClampToEdge";
    case QSSGRenderTextureCoordOp::MirroredRepeat:
        return "MirroredRepeat";
    case QSSGRenderTextureCoordOp::Repeat:
        return "Repeat";
    default:
        break;
    }
    return "Unknown";
}

struct QSSGRenderVertexBufferEntry
{
    QByteArray m_name;
    /** Datatype of the this entry points to in the buffer */
    QSSGRenderComponentType m_componentType;
    /** Number of components of each data member. 1,2,3, or 4.  Don't be stupid.*/
    quint32 m_numComponents;
    /** Offset from the beginning of the buffer of the first item */
    quint32 m_firstItemOffset;

    QSSGRenderVertexBufferEntry(const QByteArray &nm,
                                QSSGRenderComponentType type,
                                quint32 numComponents,
                                quint32 firstItemOffset = 0)
        : m_name(nm), m_componentType(type), m_numComponents(numComponents), m_firstItemOffset(firstItemOffset)
    {
    }

    QSSGRenderVertexBufferEntry()
        : m_componentType(QSSGRenderComponentType::Float32), m_numComponents(0), m_firstItemOffset(0)
    {
    }

    QSSGRenderVertexBufferEntry(const QSSGRenderVertexBufferEntry &inOther)
        : m_name(inOther.m_name)
        , m_componentType(inOther.m_componentType)
        , m_numComponents(inOther.m_numComponents)
        , m_firstItemOffset(inOther.m_firstItemOffset)
    {
    }

    QSSGRenderVertexBufferEntry &operator=(const QSSGRenderVertexBufferEntry &inOther)
    {
        if (this != &inOther) {
            m_name = inOther.m_name;
            m_componentType = inOther.m_componentType;
            m_numComponents = inOther.m_numComponents;
            m_firstItemOffset = inOther.m_firstItemOffset;
        }
        return *this;
    }
};

enum class QSSGRenderDrawMode // stored in mesh files, the values must not change, must match Mesh::DrawMode
{
    Points = 1,
    LineStrip,
    LineLoop,
    Lines,
    TriangleStrip,
    TriangleFan,
    Triangles
};

enum class QSSGRenderWinding // stored in mesh files, the values must not change, must match Mesh::Winding
{
    Clockwise = 1,
    CounterClockwise
};

inline const char *toString(QSSGRenderWinding value)
{
    switch (value) {
    case QSSGRenderWinding::Clockwise:
        return "Clockwise";
    case QSSGRenderWinding::CounterClockwise:
        return "CounterClockwise";
    default:
        break;
    }
    return "Unknown";
}

enum class QSSGCullFaceMode
{
    Unknown = 0,
    Back,
    Front,
    Disabled,
    FrontAndBack, // Not exposed in the front-end
};

inline const char *toString(QSSGCullFaceMode value)
{
    switch (value) {
    case QSSGCullFaceMode::Front:
        return "Front";
    case QSSGCullFaceMode::Back:
        return "Back";
    case QSSGCullFaceMode::FrontAndBack:
        return "FrontAndBack";
    default:
        break;
    }
    return "Unknown";
}

enum class QSSGDepthDrawMode
{
    OpaqueOnly,
    Always,
    Never,
    OpaquePrePass
};

inline const char *toString(QSSGDepthDrawMode value)
{
    switch (value) {
    case QSSGDepthDrawMode::OpaqueOnly:
        return "OpaqueOnly";
    case QSSGDepthDrawMode::Always:
        return "Always";
    case QSSGDepthDrawMode::Never:
        return "Never";
    case QSSGDepthDrawMode::OpaquePrePass:
        return "OpaquePrePass";
    }
    return "Unknown";
}

// Return coordinates in pixels but relative to this rect.
inline QVector2D toRectRelative(const QRectF &r, const QVector2D &absoluteCoordinates)
{
    return QVector2D(absoluteCoordinates.x() - float(r.x()), absoluteCoordinates.y() - float(r.y()));
}

inline QVector2D halfDims(const QRectF &r)
{
    return QVector2D(float(r.width() / 2.0), float(r.height() / 2.0));
}

// Take coordinates in global space and move local space where 0,0 is the center
// of the rect but return value in pixels, not in normalized -1,1 range
inline QVector2D toNormalizedRectRelative(const QRectF &r, QVector2D absoluteCoordinates)
{
    // normalize them
    const QVector2D relativeCoords(toRectRelative(r, absoluteCoordinates));
    const QVector2D halfD(halfDims(r));
    const QVector2D normalized((relativeCoords.x() / halfD.x()) - 1.0f, (relativeCoords.y() / halfD.y()) - 1.0f);
    return QVector2D(normalized.x() * halfD.x(), normalized.y() * halfD.y());
}

inline QVector2D relativeToNormalizedCoordinates(const QRectF &r, QVector2D rectRelativeCoords)
{
    return { (rectRelativeCoords.x() / halfDims(r).x()) - 1.0f, (rectRelativeCoords.y() / halfDims(r).y()) - 1.0f };
}

// Normalized coordinates are in the range of -1,1 where -1 is the left, bottom edges
// and 1 is the top,right edges.
inline QVector2D absoluteToNormalizedCoordinates(const QRectF &r, const QVector2D &absoluteCoordinates)
{
    return relativeToNormalizedCoordinates(r, toRectRelative(r, absoluteCoordinates));
}

inline QVector2D toAbsoluteCoords(const QRectF &r, const QVector2D &inRelativeCoords)
{
    return QVector2D(inRelativeCoords.x() + float(r.x()), inRelativeCoords.y() + float(r.y()));
}

template<typename TDataType>
struct QSSGRenderGenericVec2
{
    TDataType x;
    TDataType y;
    QSSGRenderGenericVec2(TDataType _x, TDataType _y) : x(_x), y(_y) {}
    QSSGRenderGenericVec2() {}
    bool operator==(const QSSGRenderGenericVec2 &inOther) const { return x == inOther.x && y == inOther.y; }
};

template<typename TDataType>
struct QSSGRenderGenericVec3
{
    TDataType x;
    TDataType y;
    TDataType z;
    QSSGRenderGenericVec3(TDataType _x, TDataType _y, TDataType _z) : x(_x), y(_y), z(_z) {}
    QSSGRenderGenericVec3() {}
    bool operator==(const QSSGRenderGenericVec3 &inOther) const
    {
        return x == inOther.x && y == inOther.y && z == inOther.z;
    }
};

template<typename TDataType>
struct QSSGRenderGenericVec4
{
    TDataType x;
    TDataType y;
    TDataType z;
    TDataType w;
    QSSGRenderGenericVec4(TDataType _x, TDataType _y, TDataType _z, TDataType _w) : x(_x), y(_y), z(_z), w(_w) {}
    QSSGRenderGenericVec4() {}
    bool operator==(const QSSGRenderGenericVec4 &inOther) const
    {
        return x == inOther.x && y == inOther.y && z == inOther.z && w == inOther.w;
    }
};

typedef QSSGRenderGenericVec2<bool> bool_2;
typedef QSSGRenderGenericVec3<bool> bool_3;
typedef QSSGRenderGenericVec4<bool> bool_4;
typedef QSSGRenderGenericVec2<quint32> quint32_2;
typedef QSSGRenderGenericVec3<quint32> quint32_3;
typedef QSSGRenderGenericVec4<quint32> quint32_4;
typedef QSSGRenderGenericVec2<qint32> qint32_2;
typedef QSSGRenderGenericVec3<qint32> qint32_3;
typedef QSSGRenderGenericVec4<qint32> qint32_4;

enum class QSSGRenderShaderDataType : quint32
{
    Unknown = 0,
    Integer, // qint32,
    IntegerVec2, // qint32_2,
    IntegerVec3, // qint32_3,
    IntegerVec4, // qint32_4,
    Boolean, // bool
    BooleanVec2, // bool_2,
    BooleanVec3, // bool_3,
    BooleanVec4, // bool_4,
    Float, // float,
    Vec2, // QVector2D,
    Vec3, // QVector3D,
    Vec4, // QVector4D,
    UnsignedInteger, // quint32,
    UnsignedIntegerVec2, // quint32_2,
    UnsignedIntegerVec3, // quint32_3,
    UnsignedIntegerVec4, // quint32_4,
    Matrix3x3, // QMatrix3x3,
    Matrix4x4, // QMatrix4x4,
    Rgba, // QColor
    Size, // QSize
    SizeF, // QSizeF
    Point, // QPoint
    PointF, // QPointF
    Rect, // QRect
    RectF, // QRectF
    Quaternion, // QQuaternion
    Texture,
};

inline const char *toString(QSSGRenderShaderDataType type)
{
    switch (type) {
    case QSSGRenderShaderDataType::Integer: // qint32:
        return "Integer";
    case QSSGRenderShaderDataType::IntegerVec2: // qint32_2:
        return "IntegerVec2";
    case QSSGRenderShaderDataType::IntegerVec3: // qint32_3:
                return "IntegerVec3";
    case QSSGRenderShaderDataType::IntegerVec4: // qint32_4:
        return "IntegerVec4";
    case QSSGRenderShaderDataType::Boolean: // bool
        return "Boolean";
    case QSSGRenderShaderDataType::BooleanVec2: // bool_2:
        return "BooleanVec2";
    case QSSGRenderShaderDataType::BooleanVec3: // bool_3:
        return "BooleanVec3";
    case QSSGRenderShaderDataType::BooleanVec4: // bool_4:
        return "BooleanVec4";
    case QSSGRenderShaderDataType::Float: // float:
        return "Float";
    case QSSGRenderShaderDataType::Vec2: // QVector2D:
        return "Vec2";
    case QSSGRenderShaderDataType::Vec3: // QVector3D:
        return "Vec3";
    case QSSGRenderShaderDataType::Vec4: // QVector4D:
        return "Vec4";
    case QSSGRenderShaderDataType::UnsignedInteger: // quint32:
        return "UnsignedInteger";
    case QSSGRenderShaderDataType::UnsignedIntegerVec2: // quint32_2:
        return "UnsignedIntegerVec2";
    case QSSGRenderShaderDataType::UnsignedIntegerVec3: // quint32_3:
        return "UnsignedIntegerVec3";
    case QSSGRenderShaderDataType::UnsignedIntegerVec4: // quint32_4:
        return "UnsignedIntegerVec4";
    case QSSGRenderShaderDataType::Matrix3x3: // QMatrix3x3:
        return "Matrix3x3";
    case QSSGRenderShaderDataType::Matrix4x4: // QMatrix4x4:
        return "Matrix4x4";
    case QSSGRenderShaderDataType::Rgba: // QColor
        return "Rgba";
    case QSSGRenderShaderDataType::Size:
        return "Size";
    case QSSGRenderShaderDataType::SizeF:
        return "SizeF";
    case QSSGRenderShaderDataType::Point:
        return "Point";
    case QSSGRenderShaderDataType::PointF:
        return "PointF";
    case QSSGRenderShaderDataType::Rect:
        return "Rect";
    case QSSGRenderShaderDataType::RectF:
        return "RectF";
    case QSSGRenderShaderDataType::Quaternion:
        return "Quaternion";
    case QSSGRenderShaderDataType::Texture:
        return "Texture";

    case QSSGRenderShaderDataType::Unknown:
    default:
        return "Unknown";
    }
}

enum class QSSGRenderTextureTypeValue
{
    Unknown = 0,
    Diffuse,
    Specular,
    Environment,
    Bump,
    Normal,
    Emissive,
    Anisotropy,
    Translucent
};

inline const char *toString(QSSGRenderTextureTypeValue value)
{
    switch (value) {
    case QSSGRenderTextureTypeValue::Unknown:
        return "Unknown";
    case QSSGRenderTextureTypeValue::Diffuse:
        return "Diffuse";
    case QSSGRenderTextureTypeValue::Specular:
        return "Specular";
    case QSSGRenderTextureTypeValue::Environment:
        return "Environment";
    case QSSGRenderTextureTypeValue::Bump:
        return "Bump";
    case QSSGRenderTextureTypeValue::Normal:
        return "Normal";
    case QSSGRenderTextureTypeValue::Emissive:
        return "Emissive";
    case QSSGRenderTextureTypeValue::Anisotropy:
        return "Anisotropy";
    case QSSGRenderTextureTypeValue::Translucent:
        return "Translucent";
    }
    return nullptr;
}

QT_END_NAMESPACE

#endif
