/****************************************************************************
**
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

#include "qquick3dcustommaterial_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick/QQuickWindow>

#include "qquick3dobject_p.h"
#include "qquick3dviewport_p.h"
#include "qquick3dscenemanager_p.h"

Q_DECLARE_OPAQUE_POINTER(QQuick3DShaderUtilsTextureInput)

QT_BEGIN_NAMESPACE

/*!
    \qmltype CustomMaterial
    \inherits Material
    \inqmlmodule QtQuick3D.Materials
    \brief Base component for creating custom materials used to shade models.

    The custom material allows using custom shader code for a material. A
    vertex, fragment, or both shaders can be provided. The \l vertexShader and
    \l fragmentShader properties are URLs, referencing files containing shader
    snippets, and work very similarly to ShaderEffect or
    \l{Image::source}{Image.source}. Only the \c file and \c qrc schemes are
    supported with custom materials. It is also possible to omit the \c file
    scheme, allowing to specify a relative path in a convenient way. Such a
    path is resolved relative to the component's (the \c{.qml} file's)
    location.

    There are two main types of custom materials. This is specified by the \l
    shadingMode property. In \l{CustomMaterial.Unshaded}{unshaded} custom
    materials the fragment shader outputs a single \c vec4 color, ignoring
    lights, light probes, shadowing in the scene. In
    \l{CustomMaterial.Shaded}{shaded} materials the shader is expected to
    implement certain functions and work with built-in variables to take
    lighting and shadow contribution into account. See \l {Qt Quick 3D Custom
    Material Reference}{reference} on how to implement the material using the
    material interface.

    \qml
    CustomMaterial {
        // These properties are automatically exposed to the shaders
        property bool uEnvironmentMappingEnabled: true
        property bool uShadowMappingEnabled: false
        property real roughness: 0.0
        property vector3d metal_color: Qt.vector3d(0.805, 0.395, 0.305)

        property TextureInput uEnvironmentTexture: TextureInput {
                enabled: uEnvironmentMappingEnabled
                texture: Texture {
                    id: envImage
                    source: "maps/spherical_checker.png"
                }
        }
        property TextureInput uBakedShadowTexture: TextureInput {
                enabled: uShadowMappingEnabled
                texture: Texture {
                    id: shadowImage
                    source: "maps/shadow.png"
                }
        }

        shaderKey: CustomMaterial.Glossy

        fragmentShader: "shaders/copper.frag"
    }
    \endqml

    The example here from CopperMaterial shows how the material is built.
    First, the shader parameters are specified as properties. The names and
    types must match the names in the shader code. Textures use TextureInput to
    assign \l{QtQuick3D::Texture}{texture} into the shader variable. The
    shaderKey property specifies more information about the shader and also
    configures some of its features on or off when the custom material is built
    by QtQuick3D shader generator.
*/

/*!
    \qmlproperty enumeration CustomMaterial::shadingMode
    Specifies the type of the material. The default value is Shaded.

    \value CustomMaterial.Unshaded
    \value CustomMaterial.Shaded
*/

/*!
    \qmlproperty bool CustomMaterial::hasTransparency

    Specifies that the material has transparency. For example, a material where
    the fragment shader outputs fragment colors with an alpha smaller than 1.0
    will need this to be set to true. The default value is false.
*/

/*!
    \qmlproperty bool CustomMaterial::alwaysDirty
    Specifies that the material state is always dirty, which indicates that the material needs
    to be refreshed every time it is used by the QtQuick3D.
*/

/*!
    \qmlproperty enumeration CustomMaterial::shaderKey
    Specifies the options used by the shader using the combination of shader key values.

    \value CustomMaterial.Diffuse The shader uses diffuse lighting.
    \value CustomMaterial.Specular The shader uses specular lighting.
    \value CustomMaterial.Cutout The shader uses alpha cutout.
    \value CustomMaterial.Refraction The shader uses refraction.
    \value CustomMaterial.Transparent The shader uses transparency.
    \value CustomMaterial.Transmissive The shader uses transmissiveness.
    \value CustomMaterial.Glossy The shader is default glossy. This is a combination
    of \c CustomMaterial.Diffuse and \c CustomMaterial.Specular.
*/

/*!
    \qmlproperty enumeration CustomMaterial::sourceBlend

    Specifies the source blend factor. The default value is \l
    CustomMaterial.NoBlend. Note that blending is only active when \l
    hasTransparency is enabled and the source and destination blend factors are
    something other than CustomMaterial.NoBlend.

    \value CustomMaterial.NoBlend
    \value CustomMaterial.Zero
    \value CustomMaterial.One
    \value CustomMaterial.SrcColor
    \value CustomMaterial.OneMinusSrcColor
    \value CustomMaterial.DstColor
    \value CustomMaterial.OneMinusDstColor
    \value CustomMaterial.SrcAlpha
    \value CustomMaterial.OneMinusSrcAlpha
    \value CustomMaterial.DstAlpha
    \value CustomMaterial.OneMinusDstAlpha
    \value CustomMaterial.ConstantColor
    \value CustomMaterial.OneMinusConstantColor
    \value CustomMaterial.ConstantAlpha
    \value CustomMaterial.OneMinusConstantAlpha
    \value CustomMaterial.SrcAlphaSaturate
*/

/*!
    \qmlproperty enumeration CustomMaterial::destinationBlend

    Specifies the destination blend factor. The default value is \l
    CustomMaterial.NoBlend. Note that blending is only active when \l
    hasTransparency is enabled and the source and destination blend factors are
    something other than CustomMaterial.NoBlend.

    \value CustomMaterial.NoBlend
    \value CustomMaterial.Zero
    \value CustomMaterial.One
    \value CustomMaterial.SrcColor
    \value CustomMaterial.OneMinusSrcColor
    \value CustomMaterial.DstColor
    \value CustomMaterial.OneMinusDstColor
    \value CustomMaterial.SrcAlpha
    \value CustomMaterial.OneMinusSrcAlpha
    \value CustomMaterial.DstAlpha
    \value CustomMaterial.OneMinusDstAlpha
    \value CustomMaterial.ConstantColor
    \value CustomMaterial.OneMinusConstantColor
    \value CustomMaterial.ConstantAlpha
    \value CustomMaterial.OneMinusConstantAlpha
    \value CustomMaterial.SrcAlphaSaturate
*/

static inline QRhiGraphicsPipeline::BlendFactor toRhiBlendFactor(QQuick3DCustomMaterial::BlendMode mode)
{
    switch (mode) {
    case QQuick3DCustomMaterial::BlendMode::Zero:
        return QRhiGraphicsPipeline::Zero;
    case QQuick3DCustomMaterial::BlendMode::One:
        return QRhiGraphicsPipeline::One;
    case QQuick3DCustomMaterial::BlendMode::SrcColor:
        return QRhiGraphicsPipeline::SrcColor;
    case QQuick3DCustomMaterial::BlendMode::OneMinusSrcColor:
        return QRhiGraphicsPipeline::OneMinusSrcColor;
    case QQuick3DCustomMaterial::BlendMode::DstColor:
        return QRhiGraphicsPipeline::DstColor;
    case QQuick3DCustomMaterial::BlendMode::OneMinusDstColor:
        return QRhiGraphicsPipeline::OneMinusDstColor;
    case QQuick3DCustomMaterial::BlendMode::SrcAlpha:
        return QRhiGraphicsPipeline::SrcAlpha;
    case QQuick3DCustomMaterial::BlendMode::OneMinusSrcAlpha:
        return QRhiGraphicsPipeline::OneMinusSrcAlpha;
    case QQuick3DCustomMaterial::BlendMode::DstAlpha:
        return QRhiGraphicsPipeline::DstAlpha;
    case QQuick3DCustomMaterial::BlendMode::OneMinusDstAlpha:
        return QRhiGraphicsPipeline::OneMinusDstAlpha;
    case QQuick3DCustomMaterial::BlendMode::ConstantColor:
        return QRhiGraphicsPipeline::ConstantColor;
    case QQuick3DCustomMaterial::BlendMode::OneMinusConstantColor:
        return QRhiGraphicsPipeline::OneMinusConstantColor;
    case QQuick3DCustomMaterial::BlendMode::ConstantAlpha:
        return QRhiGraphicsPipeline::ConstantAlpha;
    case QQuick3DCustomMaterial::BlendMode::OneMinusConstantAlpha:
        return QRhiGraphicsPipeline::OneMinusConstantAlpha;
    case QQuick3DCustomMaterial::BlendMode::SrcAlphaSaturate:
        return QRhiGraphicsPipeline::SrcAlphaSaturate;
    default:
        return QRhiGraphicsPipeline::One;
    }
}

template <QVariant::Type>
struct ShaderType
{
};

template<>
struct ShaderType<QVariant::Double>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Float; }
    static QByteArray name() { return QByteArrayLiteral("float"); }
};

template<>
struct ShaderType<QVariant::Bool>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Boolean; }
    static QByteArray name() { return QByteArrayLiteral("bool"); }
};

template<>
struct ShaderType<QVariant::Int>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Integer; }
    static QByteArray name() { return QByteArrayLiteral("int"); }
};

template<>
struct ShaderType<QVariant::Vector2D>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Vec2; }
    static QByteArray name() { return QByteArrayLiteral("vec2"); }
};

template<>
struct ShaderType<QVariant::Vector3D>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Vec3; }
    static QByteArray name() { return QByteArrayLiteral("vec3"); }
};

template<>
struct ShaderType<QVariant::Vector4D>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Vec4; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

template<>
struct ShaderType<QVariant::Color>
{
    static constexpr QSSGRenderShaderDataType type() { return QSSGRenderShaderDataType::Rgba; }
    static QByteArray name() { return QByteArrayLiteral("vec4"); }
};

QQuick3DCustomMaterial::QQuick3DCustomMaterial(QQuick3DObject *parent)
    : QQuick3DMaterial(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::CustomMaterial)), parent)
{
}

QQuick3DCustomMaterial::~QQuick3DCustomMaterial() {}

bool QQuick3DCustomMaterial::hasTransparency() const
{
    return m_hasTransparency;
}

void QQuick3DCustomMaterial::setHasTransparency(bool hasTransparency)
{
    if (m_hasTransparency == hasTransparency)
        return;

    m_hasTransparency = hasTransparency;
    update();
    emit hasTransparencyChanged();
}

QQuick3DCustomMaterial::BlendMode QQuick3DCustomMaterial::srcBlend() const
{
    return m_srcBlend;
}

void QQuick3DCustomMaterial::setSrcBlend(BlendMode mode)
{
    if (m_srcBlend == mode)
        return;

    m_srcBlend = mode;
    update();
    emit srcBlendChanged();
}

QQuick3DCustomMaterial::BlendMode QQuick3DCustomMaterial::dstBlend() const
{
    return m_dstBlend;
}

void QQuick3DCustomMaterial::setDstBlend(BlendMode mode)
{
    if (m_dstBlend == mode)
        return;

    m_dstBlend = mode;
    update();
    emit dstBlendChanged();
}

QQuick3DCustomMaterial::ShadingMode QQuick3DCustomMaterial::shadingMode() const
{
    return m_shadingMode;
}

void QQuick3DCustomMaterial::setShadingMode(ShadingMode mode)
{
    if (m_shadingMode == mode)
        return;

    m_shadingMode = mode;
    markDirty(Dirty::ShaderSettingsDirty);
    emit shadingModeChanged();
}

QUrl QQuick3DCustomMaterial::vertexShader() const
{
    return m_vertexShader;
}

void QQuick3DCustomMaterial::setVertexShader(const QUrl &url)
{
    if (m_vertexShader == url)
        return;

    m_vertexShader = url;
    emit vertexShaderChanged();
}

QUrl QQuick3DCustomMaterial::fragmentShader() const
{
    return m_fragmentShader;
}

void QQuick3DCustomMaterial::setFragmentShader(const QUrl &url)
{
    if (m_fragmentShader == url)
        return;

    m_fragmentShader = url;
    emit fragmentShaderChanged();
}

void QQuick3DCustomMaterial::markAllDirty()
{
    m_dirtyAttributes = 0xffffffff;
    QQuick3DMaterial::markAllDirty();
}

bool QQuick3DCustomMaterial::alwaysDirty() const
{
    return m_alwaysDirty;
}

void QQuick3DCustomMaterial::setAlwaysDirty(bool alwaysDirty)
{
    if (m_alwaysDirty == alwaysDirty)
        return;

    m_alwaysDirty = alwaysDirty;
    update();
    emit alwaysDirtyChanged();
}

QQuick3DCustomMaterial::ShaderKeyFlags QQuick3DCustomMaterial::shaderKey() const
{
    return m_shaderKey;
}

void QQuick3DCustomMaterial::setShaderKey(ShaderKeyFlags key)
{
    if (m_shaderKey == key)
        return;

    m_shaderKey = key;
    markDirty(Dirty::ShaderSettingsDirty);
    emit shaderKeyChanged();
}

QSSGRenderGraphObject *QQuick3DCustomMaterial::updateSpatialNode(QSSGRenderGraphObject *node)
{
    // Find the parent window
    QQuickWindow *window = nullptr;
    if (const auto &manager = QQuick3DObjectPrivate::get(this)->sceneManager)
        window = manager->window();

    using StringPair = QPair<QByteArray, QByteArray>;
    QVarLengthArray<StringPair, 16> uniforms;
    QSSGRenderCustomMaterial *customMaterial = static_cast<QSSGRenderCustomMaterial *>(node);
    if (customMaterial && (m_dirtyAttributes & ShaderSettingsDirty)) {
        delete customMaterial;
        customMaterial = nullptr;
    }

    if (!customMaterial) {
        markAllDirty();
        customMaterial = new QSSGRenderCustomMaterial;
        customMaterial->m_shadingMode = QSSGRenderCustomMaterial::ShadingMode(int(m_shadingMode));
        customMaterial->m_shaderKeyValues = QSSGRenderCustomMaterial::MaterialShaderKeyFlags(int(m_shaderKey));

        QMetaMethod propertyDirtyMethod;
        const int idx = metaObject()->indexOfSlot("onPropertyDirty()");
        if (idx != -1)
            propertyDirtyMethod = metaObject()->method(idx);

        // Properties
        const int propCount = metaObject()->propertyCount();
        int propOffset = metaObject()->propertyOffset();

        // Custom materials can have multilayered inheritance structure, so find the actual propOffset
        const QMetaObject *superClass = metaObject()->superClass();
        while (superClass && qstrcmp(superClass->className(), "QQuick3DCustomMaterial") != 0)  {
            propOffset = superClass->propertyOffset();
            superClass = superClass->superClass();
        }

        QVector<QMetaProperty> userProperties;
        for (int i = propOffset; i != propCount; ++i) {
            const auto property = metaObject()->property(i);
            if (Q_UNLIKELY(!property.isValid()))
                continue;

            // Track the property changes
            if (property.hasNotifySignal() && propertyDirtyMethod.isValid())
                connect(this, property.notifySignal(), this, propertyDirtyMethod);

            if (property.type() == QVariant::Double) {
                uniforms.append({ ShaderType<QVariant::Double>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Double>::type(), i});
            } else if (property.type() == QVariant::Bool) {
                uniforms.append({ ShaderType<QVariant::Bool>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Bool>::type(), i});
            } else if (property.type() == QVariant::Vector2D) {
                uniforms.append({ ShaderType<QVariant::Vector2D>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector2D>::type(), i});
            } else if (property.type() == QVariant::Vector3D) {
                uniforms.append({ ShaderType<QVariant::Vector3D>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector3D>::type(), i});
            } else if (property.type() == QVariant::Vector4D) {
                uniforms.append({ ShaderType<QVariant::Vector4D>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector4D>::type(), i});
            } else if (property.type() == QVariant::Int) {
                uniforms.append({ ShaderType<QVariant::Int>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Int>::type(), i});
            } else if (property.type() == QVariant::Color) {
                uniforms.append({ ShaderType<QVariant::Color>::name(), property.name() });
                customMaterial->m_properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Color>::type(), i});
            } else if (property.type() == QVariant::UserType) {
                if (property.userType() == qMetaTypeId<QQuick3DShaderUtilsTextureInput *>())
                    userProperties.push_back(property);
            } else {
                qWarning("No know uniform convertion found for property %s. Skipping", property.name());
            }
        }

        // Textures
        for (const auto &userProperty : qAsConst(userProperties)) {
            QSSGRenderCustomMaterial::TextureProperty textureData;
            QQuick3DShaderUtilsTextureInput *texture = userProperty.read(this).value<QQuick3DShaderUtilsTextureInput *>();
            const QByteArray &name = userProperty.name();
            if (name.isEmpty()) // Warnings here will just drown in the shader error messages
                continue;
            texture->name = name;
            QQuick3DTexture *tex = texture->texture(); //
            connect(texture, &QQuick3DShaderUtilsTextureInput::textureDirty, this, &QQuick3DCustomMaterial::onTextureDirty);
            textureData.name = name;
            if (texture->enabled)
                textureData.texImage = tex->getRenderImage();
            textureData.shaderDataType = QSSGRenderShaderDataType::Texture2D;
            textureData.clampType = tex->horizontalTiling() == QQuick3DTexture::Repeat ? QSSGRenderTextureCoordOp::Repeat
                                                                                     : (tex->horizontalTiling() == QQuick3DTexture::ClampToEdge) ? QSSGRenderTextureCoordOp::ClampToEdge
                                                                                                                                               : QSSGRenderTextureCoordOp::MirroredRepeat;
            uniforms.append({ QByteArrayLiteral("sampler2D"), textureData.name });
            customMaterial->m_textureProperties.push_back(textureData);
        }

        //#ifdef QQ3D_SHADER_META
        ///*{
        //    "uniforms": [
        //        { "type": "vec2", "name": "CameraClipRange" }
        //    ]
        //}*/
        //#endif // QQ3D_SHADER_META
        static const char *metaStart = "#ifdef QQ3D_SHADER_META\n/*{\n  \"uniforms\": [\n";
        static const char *metaEnd = "  ]\n}*/\n#endif\n";
        QByteArray shaderPrefix = QByteArrayLiteral("#include \"customMaterial.glsllib\"\n");
        shaderPrefix.append(metaStart);
        for (int i = 0, count = uniforms.count(); i < count; ++i) {
            const auto &typeAndName(uniforms[i]);
            shaderPrefix.append("    { \"type\": \"" + typeAndName.first + "\", \"name\": \"" + typeAndName.second + "\" }");
            if (i < count - 1)
                shaderPrefix.append(",");
            shaderPrefix.append("\n");
        }
        shaderPrefix.append(metaEnd);

        QByteArray vertex, fragment, shaderCode, shaderPathKey;
        const QQmlContext *context = qmlContext(this);

        if (!m_vertexShader.isEmpty())
            vertex = QSSGShaderUtils::resolveShader(m_vertexShader, context, shaderPathKey);

        if (!m_fragmentShader.isEmpty())
            fragment = QSSGShaderUtils::resolveShader(m_fragmentShader, context, shaderPathKey);

        if (!vertex.isEmpty() || !fragment.isEmpty()) {
            shaderCode = QSSGShaderUtils::mergeShaderCode(shaderPrefix, QByteArray(), QByteArray(), vertex, fragment);
            customMaterial->m_shaderPathKey = shaderPathKey;
            const auto &renderContext = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));
            // Store the source code, will be retrieved by
            // QSSGCustomMaterialShaderGenerator::generateVertexShader and
            // generateFragmentShader.
            renderContext->shaderLibraryManager()->setShaderData(shaderPathKey, shaderCode);
        }
    }

    customMaterial->m_alwaysDirty = m_alwaysDirty;
    customMaterial->m_hasTransparency = m_hasTransparency;
    if (m_hasTransparency && m_srcBlend != BlendMode::NoBlend && m_dstBlend != BlendMode::NoBlend) {
        customMaterial->m_hasBlending = true;
        customMaterial->m_srcBlend = toRhiBlendFactor(m_srcBlend);
        customMaterial->m_dstBlend = toRhiBlendFactor(m_dstBlend);
    } else {
        customMaterial->m_hasBlending = false;
    }

    QQuick3DMaterial::updateSpatialNode(customMaterial);

    if (m_dirtyAttributes & Dirty::PropertyDirty) {
        for (const auto &prop : qAsConst(customMaterial->m_properties)) {
            auto p = metaObject()->property(prop.pid);
            if (Q_LIKELY(p.isValid()))
                prop.value = p.read(this);
        }
    }

    if (m_dirtyAttributes & Dirty::TextureDirty) {
        // TODO:
    }

    m_dirtyAttributes = 0;

    return customMaterial;
}

void QQuick3DCustomMaterial::onPropertyDirty()
{
    markDirty(Dirty::PropertyDirty);
    update();
}

void QQuick3DCustomMaterial::onTextureDirty(QQuick3DShaderUtilsTextureInput *texture)
{
    Q_UNUSED(texture);
    markDirty(Dirty::TextureDirty);
    update();
}

QT_END_NAMESPACE
