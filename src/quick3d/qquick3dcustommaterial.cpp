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

    ### rewrite this paragraph
    There are two types of custom materials, which
    differ on how they are using the material library. First one uses the
    custom material interface provided by the library to implement materials
    similarly to many of the materials in the material library without
    implementing it's own main function. This type of material must implement
    all the required functions of the material. The second type implements it's
    own main function, but can still use functionality from the material
    library. See \l {Qt Quick 3D Custom Material Reference}{reference} on how
    to implement the material using the material interface.

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
    \qmlproperty bool CustomMaterial::hasTransparency
    Specifies that the material has transparency.
*/
/*!
    \qmlproperty bool CustomMaterial::hasRefraction
    Specifies that the material has refraction.
*/
/*!
    \qmlproperty bool CustomMaterial::alwaysDirty
    Specifies that the material state is always dirty, which indicates that the material needs
    to be refreshed every time it is used by the QtQuick3D.
*/

/*!
    \qmlproperty string CustomMaterial::shaderKey
    Specifies the options used by the shader using the combination of shader key values.

    \value CustomMaterial.Diffuse The shader uses diffuse lighting.
    \value ShaderInCustomMaterialfo.Specular The shader uses specular lighting.
    \value CustomMaterial.Cutout The shader uses alpha cutout.
    \value CustomMaterial.Refraction The shader uses refraction.
    \value CustomMaterial.Transparent The shader uses transparency.
    \value CustomMaterial.Transmissive The shader uses transmissiveness.
    \value CustomMaterial.Glossy The shader is default glossy. This is a combination
    of \c CustomMaterial.Diffuse and \c CustomMaterial.Specular.
*/

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

bool QQuick3DCustomMaterial::hasRefraction() const
{
    return m_hasRefraction;
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

void QQuick3DCustomMaterial::setHasTransparency(bool hasTransparency)
{
    if (m_hasTransparency == hasTransparency)
        return;

    m_hasTransparency = hasTransparency;
    emit hasTransparencyChanged();
}

void QQuick3DCustomMaterial::setHasRefraction(bool hasRefraction)
{
    if (m_hasRefraction == hasRefraction)
        return;

    m_hasRefraction = hasRefraction;
    emit hasRefractionChanged();
}

void QQuick3DCustomMaterial::setAlwaysDirty(bool alwaysDirty)
{
    if (m_alwaysDirty == alwaysDirty)
        return;

    m_alwaysDirty = alwaysDirty;
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
    if (!customMaterial) {
        markAllDirty();
        customMaterial = new QSSGRenderCustomMaterial;
        customMaterial->m_shaderKeyValues = QSSGRenderCustomMaterial::MaterialShaderKeyFlags(int(m_shaderKey));
        customMaterial->m_alwaysDirty = m_alwaysDirty;
        customMaterial->m_hasTransparency = m_hasTransparency;
        customMaterial->m_hasRefraction = m_hasRefraction;

        QByteArray shaderPrefix = QByteArrayLiteral("#include \"customMaterial.glsllib\"\n");

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
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Double>::type(), i});
            } else if (property.type() == QVariant::Bool) {
                uniforms.append({ ShaderType<QVariant::Bool>::name(), property.name() });
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Bool>::type(), i});
            } else if (property.type() == QVariant::Vector2D) {
                uniforms.append({ ShaderType<QVariant::Vector2D>::name(), property.name() });
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector2D>::type(), i});
            } else if (property.type() == QVariant::Vector3D) {
                uniforms.append({ ShaderType<QVariant::Vector3D>::name(), property.name() });
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector3D>::type(), i});
            } else if (property.type() == QVariant::Vector4D) {
                uniforms.append({ ShaderType<QVariant::Vector4D>::name(), property.name() });
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Vector4D>::type(), i});
            } else if (property.type() == QVariant::Int) {
                uniforms.append({ ShaderType<QVariant::Int>::name(), property.name() });
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Int>::type(), i});
            } else if (property.type() == QVariant::Color) {
                uniforms.append({ ShaderType<QVariant::Color>::name(), property.name() });
                customMaterial->properties.push_back({ property.name(), property.read(this), ShaderType<QVariant::Color>::type(), i});
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
            customMaterial->textureProperties.push_back(textureData);
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

            customMaterial->commands.push_back(new QSSGBindShader(shaderPathKey));
            customMaterial->commands.push_back(new QSSGApplyInstanceValue());
            customMaterial->commands.push_back(new QSSGRender);

            const auto &renderContext = QSSGRenderContextInterface::getRenderContextInterface(quintptr(window));
            renderContext->customMaterialSystem()->setMaterialClassShader(shaderPathKey, shaderCode);
        }
    }

    QQuick3DMaterial::updateSpatialNode(customMaterial);

    if (m_dirtyAttributes & Dirty::PropertyDirty) {
        for (const auto &prop : qAsConst(customMaterial->properties)) {
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
