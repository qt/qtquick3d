// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssguserrenderpassmanager_p.h"

QT_BEGIN_NAMESPACE

QSSGUserRenderPassManager::QSSGUserRenderPassManager(const QSSGRenderContextInterface &inContext)
    : m_context(inContext)
{

}

QSSGUserRenderPassManager::~QSSGUserRenderPassManager()
{
    for (auto &rt : m_renderPassRenderTargets)
        rt.reset();

    const auto textures = m_trackedTextures.keys();
    qDeleteAll(textures);
}

void QSSGUserRenderPassManager::scheduleUserPass(QSSGRenderUserPass *userPasses)
{
    if (userPasses != nullptr) {
        auto it = std::find(m_scheduledUserPasses.begin(), m_scheduledUserPasses.end(), userPasses);
        if (it == m_scheduledUserPasses.end()) {
            m_scheduledUserPasses.push_back(userPasses);
            m_passlistDirty = true;
        }
    }
}

QSSGRhiRenderableTextureV2Ptr QSSGUserRenderPassManager::getOrCreateRenderableTexture(const QSSGRenderUserPass &userPass)
{
    auto it = m_renderPassRenderTargets.find(&userPass);
    if (it != m_renderPassRenderTargets.end())
        return it.value();

    auto iter = m_renderPassRenderTargets.insert(&userPass, std::make_shared<QSSGRhiRenderableTextureV2>(this, QSSGRhiRenderableTextureV2::Private::Initialize));
    return iter.value();
}

QSSGRhiRenderableTextureV2Ptr QSSGUserRenderPassManager::getUserPassTexureResult(const QSSGRenderUserPass &userPass) const
{
    const auto foundIt = m_renderPassRenderTargets.find(&userPass);
    if (foundIt != m_renderPassRenderTargets.end())
        return foundIt.value();

    return {};
}

void QSSGUserRenderPassManager::updateUserPassOrder(bool forceUpdate)
{
    if (m_passlistDirty || forceUpdate) {
        std::sort(m_scheduledUserPasses.begin(), m_scheduledUserPasses.end(), [](const QSSGRenderUserPass *a, const QSSGRenderUserPass *b) {
            return a->m_dependencyIndex > b->m_dependencyIndex;
        });
    }

    m_passlistDirty = false;
}

void QSSGUserRenderPassManager::derefTexture(QRhiTexture *texture)
{
    auto foundIt = m_trackedTextures.find(texture);

    if (foundIt != m_trackedTextures.constEnd() && !(foundIt.value() > 1)) {
        m_trackedTextures.erase(foundIt);
        delete texture;
    } else if (foundIt != m_trackedTextures.constEnd()) {
        foundIt.value() -= 1;
    }
}

void QSSGUserRenderPassManager::refTexture(QRhiTexture *texture)
{
    auto it = m_trackedTextures.find(texture);
    if (it != m_trackedTextures.end()) {
        it.value() += 1;
    } else {
        m_trackedTextures.insert(texture, 1);
    }
}

QT_END_NAMESPACE
