// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QSSGRENDERUSERPASS_P_H
#define QSSGRENDERUSERPASS_P_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QSSGCommand;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderUserPassBase : public QSSGRenderGraphObject
{
public:
    enum class DirtyFlag : quint8
    {
        ShaderDirty = 0x1
    };
    using FlagT = std::underlying_type_t<DirtyFlag>;

    enum class State : quint8
    {
        None,
        Ready,
        Error,
    };

    static constexpr DirtyFlag DirtyMask { std::numeric_limits<FlagT>::max() };

    bool isDirty(DirtyFlag flag = DirtyMask) const
    {
        return (m_dirtyFlags & FlagT(flag)) != 0;
    }
    void markDirty(DirtyFlag flag)
    {
        m_dirtyFlags |= FlagT(flag);
    }
    void clearDirty(DirtyFlag flag)
    {
        m_dirtyFlags &= ~FlagT(flag);
    }

    bool isReady() const { return (m_state == State::Ready); }

    // True for a RenderPass used as a sub-pass. Only render passes can be sub-passes.
    bool isSubPass() const;

    QVector<QSSGCommand *> commands;

    FlagT m_dirtyFlags = 0;
    State m_state = State::None;

    // Number of RenderPass ancestors of the frontend item. Deeper-nested passes
    // render first ("children before parents"), so a pass declared inside its
    // consumer renders before it. Derived at sync when the scene manager's pass
    // set is dirty.
    quint32 m_nestingDepth = 0;
    // Registration order in the scene manager's userRenderPasses list. Used as a
    // stable tiebreak when ordering scheduled passes with equal nesting depth,
    // so the render order does not depend on when a provider scheduled a pass.
    quint32 m_declarationOrder = 0;

    ~QSSGRenderUserPassBase() override;

    virtual void finalizeShaders(const QSSGRenderContextInterface &ctx)
    {
        Q_UNUSED(ctx);
    }
    void resetCommands();

protected:
    explicit QSSGRenderUserPassBase(Type type)
        : QSSGRenderGraphObject(type, FlagT(Flags::HasGraphicsResources))
    {
    }
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGRenderUserPass : public QSSGRenderUserPassBase
{
public:
    enum MaterialModes {
        OriginalMaterial,
        AugmentMaterial,
        OverrideMaterial
    };

    enum PassModes {
        UserPass,
        SkyboxPass,
        Item2DPass,
        InfiniteGridPass
    };

    enum class Role : quint8 {
        TopLevel,
        SubPass
    };

    QSSGRenderUserPass();
    ~QSSGRenderUserPass();

    // Replaces the augment macros (e.g. BASE_COLOR) in \a shaderCode in-place with their
    // built-in variable names, setting the corresponding feature flags on \a augmentation.
    // Exposed for testing; normally called via finalizeShaders().
    static void replaceAugmentMacros(QByteArray &shaderCode, QSSGUserShaderAugmentation &augmentation);

    void finalizeShaders(const QSSGRenderContextInterface &ctx) override;

    QColor clearColor = Qt::black;
    QRhiTextureRenderTarget::Flags renderTargetFlags = {};

    // Material Mode
    MaterialModes materialMode = OriginalMaterial;
    QSSGRenderGraphObject *overrideMaterial = nullptr;
    QSSGUserShaderAugmentation shaderAugmentation;
    QRhiDepthStencilClearValue depthStencilClearValue = { };
    PassModes passMode = UserPass;
    Role role = Role::TopLevel;
};

inline bool QSSGRenderUserPassBase::isSubPass() const
{
    return type == Type::RenderPass
            && static_cast<const QSSGRenderUserPass *>(this)->role == QSSGRenderUserPass::Role::SubPass;
}

using QSSGRenderUserPassPtr = std::shared_ptr<QSSGRenderUserPass>;

QT_END_NAMESPACE

#endif // QSSGRENDERUSERPASS_P_H
