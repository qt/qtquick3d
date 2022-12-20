// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgrenderbasetypes_p.h"

QT_BEGIN_NAMESPACE

const char *QSSGRenderTextureFormat::toString() const
{
    switch (format) {
    case QSSGRenderTextureFormat::R8:
        return "R8";
    case QSSGRenderTextureFormat::R16:
        return "R16";
    case QSSGRenderTextureFormat::R16F:
        return "R16F";
    case QSSGRenderTextureFormat::R32I:
        return "R32I";
    case QSSGRenderTextureFormat::R32UI:
        return "R32UI";
    case QSSGRenderTextureFormat::R32F:
        return "R32F";
    case QSSGRenderTextureFormat::RG8:
        return "RG8";
    case QSSGRenderTextureFormat::RGBA8:
        return "RGBA8";
    case QSSGRenderTextureFormat::RGB8:
        return "RGB8";
    case QSSGRenderTextureFormat::SRGB8:
        return "SRGB8";
    case QSSGRenderTextureFormat::SRGB8A8:
        return "SRGB8A8";
    case QSSGRenderTextureFormat::RGB565:
        return "RGB565";
    case QSSGRenderTextureFormat::RGBA5551:
        return "RGBA5551";
    case QSSGRenderTextureFormat::Alpha8:
        return "Alpha8";
    case QSSGRenderTextureFormat::Luminance8:
        return "Luminance8";
    case QSSGRenderTextureFormat::Luminance16:
        return "Luminance16";
    case QSSGRenderTextureFormat::LuminanceAlpha8:
        return "LuminanceAlpha8";
    case QSSGRenderTextureFormat::RGBA16F:
        return "RGBA16F";
    case QSSGRenderTextureFormat::RG16F:
        return "RG16F";
    case QSSGRenderTextureFormat::RG32F:
        return "RG32F";
    case QSSGRenderTextureFormat::RGB32F:
        return "RGB32F";
    case QSSGRenderTextureFormat::RGBA32F:
        return "RGBA32F";
    case QSSGRenderTextureFormat::R11G11B10:
        return "R11G11B10";
    case QSSGRenderTextureFormat::RGB9E5:
        return "RGB9E5";
    case QSSGRenderTextureFormat::RGBE8:
        return "RGBE8";
    case QSSGRenderTextureFormat::RGBA_DXT1:
        return "RGBA_DXT1";
    case QSSGRenderTextureFormat::RGB_DXT1:
        return "RGB_DXT1";
    case QSSGRenderTextureFormat::RGBA_DXT3:
        return "RGBA_DXT3";
    case QSSGRenderTextureFormat::RGBA_DXT5:
        return "RGBA_DXT5";
    case QSSGRenderTextureFormat::R11_EAC_UNorm:
        return "R11_EAC_UNorm";
    case QSSGRenderTextureFormat::R11_EAC_SNorm:
        return "R11_EAC_SNorm";
    case QSSGRenderTextureFormat::RG11_EAC_UNorm:
        return "RG11_EAC_UNorm";
    case QSSGRenderTextureFormat::RG11_EAC_SNorm:
        return "RG11_EAC_SNorm";
    case QSSGRenderTextureFormat::RGB8_ETC2:
        return "RGB8_ETC2";
    case QSSGRenderTextureFormat::SRGB8_ETC2:
        return "SRGB8_ETC2";
    case QSSGRenderTextureFormat::RGB8_PunchThrough_Alpha1_ETC2:
        return "RGB8_PunchThrough_Alpha1_ETC2";
    case QSSGRenderTextureFormat::SRGB8_PunchThrough_Alpha1_ETC2:
        return "SRGB8_PunchThrough_Alpha1_ETC2";
    case QSSGRenderTextureFormat::RGBA8_ETC2_EAC:
        return "RGBA8_ETC2_EAC";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ETC2_EAC:
        return "SRGB8_Alpha8_ETC2_EAC";
    case QSSGRenderTextureFormat::RGBA_ASTC_4x4:
        return "RGBA_ASTC_4x4";
    case QSSGRenderTextureFormat::RGBA_ASTC_5x4:
        return "RGBA_ASTC_5x4";
    case QSSGRenderTextureFormat::RGBA_ASTC_5x5:
        return "RGBA_ASTC_5x5";
    case QSSGRenderTextureFormat::RGBA_ASTC_6x5:
        return "RGBA_ASTC_6x5";
    case QSSGRenderTextureFormat::RGBA_ASTC_6x6:
        return "RGBA_ASTC_6x6";
    case QSSGRenderTextureFormat::RGBA_ASTC_8x5:
        return "RGBA_ASTC_8x5";
    case QSSGRenderTextureFormat::RGBA_ASTC_8x6:
        return "RGBA_ASTC_8x6";
    case QSSGRenderTextureFormat::RGBA_ASTC_8x8:
        return "RGBA_ASTC_8x8";
    case QSSGRenderTextureFormat::RGBA_ASTC_10x5:
        return "RGBA_ASTC_10x5";
    case QSSGRenderTextureFormat::RGBA_ASTC_10x6:
        return "RGBA_ASTC_10x6";
    case QSSGRenderTextureFormat::RGBA_ASTC_10x8:
        return "RGBA_ASTC_10x8";
    case QSSGRenderTextureFormat::RGBA_ASTC_10x10:
        return "RGBA_ASTC_10x10";
    case QSSGRenderTextureFormat::RGBA_ASTC_12x10:
        return "RGBA_ASTC_12x10";
    case QSSGRenderTextureFormat::RGBA_ASTC_12x12:
        return "RGBA_ASTC_12x12";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_4x4:
        return "SRGB8_Alpha8_ASTC_4x4";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_5x4:
        return "SRGB8_Alpha8_ASTC_5x4";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_5x5:
        return "SRGB8_Alpha8_ASTC_5x5";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_6x5:
        return "SRGB8_Alpha8_ASTC_6x5";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_6x6:
        return "SRGB8_Alpha8_ASTC_6x6";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x5:
        return "SRGB8_Alpha8_ASTC_8x5";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x6:
        return "SRGB8_Alpha8_ASTC_8x6";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_8x8:
        return "SRGB8_Alpha8_ASTC_8x8";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x5:
        return "SRGB8_Alpha8_ASTC_10x5";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x6:
        return "SRGB8_Alpha8_ASTC_10x6";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x8:
        return "SRGB8_Alpha8_ASTC_10x8";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_10x10:
        return "SRGB8_Alpha8_ASTC_10x10";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_12x10:
        return "SRGB8_Alpha8_ASTC_12x10";
    case QSSGRenderTextureFormat::SRGB8_Alpha8_ASTC_12x12:
        return "SRGB8_Alpha8_ASTC_12x12";
    case QSSGRenderTextureFormat::BC1:
        return "BC1";
    case QSSGRenderTextureFormat::BC2:
        return "BC2";
    case QSSGRenderTextureFormat::BC3:
        return "BC3";
    case QSSGRenderTextureFormat::BC4:
        return "BC4";
    case QSSGRenderTextureFormat::BC5:
        return "BC5";
    case QSSGRenderTextureFormat::BC6H:
        return "BC6H";
    case QSSGRenderTextureFormat::BC7:
        return "BC7";
    case QSSGRenderTextureFormat::Depth16:
        return "Depth16";
    case QSSGRenderTextureFormat::Depth24:
        return "Depth24";
    case QSSGRenderTextureFormat::Depth32:
        return "Depth32";
    case QSSGRenderTextureFormat::Depth24Stencil8:
        return "Depth24Stencil8";
    case QSSGRenderTextureFormat::RGB10_A2:
        return "RGB10_A2";
    case QSSGRenderTextureFormat::RGB16F:
        return "RGB16F";
    case QSSGRenderTextureFormat::RGBA32UI:
        return "RGBA32UI";
    case QSSGRenderTextureFormat::RGB32UI:
        return "RGB32UI";
    case QSSGRenderTextureFormat::RGBA16UI:
        return "RGBA16UI";
    case QSSGRenderTextureFormat::RGB16UI:
        return "RGB16UI";
    case QSSGRenderTextureFormat::RGBA8UI:
        return "RGBA8UI";
    case QSSGRenderTextureFormat::RGB8UI:
        return "RGB8UI";
    case QSSGRenderTextureFormat::RGBA32I:
        return "RGBA32I";
    case QSSGRenderTextureFormat::RGB32I:
        return "RGB32I";
    case QSSGRenderTextureFormat::RGBA16I:
        return "RGBA16I";
    case QSSGRenderTextureFormat::RGB16I:
        return "RGB16I";
    case QSSGRenderTextureFormat::RGBA8I:
        return "RGBA8I";
    case QSSGRenderTextureFormat::RGB8I:
        return "RGB8I";
    case QSSGRenderTextureFormat::Unknown:
        return "Unknown";
    }

    Q_UNREACHABLE_RETURN(nullptr);
}

const char *QSSGBaseTypeHelpers::toString(QSSGRenderWinding value)
{
    switch (value) {
    case QSSGRenderWinding::Clockwise:
        return "Clockwise";
    case QSSGRenderWinding::CounterClockwise:
        return "CounterClockwise";
    }

    Q_UNREACHABLE_RETURN(nullptr);
}

const char *QSSGBaseTypeHelpers::toString(QSSGCullFaceMode value)
{
    switch (value) {
    case QSSGCullFaceMode::Front:
        return "Front";
    case QSSGCullFaceMode::Back:
        return "Back";
    case QSSGCullFaceMode::FrontAndBack:
        return "FrontAndBack";
    case QSSGCullFaceMode::Unknown:
        return "Unknown";
    case QSSGCullFaceMode::Disabled:
        return "Disabled";
    }

    Q_UNREACHABLE_RETURN(nullptr);
}

const char *QSSGBaseTypeHelpers::toString(QSSGRenderTextureCubeFace value)
{
    switch (value) {
    case QSSGRenderTextureCubeFace::NegX:
        return "NegX";
    case QSSGRenderTextureCubeFace::NegZ:
        return "NegZ";
    case QSSGRenderTextureCubeFace::NegY:
        return "NegY";
    case QSSGRenderTextureCubeFace::PosY:
        return "PosY";
    case QSSGRenderTextureCubeFace::PosX:
        return "PosX";
    case QSSGRenderTextureCubeFace::PosZ:
        return "PosZ";
    }

    Q_UNREACHABLE_RETURN(nullptr);
}

const char *QSSGBaseTypeHelpers::toString(QSSGRenderTextureTypeValue value)
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

    Q_UNREACHABLE_RETURN(nullptr);
}

const char *QSSGBaseTypeHelpers::toString(QSSGDepthDrawMode value)
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

    Q_UNREACHABLE_RETURN(nullptr);
}

const char *QSSGBaseTypeHelpers::toString(QSSGRenderComponentType value)
{
    switch (value) {
    case QSSGRenderComponentType::UnsignedInteger8:
        return "UnsignedInt8";
    case QSSGRenderComponentType::Integer8:
        return "Int8";
    case QSSGRenderComponentType::UnsignedInteger16:
        return "UnsignedInt16";
    case QSSGRenderComponentType::Integer16:
        return "Int16";
    case QSSGRenderComponentType::UnsignedInteger32:
        return "UnsignedInt32";
    case QSSGRenderComponentType::Integer32:
        return "Int32";
    case QSSGRenderComponentType::UnsignedInteger64:
        return "UnsignedInt64";
    case QSSGRenderComponentType::Integer64:
        return "Int64";
    case QSSGRenderComponentType::Float16:
        return "Float16";
    case QSSGRenderComponentType::Float32:
        return "Float32";
    case QSSGRenderComponentType::Float64:
        return "Float64";
    }

    Q_UNREACHABLE_RETURN("Unknown");
}

const char *QSSGBaseTypeHelpers::toString(QSSGRenderTextureFormat::Format value)
{
    return QSSGRenderTextureFormat(value).toString();
}

const char *QSSGBaseTypeHelpers::toString(QSSGRenderTextureCoordOp value)
{
    switch (value) {
    case QSSGRenderTextureCoordOp::ClampToEdge:
        return "ClampToEdge";
    case QSSGRenderTextureCoordOp::MirroredRepeat:
        return "MirroredRepeat";
    case QSSGRenderTextureCoordOp::Repeat:
        return "Repeat";
    case QSSGRenderTextureCoordOp::Unknown:
        return "Unknown";
    }

    Q_UNREACHABLE_RETURN(nullptr);
}

const char *QSSGBaseTypeHelpers::toString(QSSGRenderTextureFilterOp value)
{
    switch (value) {
    case QSSGRenderTextureFilterOp::Nearest:
        return "Nearest";
    case QSSGRenderTextureFilterOp::Linear:
        return "Linear";
    case QSSGRenderTextureFilterOp::None:
        return "None";
    }

    Q_UNREACHABLE_RETURN(nullptr);
}

size_t QSSGBaseTypeHelpers::getSizeOfType(QSSGRenderComponentType type)
{
    switch (type) {
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
    }
    Q_UNREACHABLE_RETURN(0);
}

QT_END_NAMESPACE
