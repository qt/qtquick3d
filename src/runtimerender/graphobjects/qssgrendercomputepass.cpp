// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#include "qssgrendercomputepass_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>
#include <QtQuick3DRuntimeRender/private/qssgcomputemanager_p.h>

#include <ssg/qssgrendercontextcore.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcRenderComputePass, "qt.quick3d.runtimerender.computepass")

QSSGRenderComputePass::QSSGRenderComputePass()
    : QSSGRenderUserPassBase(Type::ComputePass)
{

}

QSSGRenderComputePass::~QSSGRenderComputePass()
{
    resetCommands();
}

// Mirrors the checks prepareComputePass() skips the pass on: only the first
// Dispatch counts, and a zero group count dispatches nothing.
bool QSSGRenderComputePass::willDispatch() const
{
    if (!isReady())
        return false;
    for (const QSSGCommand *command : commands) {
        if (command->m_type == CommandType::Dispatch) {
            const auto &groupCount = static_cast<const QSSGDispatch *>(command)->m_groupCount;
            return groupCount.x != 0 && groupCount.y != 0 && groupCount.z != 0;
        }
    }
    return false;
}

void QSSGRenderComputePass::finalizeShaders(const QSSGRenderContextInterface &ctx)
{
    Q_UNUSED(ctx);
    if (!isDirty(QSSGRenderComputePass::DirtyFlag::ShaderDirty))
        return;

    clearDirty(DirtyFlag::ShaderDirty);

    qCDebug(lcRenderComputePass) << "Finalizing shaders for compute pass shader code.";
    if (finalizeComputeShader(computeShader)) {
        m_state = State::Ready;
    } else if (!computeShader.body.isEmpty()) {
        m_state = State::Error;
        qWarning() << "Finalizing compute pass shaders failed.";
    } else {
        m_state = State::None;
    }
}

QT_END_NAMESPACE
