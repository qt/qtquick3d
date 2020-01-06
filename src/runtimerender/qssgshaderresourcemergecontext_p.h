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

#include "qssgrendershadercodegeneratorv2_p.h"
#include "qssgrendershadermetadata_p.h"

QT_BEGIN_NAMESPACE

class QSSGShaderResourceMergeContext
{
public:
    // Resource bindings 0..2 are reserved for uniform buffers. (0 is cbMain, 1 is
    // cbBufferLights, 2 is cbBufferAreaLights, when present)
    static const int FIRST_CUSTOM_RESOURCE_BINDING_POINT = 3;

    struct InOutVar {
        QByteArray type;
        QByteArray name;
        int location;
        bool output;
    };

    struct Sampler {
        QByteArray type;
        QByteArray name;
        int binding;
    };

    struct BlockMember {
        QByteArray type;
        QByteArray name;
        QSSGRenderShaderMetadata::Uniform::Condition conditionType;
        QByteArray conditionName;
    };

    QHash<QByteArray, InOutVar> m_vertexInputs;
    QHash<QByteArray, InOutVar> m_inOutVars;
    QHash<QByteArray, Sampler> m_samplers;

    // not strictly required to use an ordered map, just to avoid differences
    // between runs in the layout
    QMap<QByteArray, BlockMember> m_uniformMembers;

    int m_nextFreeVertexInputLocation = 0;
    int m_nextFreeInOutLocation = 0;
    int m_nextFreeResourceBinding = FIRST_CUSTOM_RESOURCE_BINDING_POINT;

    void registerVertexInput(const QByteArray &type, const QByteArray &name)
    {
        if (m_vertexInputs.contains(name))
            return;
        InOutVar var { type, name, m_nextFreeVertexInputLocation++, false };
        m_vertexInputs.insert(name, var);
    }

    void registerInput(const QByteArray &type, const QByteArray &name)
    {
        if (m_inOutVars.contains(name))
            return;
        InOutVar var { type, name, m_nextFreeInOutLocation++, false };
        m_inOutVars.insert(name, var);
    }

    void registerOutput(const QByteArray &type, const QByteArray &name)
    {
        if (m_inOutVars.contains(name))
            return;
        InOutVar var { type, name, m_nextFreeInOutLocation++, true };
        m_inOutVars.insert(name, var);
    }

    void registerSampler(const QByteArray &type, const QByteArray &name)
    {
        if (m_samplers.contains(name))
            return;
        Sampler var { type, name, m_nextFreeResourceBinding++ };
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
