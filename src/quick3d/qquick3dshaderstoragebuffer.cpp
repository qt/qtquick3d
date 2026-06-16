// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qquick3dshaderstoragebuffer_p.h"
#include "qquick3dobject_p.h"
#include "qquick3drenderextensions.h"
#include <ssg/qssgrenderextensions.h>

#include <QtQuick3DRuntimeRender/private/qssgrenderbuffer_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ShaderStorageBuffer
    \inherits Object3D
    \inqmlmodule QtQuick3D
    \brief Declares a GPU buffer resource that shaders can read or write.
    \since 6.13

    A ShaderStorageBuffer is a GPU-side shader storage buffer, with content
    that is either uploaded from the CPU (via \l bufferData) or produced
    entirely on the GPU (by a \l{ComputePass}{ComputePass}, a
    \l CustomMaterial, or an \l Effect writing to it). It is meant to be
    read from and/or written to directly in shader code, rather than used
    as a regular uniform buffer.

    A ShaderStorageBuffer is exposed to a shader by using it as a property in the
    ComputePass, RenderPass, Effect, or CustomMaterial:

    \qml
    ShaderStorageBuffer {
        id: myBuffer
        size: 1024
    }

    ComputePass {
        computeShader: "generate.comp"
        property ShaderStorageBuffer targetBuffer: myBuffer
    }
    \endqml

    In the shader, the property is declared with the \c STORAGE_BUFFER
    macro, using the same name as the QML property (\c targetBuffer in the
    example above):

    \badcode
    STORAGE_BUFFER(targetBuffer, "writeonly") {
        vec4 values[];
    }

    void main() {
        values[gl_GlobalInvocationID.x] = vec4(1.0);
    }
    \endcode

    The last argument to \c STORAGE_BUFFER is a memory qualifier string made
    up of any combination of \c readonly, \c writeonly, \c coherent,
    \c volatile, and \c restrict, matching the GLSL memory qualifiers of the
    same names; an empty string means implicit read-write access. Any other
    word is rejected with a warning, and the shader cannot be used.

    \note The declaration is recognized by scanning the shader source before
    any preprocessing takes place. The memory qualifiers must therefore be
    written as a literal string, and the name as a plain identifier;
    \c{#define}'d values are not recognized. Each name can be declared only
    once in a shader, even in different preprocessor branches, while
    declarations inside comments are ignored. Like other GLSL interface
    blocks, the block can end with a semicolon. A declaration that does not
    follow this form is left in the shader source unprocessed, which
    typically shows up as a shader compilation error.

    \sa ShaderStorageBufferData, ComputePass, ShaderImage
*/

QQuick3DShaderStorageBuffer::QQuick3DShaderStorageBuffer(QQuick3DObject *parent)
    : QQuick3DObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::ShaderStorageBuffer, QQuick3DObjectPrivate::Flags::RequiresSecondaryUpdate)), parent)
{

}

QQuick3DShaderStorageBuffer::~QQuick3DShaderStorageBuffer()
{
    QObject::disconnect(m_modifiedConnection);
}

/*!
    \qmlproperty int ShaderStorageBuffer::size
    This property holds the size of the buffer, in bytes.

    Setting \l bufferData automatically updates this property to match the
    size of the assigned data; setting \c size directly is meant for
    buffers whose content is produced entirely on the GPU, without a
    \l bufferData. Setting \c size is ignored if \l bufferData or \l bufferProvider
    is also set.
*/
quint32 QQuick3DShaderStorageBuffer::size() const
{
    return m_size;
}

/*!
    \qmlproperty RenderExtension ShaderStorageBuffer::bufferProvider
    This property holds a \l RenderExtension that is responsible for
    providing this buffer's backing GPU resource, for advanced use cases
    where the buffer needs to be created and managed by custom C++
    rendering code rather than by the engine.

    \note When \c bufferProvider is set, the \l size is \e{not} automatically updated to
    match the size provided, but setting size manually is still ignored.
*/
QQuick3DRenderExtension *QQuick3DShaderStorageBuffer::bufferProvider() const
{
    return m_provider;
}

/*!
    \qmlproperty ShaderStorageBufferData ShaderStorageBuffer::bufferData
    This property holds the \l ShaderStorageBufferData providing this buffer's
    content from the CPU. Assigning it uploads the data returned by
    \l{QQuick3DShaderStorageBufferData::}{bufferData()} to the GPU and updates \l size to
    match; setting it to \c null resets \l size to \c 0.
*/
QQuick3DShaderStorageBufferData *QQuick3DShaderStorageBuffer::bufferData() const
{
    return m_data;
}

void QQuick3DShaderStorageBuffer::setBufferProvider(QQuick3DRenderExtension *provider)
{
    if (m_provider == provider)
        return;

    QQuick3DObjectPrivate::attachWatcher(this, &QQuick3DShaderStorageBuffer::setBufferProvider, provider, m_provider);

    m_provider = provider;

    m_dirtyFlags.setFlag(DirtyFlag::ExtensionDirty);

    emit bufferProviderChanged();
    update();
}

void QQuick3DShaderStorageBuffer::setSize(quint32 size)
{
    if (m_data || m_provider) {
        qWarning() << "Size is set automatically from bufferData or bufferProvider. Setting size manually ignored.";
        return;
    }
    setSizePriv(size);
}

void QQuick3DShaderStorageBuffer::setSizePriv(quint32 size)
{
    if (m_size == size) return;
    m_size = size;
    m_dirtyFlags.setFlag(DirtyFlag::SizeDirty);
    emit sizeChanged();
    update();
}

void QQuick3DShaderStorageBuffer::setBufferData(QQuick3DShaderStorageBufferData *data)
{
    if (m_data == data) return;
    QObject::disconnect(m_modifiedConnection);
    QObject::disconnect(m_destroyedConnection);
    m_data = data;
    if (data) {
        m_destroyedConnection = QObject::connect(data, &QObject::destroyed, this, [this]() {
            setBufferData(nullptr);
        });
        m_modifiedConnection = QObject::connect(data, &QQuick3DShaderStorageBufferData::bufferDataDirty, this, [&](){
            m_dirtyFlags.setFlag(DirtyFlag::DataDirty);
            setSizePriv(m_data->bufferData().size());
            update();
        });
        setSizePriv(m_data->bufferData().size());
    } else {
        setSizePriv(0);
    }
    m_dirtyFlags.setFlag(DirtyFlag::DataDirty);
    emit bufferDataChanged();
    update();
}

void QQuick3DShaderStorageBuffer::markAllDirty()
{
    m_dirtyFlags = DirtyFlags(0xFFFF);
    QQuick3DObject::markAllDirty();
}

void QQuick3DShaderStorageBuffer::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    QQuick3DObject::itemChange(change, value);
    if (change == QQuick3DObject::ItemChange::ItemSceneChange) {
        if (m_provider) {
            const auto &sceneManager = value.sceneManager;
            if (sceneManager)
                QQuick3DObjectPrivate::refSceneManager(m_provider, *sceneManager);
            else
                QQuick3DObjectPrivate::derefSceneManager(m_provider);
        }
    }
}

QSSGRenderGraphObject *QQuick3DShaderStorageBuffer::updateSpatialNode(QSSGRenderGraphObject *node)
{
    if (!node) {
        markAllDirty();
        node = new QSSGRenderBuffer();
    }
    QQuick3DObject::updateSpatialNode(node);
    auto bufferNode = static_cast<QSSGRenderBuffer *>(node);

    if (m_dirtyFlags.testFlag(DirtyFlag::SizeDirty)) {
        m_dirtyFlags.setFlag(DirtyFlag::SizeDirty, false);

        bufferNode->m_size = m_size;
        bufferNode->m_flags.setFlag(QSSGRenderBuffer::Flag::Dirty);
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::ExtensionDirty)) {
        QSSGRenderExtension *extensionsSource = nullptr;
        bool waitingForProvider = false;
        if (m_provider) {
            auto *sn = QQuick3DObjectPrivate::get(m_provider)->spatialNode;
            // NOTE: We don't clear if we haven't gotten the spatial node yet, as
            // we'll be called once _again_ when the extensions have been processed.
            waitingForProvider = (sn == nullptr);
            if (sn && QSSG_GUARD(QSSGRenderGraphObject::isExtension(sn->type)))
                extensionsSource = static_cast<QSSGRenderExtension *>(sn);
        }

        if (!waitingForProvider && bufferNode->m_extensionsSource != extensionsSource) {
            bufferNode->m_extensionsSource = extensionsSource;
            bufferNode->m_flags.setFlag(QSSGRenderBuffer::Flag::Dirty);
        }

        if (waitingForProvider)
            QQuick3DObjectPrivate::get(this)->requestSecondaryUpdate();
        m_dirtyFlags.setFlag(DirtyFlag::ExtensionDirty, waitingForProvider);
    }
    if (m_dirtyFlags.testFlag(DirtyFlag::DataDirty)) {
        if (m_data)
            bufferNode->m_data = m_data->bufferData();
        else
            bufferNode->m_data.clear();
        bufferNode->m_flags.setFlag(QSSGRenderBuffer::Flag::Dirty);
        bufferNode->m_flags.setFlag(QSSGRenderBuffer::Flag::DataDirty);
        m_dirtyFlags.setFlag(DirtyFlag::DataDirty, false);
    }

    return bufferNode;
}

QT_END_NAMESPACE
