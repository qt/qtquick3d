// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

    struct BlockMember {
        QByteArray type;
        QByteArray name;
        QSSGRenderShaderMetadata::Uniform::Condition conditionType;
        QByteArray conditionName;
    };

    // Using QMap intentionally - while it is not strictly required to use an
    // ordered map, being sorted by key when iterating is helpful to get the
    // same ordered list of vertex inputs, uniforms, etc. on every run, which
    // in turn helps shader (disk) cache efficiency due to not generating a
    // different shader string just because QHash decided to iterate entries in
    // a different order.
    QMap<QByteArray, InOutVar> m_inOutVars;
    QMap<QByteArray, Sampler> m_samplers;
    QMap<QByteArray, BlockMember> m_uniformMembers;

    int m_nextFreeResourceBinding = FIRST_CUSTOM_RESOURCE_BINDING_POINT;
    QHash<int, int> m_nextFreeInLocation;
    QHash<int, int> m_nextFreeOutLocation;

    int viewCount = 1;

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
        if (m_samplers.contains(name))
            return;
        Sampler var { type, name, conditionType, conditionName, m_nextFreeResourceBinding++ };
        m_samplers.insert(name, var);
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
};

QT_END_NAMESPACE

#endif
