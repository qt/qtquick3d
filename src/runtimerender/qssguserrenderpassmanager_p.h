// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGUSERRENDERPASSMANAGER_P_H
#define QSSGUSERRENDERPASSMANAGER_P_H

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

#include <QtQuick3DRuntimeRender/private/qssgrenderuserpass_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhicontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercommands_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGUserRenderPassManager
{
public:
    // A set of user passes (no duplicates)
    using UserPassSet = QList<QSSGRenderUserPass *>;

    const QSSGRenderContextInterface &m_context;

    explicit QSSGUserRenderPassManager(const QSSGRenderContextInterface &inContext);
    ~QSSGUserRenderPassManager();

    void scheduleUserPass(QSSGRenderUserPass *userPasses);

    QSSGRhiRenderableTextureV2Ptr getOrCreateRenderableTexture(const QSSGRenderUserPass &userPass);
    QSSGRhiRenderableTextureV2Ptr getUserPassTexureResult(const QSSGRenderUserPass &userPass) const;

    // Should be called before scheduledUserPasses is called.
    void updateUserPassOrder(bool forceUpdate = false);

    const UserPassSet &scheduledUserPasses() const { return m_scheduledUserPasses; }

    void derefTexture(QRhiTexture *texture);
    void refTexture(QRhiTexture *texture);

private:
    QHash<const QSSGRenderUserPass *, QSSGRhiRenderableTextureV2Ptr> m_renderPassRenderTargets;
    UserPassSet m_scheduledUserPasses;

    QHash<QRhiTexture *, size_t> m_trackedTextures;

    bool m_passlistDirty = false;
};

using QSSGUserRenderPassManagerPtr = std::shared_ptr<QSSGUserRenderPassManager>;

QT_END_NAMESPACE

#endif // QSSGUSERRENDERPASSMANAGER_P_H
