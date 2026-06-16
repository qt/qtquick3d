// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSGRENDERCOMPUTEPASS_P_H
#define QSSGRENDERCOMPUTEPASS_P_H

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
#include <QSet>
#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderuserpass_p.h>

#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderComputePass : public QSSGRenderUserPassBase
{
public:
    QSSGRenderComputePass();
    ~QSSGRenderComputePass();

    void finalizeShaders(const QSSGRenderContextInterface &ctx) override;
    bool willDispatch() const;
    void clearWarnings()
    {
        dispatchGroupsWarningShown = false;
        dispatchMultiWarningShown = false;
        dispatchMissingWarningShown = false;
        multiviewWarningShown = false;
        pipelineWarningShown = false;
        bindingWarningShown = false;
    }

    QSSGUserComputeShader computeShader;
    bool dispatchGroupsWarningShown = false;
    bool dispatchMultiWarningShown = false;
    bool dispatchMissingWarningShown = false;
    bool multiviewWarningShown = false;
    bool pipelineWarningShown = false;
    bool bindingWarningShown = false;
};

using QSSGRenderComputePassPtr = std::shared_ptr<QSSGRenderComputePass>;

QT_END_NAMESPACE

#endif // QSSGRENDERCOMPUTEPASS_P_H
