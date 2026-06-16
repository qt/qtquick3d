// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquick3dshaderimage_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include "private/qssgrenderbuffermanager_p.h"
#include "qquick3dobject_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ShaderImage
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Declares a GPU image resource that shaders can read and write.
    \since 6.13

    A ShaderImage is a GPU-side image resource with an explicit \l shape,
    size and pixel \l format, with no content of its own. Unlike
    \l Texture, it is not loaded from an image file or a \l TextureData. It
    is meant to be written to or read from directly in shader code
    (typically in a \l ComputePass, via \c imageStore()/imageLoad(), but
    also from a \l CustomMaterial or an \l Effect), rather than sampled as
    a regular texture.

    A ShaderImage is exposed to a shader by using it as a property in the
    ComputePass, RenderPass, Effect or CustomMaterial:

    \qml
    ShaderImage {
        id: myImage
        width: 256
        height: 256
    }

    ComputePass {
        computeShader: "generate.comp"
        property ShaderImage targetImage: myImage
    }
    \endqml

    In the shader, the property is declared with the \c SHADER_IMAGE macro,
    using the same name as the QML property (\c targetImage in the example
    above), and is then used like a regular GLSL image variable:

    \badcode
    SHADER_IMAGE("rgba8", targetImage, image2D, "writeonly");

    void main() {
        imageStore(targetImage, ivec2(gl_GlobalInvocationID.xy), vec4(1.0, 0.0, 0.0, 1.0));
    }
    \endcode

    The first argument to \c SHADER_IMAGE is the GLSL image format (such as
    \c{"rgba8"} or \c{"r32ui"}), which must match the ShaderImage's
    \l format. The third argument is the GLSL image type, which must match
    the ShaderImage's \l shape, with an \c i or \c u prefix for an integer
    \l format (for example \c image2D, or \c uimage3D for a three-dimensional
    image in an unsigned integer format). A ShaderImage whose format or shape
    does not match the declaration is reported with a warning, and the shader
    cannot be used; this also applies when the format or the shape changes
    later. The last argument is a memory qualifier string
    made up of any combination of \c readonly, \c writeonly, \c coherent,
    \c volatile, and \c restrict, matching the GLSL memory qualifiers of the
    same names; an empty string means implicit read-write access. Any other
    word is rejected with a warning, and the shader cannot be used.

    \note On OpenGL ES, implicit read-write access is only allowed for the
    \c r32f, \c r32i, and \c r32ui formats. An image in any other format has
    to be declared either \c readonly or \c writeonly, otherwise the shader
    fails to compile on OpenGL ES. Not all formats are available on OpenGL
    ES either; see \l format.

    The declaration is recognized by scanning the shader source before
    any preprocessing takes place. The format and the memory qualifiers must
    therefore be written as literal strings, and the name as a plain
    identifier; a \c{#define}'d format is not recognized. Each name can be
    declared only once in a shader, even in different preprocessor branches,
    and its format must match the \l format property. Declarations inside
    comments are ignored. A declaration that
    does not follow this form is left in the shader source unprocessed,
    which typically shows up as a shader compilation error.

    \note Atomic operations on images (\c{imageAtomicAdd()} and friends) are
    not supported with the Metal graphics API. Such shaders translate and
    compile, but the results are undefined, because the translation relies on
    a hidden helper buffer that is never bound. Atomics on a
    \l ShaderStorageBuffer are not affected and work on all backends.

    \sa ComputePass, ShaderStorageBuffer
*/

QQuick3DShaderImage::QQuick3DShaderImage(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::ShaderStorageImage)), parent)
{

}

QQuick3DShaderImage::~QQuick3DShaderImage()
{

}

/*!
    \qmlproperty enumeration ShaderImage::shape
    This property holds the shape of the image: the kind of GPU texture that
    is created, and the GLSL image type that the shader's \c SHADER_IMAGE
    declaration has to use. The default value is \c{ShaderImage.Image2D}.

    \value ShaderImage.Image1D A one-dimensional image of \l width texels,
           \c image1D in the shader.
    \value ShaderImage.Image2D A two-dimensional image of \l width by
           \l height texels, \c image2D in the shader.
    \value ShaderImage.Image3D A three-dimensional image of \l width by
           \l height by \l depth texels, \c image3D in the shader.
    \value ShaderImage.Image2DArray An array of \l arraySize two-dimensional
           images of \l width by \l height texels, \c image2DArray in the
           shader.
    \value ShaderImage.ImageCube A cube map with six faces of \l width by
           \l height texels, \c imageCube in the shader. The width and the
           height must be equal.

    The size properties that a shape does not use are ignored. With an
    integer \l format, the image type has an \c i or \c u prefix, such as
    \c iimage2D or \c uimage3D.

    \note One-dimensional images are not supported with OpenGL ES and
    Direct3D. Cube map images are not supported with Direct3D, which has no
    read-write cube texture type; a shader declaring an \c imageCube fails to
    build there. Arrays of one-dimensional images and arrays of cube maps are
    not supported.

    Setting a value outside this enumeration is ignored, with a warning, and
    the previous shape is retained.
*/
QQuick3DShaderImage::ImageShape QQuick3DShaderImage::shape() const
{
    return m_shape;
}

/*!
    \qmlproperty int ShaderImage::width
    This property holds the width of the image, in texels. The default value is \c 1.
    Values smaller than \c 1 are ignored.
*/
int QQuick3DShaderImage::width() const
{
    return m_width;
}

/*!
    \qmlproperty int ShaderImage::height
    This property holds the height of the image, in texels. It is not used
    by an \c Image1D \l shape. The default value is \c 1. Values smaller than
    \c 1 are ignored.
*/
int QQuick3DShaderImage::height() const
{
    return m_height;
}

/*!
    \qmlproperty int ShaderImage::depth
    This property holds the depth of the image, in texels. It is only used
    by an \c Image3D \l shape. The default value is \c 1. Values smaller than
    \c 1 are ignored.

    This is a size, not the number of bits per pixel that QImage::depth()
    returns.
*/
int QQuick3DShaderImage::depth() const
{
    return m_depth;
}

/*!
    \qmlproperty int ShaderImage::arraySize
    This property holds the number of images in the array. It is only used
    by an \c Image2DArray \l shape. The default value is \c 1. Values smaller
    than \c 1 are ignored.
*/
int QQuick3DShaderImage::arraySize() const
{
    return m_arraySize;
}

/*!
    \qmlproperty enumeration ShaderImage::format
    This property holds the pixel format of the image. The default value is
    \c{ShaderImage.RGBA8}, an 8-bit-per-channel unsigned normalized RGBA
    image.

    Each value corresponds to one GLSL image format qualifier, which is what
    the shader's \c SHADER_IMAGE declaration has to name as its format
    argument. The two must match: a mismatch is reported as a warning and
    leaves the image unusable by that shader.

    \value ShaderImage.RGBA8 \c{"rgba8"}, the default.
    \value ShaderImage.R8 \c{"r8"}
    \value ShaderImage.RG8 \c{"rg8"}
    \value ShaderImage.R16 \c{"r16"}
    \value ShaderImage.RG16 \c{"rg16"}
    \value ShaderImage.RGBA16F \c{"rgba16f"}
    \value ShaderImage.RGBA32F \c{"rgba32f"}
    \value ShaderImage.R16F \c{"r16f"}
    \value ShaderImage.R32F \c{"r32f"}
    \value ShaderImage.RGB10A2 \c{"rgb10_a2"}, a packed format with 10 bits
           per color channel and 2 bits of alpha.
    \value ShaderImage.R8SI \c{"r8i"}, a signed integer format.
    \value ShaderImage.R32SI \c{"r32i"}
    \value ShaderImage.RG32SI \c{"rg32i"}
    \value ShaderImage.RGBA32SI \c{"rgba32i"}
    \value ShaderImage.R8UI \c{"r8ui"}, an unsigned integer format.
    \value ShaderImage.R32UI \c{"r32ui"}
    \value ShaderImage.RG32UI \c{"rg32ui"}
    \value ShaderImage.RGBA32UI \c{"rgba32ui"}

    An integer format must be declared with a matching integer image type in
    the shader (\c iimage2D for the signed and \c uimage2D for the unsigned
    variants, rather than \c image2D), and is read and written with
    \c ivec4/\c uvec4 rather than \c vec4.

    \note OpenGL ES supports only a subset of these formats for images:
    \c RGBA8, \c RGBA16F, \c RGBA32F, \c R32F, \c R32SI, \c RGBA32SI,
    \c R32UI, and \c RGBA32UI. The other formats (\c R8, \c RG8, \c R16,
    \c RG16, \c R16F, \c RGB10A2, \c R8SI, \c R8UI, \c RG32SI, and
    \c RG32UI) are only available with desktop OpenGL, Vulkan, Metal, and
    Direct3D. On OpenGL ES, a shader that declares one of them fails to
    build. Of the supported formats, all except \c R32F, \c R32SI, and
    \c R32UI also need a \c readonly or \c writeonly qualifier in the
    \c SHADER_IMAGE declaration on OpenGL ES.

    Setting a value outside this enumeration is ignored, with a warning, and
    the previous format is retained.
*/
QQuick3DShaderImage::ImageFormat QQuick3DShaderImage::format() const
{
    return m_format;
}

void QQuick3DShaderImage::setShape(QQuick3DShaderImage::ImageShape shape)
{
    if (m_shape == shape) return;
    if (shape < Image1D || shape > ImageCube) {
        qWarning() << "ShaderImage shape not supported.";
        return;
    }
    m_shape = shape;
    emit shapeChanged();
    markSizeDirty();
}

void QQuick3DShaderImage::setWidth(int width)
{
    if (m_width == width) return;
    if (width < 1) return;
    m_width = width;
    emit widthChanged();
    markSizeDirty();
}

void QQuick3DShaderImage::setHeight(int height)
{
    if (m_height == height) return;
    if (height < 1) return;
    m_height = height;
    emit heightChanged();
    markSizeDirty();
}

void QQuick3DShaderImage::setDepth(int depth)
{
    if (m_depth == depth) return;
    if (depth < 1) return;
    m_depth = depth;
    emit depthChanged();
    markSizeDirty();
}

void QQuick3DShaderImage::setArraySize(int arraySize)
{
    if (m_arraySize == arraySize) return;
    if (arraySize < 1) return;
    m_arraySize = arraySize;
    emit arraySizeChanged();
    markSizeDirty();
}

void QQuick3DShaderImage::markSizeDirty()
{
    m_dirtyFlags.setFlag(DirtyFlag::SizeDirty);
    update();
}

static struct {QRhiTexture::Format rhiFormat; QQuick3DShaderImage::ImageFormat imageFormat; } supportedFormats[] =
{
    { QRhiTexture::RGBA8, QQuick3DShaderImage::RGBA8 },
    { QRhiTexture::R8, QQuick3DShaderImage::R8 },
    { QRhiTexture::RG8, QQuick3DShaderImage::RG8 },
    { QRhiTexture::R16, QQuick3DShaderImage::R16 },
    { QRhiTexture::RG16, QQuick3DShaderImage::RG16 },
    { QRhiTexture::RGBA16F, QQuick3DShaderImage::RGBA16F },
    { QRhiTexture::RGBA32F, QQuick3DShaderImage::RGBA32F },
    { QRhiTexture::R16F, QQuick3DShaderImage::R16F },
    { QRhiTexture::R32F, QQuick3DShaderImage::R32F },
    { QRhiTexture::RGB10A2, QQuick3DShaderImage::RGB10A2 },
    { QRhiTexture::R8SI, QQuick3DShaderImage::R8SI },
    { QRhiTexture::R32SI, QQuick3DShaderImage::R32SI },
    { QRhiTexture::RG32SI, QQuick3DShaderImage::RG32SI },
    { QRhiTexture::RGBA32SI, QQuick3DShaderImage::RGBA32SI },
    { QRhiTexture::R8UI, QQuick3DShaderImage::R8UI },
    { QRhiTexture::R32UI, QQuick3DShaderImage::R32UI },
    { QRhiTexture::RG32UI, QQuick3DShaderImage::RG32UI },
    { QRhiTexture::RGBA32UI, QQuick3DShaderImage::RGBA32UI }
};


static QRhiTexture::Format toRhiFormat(QQuick3DShaderImage::ImageFormat imageFormat)
{
    for (auto fmt : supportedFormats) {
        if (fmt.imageFormat == imageFormat)
            return fmt.rhiFormat;
    }
    return QRhiTexture::UnknownFormat;
}

void QQuick3DShaderImage::setFormat(QQuick3DShaderImage::ImageFormat format)
{
    if (m_format == format) return;
    auto rhiFormat = toRhiFormat(format);
    if (rhiFormat == QRhiTexture::UnknownFormat) {
        qWarning() << "ShaderImage format not supported.";
        return;
    }
    m_format = format;
    emit formatChanged();
    m_dirtyFlags.setFlag(DirtyFlag::FormatDirty);
    update();
}

void QQuick3DShaderImage::markAllDirty()
{
    QQuick3DObject::markAllDirty();
    m_dirtyFlags.setFlag(DirtyFlag::FormatDirty);
    m_dirtyFlags.setFlag(DirtyFlag::SizeDirty);
}

static QSSGRenderImage::ShaderImageShape toRenderShape(QQuick3DShaderImage::ImageShape shape)
{
    switch (shape) {
    case QQuick3DShaderImage::Image1D: return QSSGRenderImage::ShaderImageShape::Image1D;
    case QQuick3DShaderImage::Image2D: return QSSGRenderImage::ShaderImageShape::Image2D;
    case QQuick3DShaderImage::Image3D: return QSSGRenderImage::ShaderImageShape::Image3D;
    case QQuick3DShaderImage::Image2DArray: return QSSGRenderImage::ShaderImageShape::Image2DArray;
    case QQuick3DShaderImage::ImageCube: return QSSGRenderImage::ShaderImageShape::ImageCube;
    }
    return QSSGRenderImage::ShaderImageShape::Image2D;
}

QSSGRenderGraphObject *QQuick3DShaderImage::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderImage(QSSGRenderGraphObject::Type::Image2D);
    }
    auto *img = static_cast<QSSGRenderImage *>(node);
    img->m_shaderImage = true;
    if (m_dirtyFlags.testFlag(DirtyFlag::SizeDirty)) {
        if (m_shape == ImageCube && m_width != m_height)
            qWarning() << "ShaderImage: a cube map image must have equal width and height.";
        img->m_shaderImageShape = toRenderShape(m_shape);
        img->m_shaderImageSize.x = m_width;
        img->m_shaderImageSize.y = m_height;
        img->m_shaderImageSize.z = m_depth;
        img->m_arraySize = m_arraySize;
        img->m_flags.setFlag(QSSGRenderImage::Flag::ShaderImageDirty);
        m_dirtyFlags.setFlag(DirtyFlag::SizeDirty, false);
    }
    if (m_dirtyFlags.testFlag(DirtyFlag::FormatDirty)) {
        img->m_shaderFormat = toRhiFormat(m_format);
        img->m_flags.setFlag(QSSGRenderImage::Flag::ShaderImageDirty);
        m_dirtyFlags.setFlag(DirtyFlag::FormatDirty, false);
    }

    return node;
}

QT_END_NAMESPACE
