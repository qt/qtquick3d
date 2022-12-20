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

enum class QSSGRenderDrawMode // stored in mesh files, the values must not change, must match Mesh::DrawMode
{
    Points = 1,
    LineStrip,
    LineLoop, // Not supported
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

struct Q_QUICK3DUTILS_PRIVATE_EXPORT QSSGRenderTextureFormat
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

    const char *toString() const;

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

enum class QSSGRenderTextureFilterOp
{
    None = 0,
    Nearest,
    Linear
};

enum class QSSGRenderTextureCoordOp : quint8
{
    Unknown = 0,
    ClampToEdge,
    MirroredRepeat,
    Repeat
};

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

enum class QSSGCullFaceMode
{
    Unknown = 0,
    Back,
    Front,
    Disabled,
    FrontAndBack, // Not exposed in the front-end
};

enum class QSSGDepthDrawMode
{
    OpaqueOnly,
    Always,
    Never,
    OpaquePrePass
};

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

enum class QSSGRenderTextureCubeFace
{
    NegX,
    PosX,
    NegY,
    PosY,
    NegZ,
    PosZ
};

class Q_QUICK3DUTILS_PRIVATE_EXPORT QSSGBaseTypeHelpers
{
    QSSGBaseTypeHelpers() = default;
public:
    // Enum as string
    static const char *toString(QSSGRenderTextureCubeFace value);
    static const char *toString(QSSGRenderTextureTypeValue value);
    static const char *toString(QSSGDepthDrawMode value);
    static const char *toString(QSSGRenderWinding value);
    static const char *toString(QSSGCullFaceMode value);
    static const char *toString(QSSGRenderComponentType value);
    static const char *toString(QSSGRenderTextureFormat::Format value);
    static const char *toString(QSSGRenderTextureCoordOp value);
    static const char *toString(QSSGRenderTextureFilterOp value);

    static size_t getSizeOfType(QSSGRenderComponentType type);
};

QT_END_NAMESPACE

#endif
