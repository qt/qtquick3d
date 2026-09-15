// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSGSHADERRESOURCEMERGECONTEXT_P_H
#define QSSGSHADERRESOURCEMERGECONTEXT_P_H

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

#include "qssgrendershadercodegenerator_p.h"
#include "qssgrendershadermetadata_p.h"

QT_BEGIN_NAMESPACE

class QSSGShaderResourceMergeContext
{
public:

    QSSGShaderResourceMergeContext()
        : m_nextFreeResourceBinding(FIRST_CUSTOM_RESOURCE_BINDING_POINT + s_additionalBuffers)
    {
    }
    // Resource bindings 0..2 are reserved for uniform buffers.
    // (0 is cbMain, 1 is cbLights)
    static const int FIRST_CUSTOM_RESOURCE_BINDING_POINT = 3;

    struct InOutVar {
        QSSGShaderGeneratorStageFlags stageOutputFrom;
        QSSGShaderGeneratorStageFlags stagesInputIn;
        QByteArray type;
        QByteArray name;
        int location;
        bool output;
        bool flat;
    };

    struct Sampler {
        QByteArray type;
        QByteArray name;
        QSSGRenderShaderMetadata::Uniform::Condition conditionType;
        QByteArray conditionName;
        int binding;
    };

    struct Image : public Sampler
    {
        QByteArray imgType;
        QSSGMemoryQualifiers qualifiers;
        QSSGShaderGeneratorStageFlags stages;
    };

    struct BlockMember {
        QByteArray type;
        QByteArray name;
        QSSGRenderShaderMetadata::Uniform::Condition conditionType;
        QByteArray conditionName;
    };

    struct Buffer : public BlockMember
    {
        int binding;
        QByteArray contentType;
        QSSGMemoryQualifiers qualifiers;
        QSSGShaderGeneratorStageFlags stages;
    };

    // Using QMap intentionally - while it is not strictly required to use an
    // ordered map, being sorted by key when iterating is helpful to get the
    // same ordered list of vertex inputs, uniforms, etc. on every run, which
    // in turn helps shader (disk) cache efficiency due to not generating a
    // different shader string just because QHash decided to iterate entries in
    // a different order.
    QMap<QByteArray, InOutVar> m_inOutVars;
    QMap<QByteArray, Sampler> m_samplers;
    QMap<QByteArray, Image> m_images;
    QMap<QByteArray, Buffer> m_buffers;
    QMap<QByteArray, BlockMember> m_uniformMembers;

    int m_nextFreeResourceBinding = FIRST_CUSTOM_RESOURCE_BINDING_POINT;
    int m_nextFreeImageBinding = 0;
    QHash<int, int> m_nextFreeInLocation;
    QHash<int, int> m_nextFreeOutLocation;

    int viewCount = 1;

    void rearrangeResources()
    {
        // Put images and storage buffers before samplers. QRhi's OpenGL
        // backend uses the binding number directly as the image unit and as
        // the shader storage buffer binding point, and the guaranteed minimum
        // for both is only 8 (GL_MAX_IMAGE_UNITS, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS).
        // These are separate namespaces in OpenGL, but QRhi needs unique binding
        // numbers across all resource types, so the numbers used by uniform buffers (0..2),
        // images and buffers all count against both limits. Linked list OIT adds three
        // images, which leaves two bindings for custom images and buffers.
        // Samplers and uniform buffers are not affected: QRhi assigns texture
        // units itself and sets uniform block members as plain uniforms.
        int binding = FIRST_CUSTOM_RESOURCE_BINDING_POINT + s_additionalBuffers;
        for (auto &image : m_images)
            image.binding = binding++;
        for (auto &buffer : m_buffers)
            buffer.binding = binding++;
        for (auto &sampler : m_samplers)
            sampler.binding = binding++;
    }

    void registerInput(QSSGShaderGeneratorStage stage, const QByteArray &type, const QByteArray &name, bool flat = false)
    {
        auto it = m_inOutVars.find(name);
        if (it != m_inOutVars.end()) {
            it->stagesInputIn |= stage;
            return;
        }
        InOutVar var { {}, stage, type, name, m_nextFreeInLocation[int(stage)]++, false, flat };
        m_inOutVars.insert(name, var);
    }

    void registerOutput(QSSGShaderGeneratorStage stage, const QByteArray &type, const QByteArray &name, bool flat = false)
    {
        auto it = m_inOutVars.find(name);
        if (it != m_inOutVars.end()) {
            it->stageOutputFrom |= stage;
            return;
        }
        InOutVar var { stage, {}, type, name, m_nextFreeOutLocation[int(stage)]++, true, flat };
        m_inOutVars.insert(name, var);
    }

    void registerSampler(const QByteArray &type,
                         const QByteArray &name,
                         QSSGRenderShaderMetadata::Uniform::Condition conditionType = QSSGRenderShaderMetadata::Uniform::None,
                         const QByteArray &conditionName = QByteArray())
    {
        if (m_images.contains(name) || m_buffers.contains(name)) {
            qWarning("Sampler %s name conflicts with existing image or buffer.", name.constData());
            return;
        }
        if (m_samplers.contains(name))
            return;
        Sampler var { type, name, conditionType, conditionName, m_nextFreeResourceBinding++ };
        m_samplers.insert(name, var);
    }

    void registerImage(QSSGShaderGeneratorStage stage,
                       const QByteArray &type,
                       const QByteArray &name,
                       const QByteArray &imgtype,
                       QSSGMemoryQualifiers qualifiers,
                       QSSGRenderShaderMetadata::Uniform::Condition conditionType = QSSGRenderShaderMetadata::Uniform::None,
                       const QByteArray &conditionName = QByteArray())
    {
        if (m_samplers.contains(name) || m_buffers.contains(name)) {
            qWarning("Image %s name conflicts with existing sampler or buffer.", name.constData());
            return;
        }
        auto iter = m_images.find(name);
        if (iter != m_images.end()) {
            auto &img = *iter;
            if (img.type != type || img.imgType != imgtype || img.qualifiers != qualifiers)
                qWarning("Image %s is declared differently between shader stages.", name.constData());
            if (img.conditionType != conditionType || img.conditionName != conditionName)
                qWarning("Image %s condition doesn't match between shader stages.", name.constData());
            img.stages |= stage;
            return;
        }
        Image var {{type, name, conditionType, conditionName, m_nextFreeResourceBinding++}, imgtype, qualifiers, stage};
        m_images.insert(name, var);
    }
    void registerBuffer(QSSGShaderGeneratorStage stage,
                        const QByteArray &contentType,
                        const QByteArray &name,
                        QSSGMemoryQualifiers qualifiers,
                        QSSGRenderShaderMetadata::Uniform::Condition conditionType = QSSGRenderShaderMetadata::Uniform::None,
                        const QByteArray &conditionName = QByteArray())
    {
        if (m_samplers.contains(name) || m_images.contains(name)) {
            qWarning("Buffer %s name conflicts with existing sampler or image.", name.constData());
            return;
        }
        auto iter = m_buffers.find(name);
        if (iter != m_buffers.end()) {
            auto &buf = *iter;
            if (buf.contentType.simplified() != contentType.simplified() || buf.qualifiers != qualifiers)
                qWarning("Storage buffer %s is declared differently between shader stages.", name.constData());
            if (buf.conditionType != conditionType || buf.conditionName != conditionName)
                qWarning("Storage buffer %s condition doesn't match between shader stages.", name.constData());
            buf.stages |= stage;
            return;
        }
        Buffer var {{"buffer", name, conditionType, conditionName}, m_nextFreeResourceBinding++, contentType, qualifiers, stage};
        m_buffers.insert(name, var);
    }

    void registerUniformMember(const QByteArray &type,
                               const QByteArray &name,
                               QSSGRenderShaderMetadata::Uniform::Condition conditionType = QSSGRenderShaderMetadata::Uniform::None,
                               const QByteArray &conditionName = QByteArray())
    {
        auto it = m_uniformMembers.constFind(name);
        if (it != m_uniformMembers.constEnd()) {
            if (it->conditionType != conditionType) {
                qWarning("Encountered uniform %s with different conditions, this is not supported.",
                         name.constData());
            }
            return;
        }
        BlockMember var { type, name, conditionType, conditionName };
        m_uniformMembers.insert(name, var);
    }
    static int s_additionalBuffers;
    static void setAdditionalBufferAmount(int amount)
    {
        s_additionalBuffers = amount;
    }
};

QT_END_NAMESPACE

#endif
