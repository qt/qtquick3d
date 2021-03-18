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

#include "qssgrenderprefiltertexture_p.h"

#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>

#include <cmath>

QT_BEGIN_NAMESPACE

QSSGRenderPrefilterTexture::QSSGRenderPrefilterTexture(const QSSGRef<QSSGRenderContext> &inQSSGRenderContext,
                                                           qint32 inWidth,
                                                           qint32 inHeight,
                                                           const QSSGRef<QSSGRenderTexture2D> &inTexture2D,
                                                           QSSGRenderTextureFormat inDestFormat)
    : m_texture2D(inTexture2D), m_destinationFormat(inDestFormat), m_width(inWidth), m_height(inHeight), m_renderContext(inQSSGRenderContext)
{
    // Calculate mip level
    int maxDim = inWidth >= inHeight ? inWidth : inHeight;

    m_maxMipMapLevel = static_cast<int>(logf((float)maxDim) / logf(2.0f));
    // no concept of sizeOfFormat just does'nt make sense
    m_sizeOfFormat = m_destinationFormat.getSizeofFormat();
    m_noOfComponent = m_destinationFormat.getNumberOfComponent();
}

QSSGRef<QSSGRenderPrefilterTexture> QSSGRenderPrefilterTexture::create(const QSSGRef<QSSGRenderContext> &inQSSGRenderContext,
                                                                             qint32 inWidth,
                                                                             qint32 inHeight,
                                                                             const QSSGRef<QSSGRenderTexture2D> &inTexture2D,
                                                                             QSSGRenderTextureFormat inDestFormat)
{
    QSSGRef<QSSGRenderPrefilterTexture> theBSDFMipMap;

    if (inQSSGRenderContext->supportsCompute()) {
        theBSDFMipMap = new QSSGRenderPrefilterTextureCompute(inQSSGRenderContext, inWidth, inHeight, inTexture2D, inDestFormat);
    }

    if (!theBSDFMipMap) {
        theBSDFMipMap = new QSSGRenderPrefilterTextureCPU(inQSSGRenderContext, inWidth, inHeight, inTexture2D, inDestFormat);
    }

    return theBSDFMipMap;
}

QSSGRenderPrefilterTexture::~QSSGRenderPrefilterTexture() = default;

//------------------------------------------------------------------------------------
// CPU based filtering
//------------------------------------------------------------------------------------

QSSGRenderPrefilterTextureCPU::QSSGRenderPrefilterTextureCPU(const QSSGRef<QSSGRenderContext> &inQSSGRenderContext,
                                                                 int inWidth,
                                                                 int inHeight,
                                                                 const QSSGRef<QSSGRenderTexture2D> &inTexture2D,
                                                                 QSSGRenderTextureFormat inDestFormat)
    : QSSGRenderPrefilterTexture(inQSSGRenderContext, inWidth, inHeight, inTexture2D, inDestFormat)
{
}

inline int QSSGRenderPrefilterTextureCPU::wrapMod(int a, int base)
{
    return (a >= 0) ? a % base : (a % base) + base;
}

inline void QSSGRenderPrefilterTextureCPU::getWrappedCoords(int &sX, int &sY, int width, int height)
{
    if (sY < 0) {
        sX -= width >> 1;
        sY = -sY;
    }
    if (sY >= height) {
        sX += width >> 1;
        sY = height - sY;
    }
    sX = wrapMod(sX, width);
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

void QSSGRenderTextureFormat::decodeToFloat(void *inPtr, qint32 byteOfs, float *outPtr) const
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

void QSSGRenderTextureFormat::encodeToPixel(float *inPtr, void *outPtr, qint32 byteOfs) const
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

QSSGTextureData QSSGRenderPrefilterTextureCPU::createBsdfMipLevel(QSSGTextureData &inCurMipLevel,
                                                                      QSSGTextureData &inPrevMipLevel,
                                                                      int width,
                                                                      int height) //, IPerfTimer& inPerfTimer )
{
    QSSGTextureData retval;
    int newWidth = width >> 1;
    int newHeight = height >> 1;
    newWidth = newWidth >= 1 ? newWidth : 1;
    newHeight = newHeight >= 1 ? newHeight : 1;

    if (inCurMipLevel.data) {
        retval = inCurMipLevel;
        retval.dataSizeInBytes = newWidth * newHeight * inPrevMipLevel.format.getSizeofFormat();
    } else {
        retval.dataSizeInBytes = newWidth * newHeight * inPrevMipLevel.format.getSizeofFormat();
        retval.format = inPrevMipLevel.format; // inLoadedImage.format;
        retval.data = ::malloc(retval.dataSizeInBytes);
    }

    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            float accumVal[4];
            accumVal[0] = 0;
            accumVal[1] = 0;
            accumVal[2] = 0;
            accumVal[3] = 0;
            for (int sy = -2; sy <= 2; ++sy) {
                for (int sx = -2; sx <= 2; ++sx) {
                    int sampleX = sx + (x << 1);
                    int sampleY = sy + (y << 1);
                    getWrappedCoords(sampleX, sampleY, width, height);

                    // Cauchy filter (this is simply because it's the easiest to evaluate, and
                    // requires no complex
                    // functions).
                    float filterPdf = 1.f / (1.f + float(sx * sx + sy * sy) * 2.f);
                    // With FP HDR formats, we're not worried about intensity loss so much as
                    // unnecessary energy gain,
                    // whereas with LDR formats, the fear with a continuous normalization factor is
                    // that we'd lose
                    // intensity and saturation as well.
                    filterPdf /= (retval.format.getSizeofFormat() >= 8) ? 4.71238898f : 4.5403446f;
                    // filterPdf /= 4.5403446f;        // Discrete normalization factor
                    // filterPdf /= 4.71238898f;        // Continuous normalization factor
                    float curPix[4];
                    qint32 byteOffset = (sampleY * width + sampleX) * retval.format.getSizeofFormat();
                    if (byteOffset < 0) {
                        sampleY = height + sampleY;
                        byteOffset = (sampleY * width + sampleX) * retval.format.getSizeofFormat();
                    }

                    retval.format.decodeToFloat(inPrevMipLevel.data, byteOffset, curPix);

                    accumVal[0] += filterPdf * curPix[0];
                    accumVal[1] += filterPdf * curPix[1];
                    accumVal[2] += filterPdf * curPix[2];
                    accumVal[3] += filterPdf * curPix[3];
                }
            }

            quint32 newIdx = (y * newWidth + x) * retval.format.getSizeofFormat();

            retval.format.encodeToPixel(accumVal, retval.data, newIdx);
        }
    }

    return retval;
}

void QSSGRenderPrefilterTextureCPU::build(void *inTextureData, qint32 inTextureDataSize, QSSGRenderTextureFormat inFormat)
{
    m_sizeOfInternalFormat = inFormat.getSizeofFormat();
    m_internalNoOfComponent = inFormat.getNumberOfComponent();

    m_texture2D->setMaxLevel(m_maxMipMapLevel);
    m_texture2D->setTextureData(QSSGByteView((quint8 *)inTextureData, inTextureDataSize), 0, m_width, m_height, inFormat, m_destinationFormat);

    QSSGTextureData theMipImage;
    QSSGTextureData prevImage;
    prevImage.data = inTextureData;
    prevImage.dataSizeInBytes = inTextureDataSize;
    prevImage.format = inFormat;
    int curWidth = m_width;
    int curHeight = m_height;
    int size = inFormat.getSizeofFormat();
    for (int idx = 1; idx <= m_maxMipMapLevel; ++idx) {
        theMipImage = createBsdfMipLevel(theMipImage, prevImage, curWidth, curHeight); //, m_PerfTimer );
        curWidth = curWidth >> 1;
        curHeight = curHeight >> 1;
        curWidth = curWidth >= 1 ? curWidth : 1;
        curHeight = curHeight >= 1 ? curHeight : 1;
        inTextureDataSize = curWidth * curHeight * size;

        m_texture2D->setTextureData(toByteView((const char *)theMipImage.data, (quint32)inTextureDataSize),
                                    (quint8)idx,
                                    (quint32)curWidth,
                                    (quint32)curHeight,
                                    theMipImage.format,
                                    m_destinationFormat);

        if (prevImage.data == inTextureData)
            prevImage = QSSGTextureData();

        QSSGTextureData temp = prevImage;
        prevImage = theMipImage;
        theMipImage = temp;
    }
    ::free(theMipImage.data);
    ::free(prevImage.data);
}

//------------------------------------------------------------------------------------
// GL compute based filtering
//------------------------------------------------------------------------------------

static const char *computeUploadShader(QByteArray &prog, QSSGRenderTextureFormat inFormat, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n"
                "precision mediump image2D;\n";
    } else {
        prog += "#version 430\n"
                "#extension GL_ARB_compute_shader : enable\n";
    }

    if (inFormat == QSSGRenderTextureFormat::RGBA8) {
        prog += "// Set workgroup layout;\n"
                "layout (local_size_x = 16, local_size_y = 16) in;\n\n"
                "layout (rgba8, binding = 1) readonly uniform image2D inputImage;\n\n"
                "layout (rgba16f, binding = 2) writeonly uniform image2D outputImage;\n\n"
                "void main()\n"
                "{\n"
                "  if ( gl_GlobalInvocationID.x >= gl_NumWorkGroups.x || gl_GlobalInvocationID.y "
                ">= gl_NumWorkGroups.y )\n"
                "    return;\n"
                "  vec4 value = imageLoad(inputImage, ivec2(gl_GlobalInvocationID.xy));\n"
                "  imageStore( outputImage, ivec2(gl_GlobalInvocationID.xy), value );\n"
                "}\n";
    } else {
        prog += "float convertToFloat( in uint inValue )\n"
                "{\n"
                "  uint v = inValue & uint(0xFF);\n"
                "  float f = float(v)/256.0;\n"
                "  return f;\n"
                "}\n";

        prog += "int getMod( in int inValue, in int mod )\n"
                "{\n"
                "  int v = mod * (inValue/mod);\n"
                "  return inValue - v;\n"
                "}\n";

        prog += "vec4 getRGBValue( in int byteNo, vec4 inVal, vec4 inVal1 )\n"
                "{\n"
                "  vec4 result= vec4(0.0);\n"
                "  if( byteNo == 0) {\n"
                "    result.r = inVal.r;\n"
                "    result.g = inVal.g;\n"
                "    result.b = inVal.b;\n"
                "  }\n"
                "  else if( byteNo == 1) {\n"
                "    result.r = inVal.g;\n"
                "    result.g = inVal.b;\n"
                "    result.b = inVal.a;\n"
                "  }\n"
                "  else if( byteNo == 2) {\n"
                "    result.r = inVal.b;\n"
                "    result.g = inVal.a;\n"
                "    result.b = inVal1.r;\n"
                "  }\n"
                "  else if( byteNo == 3) {\n"
                "    result.r = inVal.a;\n"
                "    result.g = inVal1.r;\n"
                "    result.b = inVal1.g;\n"
                "  }\n"
                "  return result;\n"
                "}\n";

        prog += "// Set workgroup layout;\n"
                "layout (local_size_x = 16, local_size_y = 16) in;\n\n"
                "layout (rgba8, binding = 1) readonly uniform image2D inputImage;\n\n"
                "layout (rgba16f, binding = 2) writeonly uniform image2D outputImage;\n\n"
                "void main()\n"
                "{\n"
                "  vec4 result = vec4(0.0);\n"
                "  if ( gl_GlobalInvocationID.x >= gl_NumWorkGroups.x || gl_GlobalInvocationID.y "
                ">= gl_NumWorkGroups.y )\n"
                "    return;\n"
                "  int xpos = (int(gl_GlobalInvocationID.x)*3)/4;\n"
                "  int xmod = getMod(int(gl_GlobalInvocationID.x)*3, 4);\n"
                "  ivec2 readPos = ivec2(xpos, gl_GlobalInvocationID.y);\n"
                "  vec4 value = imageLoad(inputImage, readPos);\n"
                "  vec4 value1 = imageLoad(inputImage, ivec2(readPos.x + 1, readPos.y));\n"
                "  result = getRGBValue( xmod, value, value1);\n"
                "  imageStore( outputImage, ivec2(gl_GlobalInvocationID.xy), result );\n"
                "}\n";
    }
    return prog.constData();
}

static const char *computeWorkShader(QByteArray &prog, bool binESContext, bool rgbe)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n"
                "precision mediump image2D;\n";
    } else {
        prog += "#version 430\n"
                "#extension GL_ARB_compute_shader : enable\n";
    }

    prog += "int wrapMod( in int a, in int base )\n"
            "{\n"
            "  return ( a >= 0 ) ? a % base : -(a % base) + base;\n"
            "}\n";

    prog += "void getWrappedCoords( inout int sX, inout int sY, in int width, in int height )\n"
            "{\n"
            "  if (sY < 0) { sX -= width >> 1; sY = -sY; }\n"
            "  if (sY >= height) { sX += width >> 1; sY = height - sY; }\n"
            "  sX = wrapMod( sX, width );\n"
            "}\n";

    if (rgbe) {
        prog += "vec4 decodeRGBE(in vec4 rgbe)\n"
                "{\n"
                " float f = pow(2.0, 255.0 * rgbe.a - 128.0);\n"
                " return vec4(rgbe.rgb * f, 1.0);\n"
                "}\n";
        prog += "vec4 encodeRGBE(in vec4 rgba)\n"
                "{\n"
                " float maxMan = max(rgba.r, max(rgba.g, rgba.b));\n"
                " float maxExp = 1.0 + floor(log2(maxMan));\n"
                " return vec4(rgba.rgb / pow(2.0, maxExp), (maxExp + 128.0) / 255.0);\n"
                "}\n";
    }

    prog += "// Set workgroup layout;\n"
            "layout (local_size_x = 16, local_size_y = 16) in;\n\n";

    if (rgbe) {
        prog +=
            "layout (rgba8, binding = 1) readonly uniform image2D inputImage;\n\n"
            "layout (rgba8, binding = 2) writeonly uniform image2D outputImage;\n\n";
    } else {
        prog +=
            "layout (rgba16f, binding = 1) readonly uniform image2D inputImage;\n\n"
            "layout (rgba16f, binding = 2) writeonly uniform image2D outputImage;\n\n";
    }

    prog +=
            "void main()\n"
            "{\n"
            "  int prevWidth = int(gl_NumWorkGroups.x) << 1;\n"
            "  int prevHeight = int(gl_NumWorkGroups.y) << 1;\n"
            "  if ( gl_GlobalInvocationID.x >= gl_NumWorkGroups.x || gl_GlobalInvocationID.y >= "
            "gl_NumWorkGroups.y )\n"
            "    return;\n"
            "  vec4 accumVal = vec4(0.0);\n"
            "  for ( int sy = -2; sy <= 2; ++sy )\n"
            "  {\n"
            "    for ( int sx = -2; sx <= 2; ++sx )\n"
            "    {\n"
            "      int sampleX = sx + (int(gl_GlobalInvocationID.x) << 1);\n"
            "      int sampleY = sy + (int(gl_GlobalInvocationID.y) << 1);\n"
            "      getWrappedCoords(sampleX, sampleY, prevWidth, prevHeight);\n"
            "       if ((sampleY * prevWidth + sampleX) < 0 )\n"
            "        sampleY = prevHeight + sampleY;\n"
            "      ivec2 pos = ivec2(sampleX, sampleY);\n"
            "      vec4 value = imageLoad(inputImage, pos);\n";

    if (rgbe) {
        prog +=
            "      value = decodeRGBE(value);\n";
    }

    prog += "      float filterPdf = 1.0 / ( 1.0 + float(sx*sx + sy*sy)*2.0 );\n"
            "      filterPdf /= 4.71238898;\n"
            "      accumVal[0] += filterPdf * value.r;\n"
            "       accumVal[1] += filterPdf * value.g;\n"
            "       accumVal[2] += filterPdf * value.b;\n"
            "       accumVal[3] += filterPdf * value.a;\n"
            "    }\n"
            "  }\n";

    if (rgbe) {
        prog +=
            "  accumVal = encodeRGBE(accumVal);\n";
    }

    prog += "  imageStore( outputImage, ivec2(gl_GlobalInvocationID.xy), accumVal );\n"
            "}\n";

    return prog.constData();
}

static bool isGLESContext(const QSSGRef<QSSGRenderContext> &context)
{
    QSSGRenderContextType ctxType = context->renderContextType();

    // Need minimum of GL3 or GLES3
    if (ctxType == QSSGRenderContextType::GLES2 || ctxType == QSSGRenderContextType::GLES3
        || ctxType == QSSGRenderContextType::GLES3PLUS) {
        return true;
    }

    return false;
}

#define WORKGROUP_SIZE 16

QSSGRenderPrefilterTextureCompute::QSSGRenderPrefilterTextureCompute(const QSSGRef<QSSGRenderContext> &inQSSGRenderContext,
                                                                         qint32 inWidth,
                                                                         qint32 inHeight,
                                                                         const QSSGRef<QSSGRenderTexture2D> &inTexture2D,
                                                                         QSSGRenderTextureFormat inDestFormat)
    : QSSGRenderPrefilterTexture(inQSSGRenderContext, inWidth, inHeight, inTexture2D, inDestFormat)
{
}

QSSGRenderPrefilterTextureCompute::~QSSGRenderPrefilterTextureCompute() = default;

QSSGRenderShaderProgram *QSSGRenderPrefilterTextureCompute::createComputeProgram(
        const QSSGRef<QSSGRenderContext> &context, QSSGRenderTextureFormat inFormat)
{
    QByteArray computeProg;

    if (!m_bsdfProgram && inFormat != QSSGRenderTextureFormat::RGBE8) {
        m_bsdfProgram = context->compileComputeSource("Compute BSDF mipmap shader",
                                                      toByteView(computeWorkShader(computeProg, isGLESContext(context), false)))
                                .m_shader;
        return m_bsdfProgram.get();
    }
    if (!m_bsdfRGBEProgram && inFormat == QSSGRenderTextureFormat::RGBE8) {
        m_bsdfRGBEProgram = context->compileComputeSource("Compute BSDF RGBE mipmap shader",
                                                      toByteView(computeWorkShader(computeProg, isGLESContext(context), true)))
                                .m_shader;
        return m_bsdfRGBEProgram.get();
    }
    return nullptr;
}

QSSGRef<QSSGRenderShaderProgram> QSSGRenderPrefilterTextureCompute::getOrCreateUploadComputeProgram(const QSSGRef<QSSGRenderContext> &context,
                                                                                                          QSSGRenderTextureFormat inFormat)
{
    QByteArray computeProg;

    if (inFormat == QSSGRenderTextureFormat::RGB8) {
        if (!m_uploadProgram_RGB8) {
            m_uploadProgram_RGB8 = context->compileComputeSource("Compute BSDF mipmap level 0 RGB8 shader",
                                                                 toByteView(computeUploadShader(computeProg, inFormat, isGLESContext(context))))
                                           .m_shader;
        }

        return m_uploadProgram_RGB8;
    }

    if (!m_uploadProgram_RGBA8) {
        m_uploadProgram_RGBA8 = context->compileComputeSource("Compute BSDF mipmap level 0 RGBA8 shader",
                                                              toByteView(computeUploadShader(computeProg, inFormat, isGLESContext(context))))
                                        .m_shader;
    }

    return m_uploadProgram_RGBA8;
}

void QSSGRenderPrefilterTextureCompute::createLevel0Tex(void *inTextureData, qint32 inTextureDataSize, QSSGRenderTextureFormat inFormat)
{
    QSSGRenderTextureFormat theFormat = inFormat;
    qint32 theWidth = m_width;

    // Since we cannot use RGB format in GL compute
    // we treat it as a RGBA component format
    if (inFormat == QSSGRenderTextureFormat::RGB8) {
        // This works only with 4 byte aligned data
        Q_ASSERT(m_width % 4 == 0);
        theFormat = QSSGRenderTextureFormat::RGBA8;
        theWidth = (m_width * 3) / 4;
    }

    if (m_level0Tex == nullptr) {
        m_level0Tex = new QSSGRenderTexture2D(m_renderContext);
        m_level0Tex->setTextureStorage(1, theWidth, m_height, theFormat, theFormat, QSSGByteView((quint8 *)inTextureData, inTextureDataSize));
    } else {
        m_level0Tex->setTextureSubData(QSSGByteView((quint8 *)inTextureData, inTextureDataSize), 0, 0, 0, theWidth, m_height, theFormat);
    }
}

void QSSGRenderPrefilterTextureCompute::build(void *inTextureData, qint32 inTextureDataSize, QSSGRenderTextureFormat inFormat)
{
    bool needMipUpload = (inFormat != m_destinationFormat);
    QSSGRenderShaderProgram *program = nullptr;
    // re-upload data
    if (!m_textureCreated) {
        m_texture2D->setTextureStorage(m_maxMipMapLevel + 1,
                                       m_width,
                                       m_height,
                                       m_destinationFormat,
                                       inFormat,
                                       (needMipUpload) ? QSSGByteView()
                                                       : QSSGByteView((quint8 *)inTextureData, inTextureDataSize));
        // create a compute shader (if not aloread done) which computes the BSDF mipmaps for this
        // texture
        program = createComputeProgram(m_renderContext, inFormat);

        if (!program) {
            Q_ASSERT(false);
            return;
        }

        m_textureCreated = true;
    } else if (!needMipUpload) {
        m_texture2D->setTextureSubData(QSSGByteView((quint8 *)inTextureData, inTextureDataSize), 0, 0, 0, m_width, m_height, inFormat);
    }

    if (needMipUpload) {
        createLevel0Tex(inTextureData, inTextureDataSize, inFormat);
    }

    QSSGRef<QSSGRenderImage2D> theInputImage;
    QSSGRef<QSSGRenderImage2D> theOutputImage;
    theInputImage = new QSSGRenderImage2D(m_renderContext, m_texture2D, QSSGRenderImageAccessType::ReadWrite);
    theOutputImage = new QSSGRenderImage2D(m_renderContext, m_texture2D, QSSGRenderImageAccessType::ReadWrite);

    if (needMipUpload && m_level0Tex) {
        const QSSGRef<QSSGRenderShaderProgram> &uploadProg = getOrCreateUploadComputeProgram(m_renderContext, inFormat);
        if (!uploadProg)
            return;

        m_renderContext->setActiveShader(uploadProg);

        QSSGRef<QSSGRenderImage2D> theInputImage0
                = new QSSGRenderImage2D(m_renderContext, m_level0Tex, QSSGRenderImageAccessType::ReadWrite);

        theInputImage0->setTextureLevel(0);
        QSSGRenderCachedShaderProperty<QSSGRenderImage2D *> theCachedinputImage0("inputImage", uploadProg);
        theCachedinputImage0.set(theInputImage0.data());

        theOutputImage->setTextureLevel(0);
        QSSGRenderCachedShaderProperty<QSSGRenderImage2D *> theCachedOutputImage("outputImage", uploadProg);
        theCachedOutputImage.set(theOutputImage.data());

        m_renderContext->dispatchCompute(uploadProg, m_width, m_height, 1);

        // sync
        QSSGRenderBufferBarrierFlags flags(QSSGRenderBufferBarrierValues::ShaderImageAccess);
        m_renderContext->setMemoryBarrier(flags);
    }

    int width = m_width >> 1;
    int height = m_height >> 1;

    m_renderContext->setActiveShader(program);

    for (int i = 1; i <= m_maxMipMapLevel; ++i) {
        theOutputImage->setTextureLevel(i);
        QSSGRenderCachedShaderProperty<QSSGRenderImage2D *> theCachedOutputImage("outputImage", program);
        theCachedOutputImage.set(theOutputImage.data());
        theInputImage->setTextureLevel(i - 1);
        QSSGRenderCachedShaderProperty<QSSGRenderImage2D *> theCachedinputImage("inputImage", program);
        theCachedinputImage.set(theInputImage.data());

        m_renderContext->dispatchCompute(program, width, height, 1);

        width = width > 2 ? width >> 1 : 1;
        height = height > 2 ? height >> 1 : 1;

        // sync
        QSSGRenderBufferBarrierFlags flags(QSSGRenderBufferBarrierValues::ShaderImageAccess);
        m_renderContext->setMemoryBarrier(flags);
    }
}

QT_END_NAMESPACE
