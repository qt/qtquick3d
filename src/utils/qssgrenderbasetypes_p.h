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
    UnsignedInt8 = 1,
    Int8,
    UnsignedInt16,
    Int16,
    UnsignedInt32,
    Int32,
    UnsignedInt64,
    Int64,
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
    static constexpr quint8 DepthTextureFlag = 1u << 6;
    static constexpr quint8 CompressedTextureFlag = 1u << 7;

    enum Format : quint8 {
        // Non-compressed formats
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

        // Depth textures
        Depth16 = DepthTextureFlag + 1,
        Depth24,
        Depth32,
        Depth24Stencil8,

        // Compressed formats
        RGBA_DXT1 = CompressedTextureFlag + 1,
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
    };
    Format format;

    constexpr QSSGRenderTextureFormat(Format f) : format(f) {}

    [[nodiscard]] constexpr bool isCompressedTextureFormat() const noexcept
    {
        return (format & CompressedTextureFlag);
    }

    [[nodiscard]] constexpr bool isUncompressedTextureFormat() const noexcept
    {
        return !isCompressedTextureFormat();
    }

    [[nodiscard]] bool isDepthTextureFormat() const noexcept
    {
        return (format & DepthTextureFlag);
    }

    [[nodiscard]] const char *toString() const;

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

    void decodeToFloat(void *inPtr, qint32 byteOfs, float *outPtr) const;
    void encodeToPixel(float *inPtr, void *outPtr, qint32 byteOfs) const;

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

enum class QSSGRenderTextureCubeFace : quint8
{
    PosX,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ
};

// Same order as expected by QRHI!
static constexpr QSSGRenderTextureCubeFace QSSGRenderTextureCubeFaces[] {
    QSSGRenderTextureCubeFace::PosX, QSSGRenderTextureCubeFace::NegX,
    QSSGRenderTextureCubeFace::PosY, QSSGRenderTextureCubeFace::NegY,
    QSSGRenderTextureCubeFace::PosZ, QSSGRenderTextureCubeFace::NegZ
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

    static const char *displayName(QSSGRenderTextureCubeFace face);

    static size_t getSizeOfType(QSSGRenderComponentType type);

    // Note: These will wrap around
    static constexpr QSSGRenderTextureCubeFace next(QSSGRenderTextureCubeFace face)
    { return (face == QSSGRenderTextureCubeFaces[5]) ? QSSGRenderTextureCubeFaces[0] : QSSGRenderTextureCubeFace(quint8(face) + 1); }
    static constexpr QSSGRenderTextureCubeFace prev(QSSGRenderTextureCubeFace face)
    { return (face == QSSGRenderTextureCubeFaces[0]) ? QSSGRenderTextureCubeFaces[5] : QSSGRenderTextureCubeFace(quint8(face) - 1); }
};

QT_END_NAMESPACE

#endif
