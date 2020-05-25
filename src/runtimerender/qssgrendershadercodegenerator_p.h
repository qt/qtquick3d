/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
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

#ifndef QSSG_RENDER_SHADER_CODE_GENERATOR_V2_H
#define QSSG_RENDER_SHADER_CODE_GENERATOR_V2_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercache_p.h>

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

// TODO: !!! Find a better solution then having these globally shared like this
typedef QPair<QByteArray, QByteArray> TParamPair;
typedef QPair<QByteArray, TParamPair> TConstantBufferParamPair;
typedef QVector<TConstantBufferParamPair> TConstantBufferParamArray;
typedef QHash<QByteArray, QByteArray> TStrTableStrMap;

enum class QSSGShaderGeneratorStage
{
    None = 0,
    Vertex = 1,
    Geometry = 1 << 1,
    Fragment = 1 << 2,
    StageCount = 3,
};

Q_DECLARE_FLAGS(QSSGShaderGeneratorStageFlags, QSSGShaderGeneratorStage)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSSGShaderGeneratorStageFlags)

struct QSSGStageGeneratorBase;
class QSSGRenderContextInterface;

class QSSGShaderResourceMergeContext;

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGStageGeneratorBase
{
    enum class ShaderItemType {
        VertexInput,
        Input,
        Output,
        Uniform
    };

    TStrTableStrMap m_incoming;
    TStrTableStrMap *m_outgoing;
    QSet<QByteArray> m_includes;
    TStrTableStrMap m_uniforms;
    TStrTableStrMap m_constantBuffers;
    TConstantBufferParamArray m_constantBufferParams;
    QByteArray m_codeBuilder;
    QByteArray m_finalBuilder;
    QSSGShaderGeneratorStage m_stage;
    QSSGShaderGeneratorStageFlags m_enabledStages;
    QList<QByteArray> m_addedFunctions;
    QSSGShaderResourceMergeContext *m_mergeContext = nullptr;

    explicit QSSGStageGeneratorBase(QSSGShaderGeneratorStage inStage);
    virtual ~QSSGStageGeneratorBase() = default;

    virtual void begin(QSSGShaderGeneratorStageFlags inEnabledStages);

    virtual void addIncoming(const QByteArray &name, const QByteArray &type);

    virtual void addOutgoing(const QByteArray &name, const QByteArray &type);

    virtual void addUniform(const QByteArray &name, const QByteArray &type);

    virtual void addConstantBuffer(const QByteArray &name, const QByteArray &layout);
    virtual void addConstantBufferParam(const QByteArray &cbName, const QByteArray &paramName, const QByteArray &type);

    virtual QSSGStageGeneratorBase &operator<<(const QByteArray &data);
    virtual void append(const QByteArray &data);
    QSSGShaderGeneratorStage stage() const;

    void addShaderPass2Marker(ShaderItemType itemType);

    void addShaderItemMap(ShaderItemType itemType,
                          const TStrTableStrMap &itemMap,
                          const QByteArray &inItemSuffix = QByteArray());

    virtual void addShaderIncomingMap();

    virtual void addShaderUniformMap();

    virtual void addShaderOutgoingMap();

    virtual void addShaderConstantBufferItemMap(const QByteArray &itemType, const TStrTableStrMap &cbMap, TConstantBufferParamArray cbParamsArray);

    virtual void appendShaderCode() final;

    virtual void updateShaderCacheFlags(QSSGShaderCacheProgramFlags &);

    virtual void addInclude(const QByteArray &name) final;

    void buildShaderSourcePass1(QSSGShaderResourceMergeContext *mergeContext);

    QByteArray buildShaderSourcePass2(QSSGShaderResourceMergeContext *mergeContext);

    virtual void addFunction(const QByteArray &functionName) final;
};

struct QSSGVertexShaderGenerator final : public QSSGStageGeneratorBase
{
    QSSGVertexShaderGenerator();
};

struct QSSGGeometryShaderGenerator final : public QSSGStageGeneratorBase
{
    QSSGGeometryShaderGenerator();
    void addShaderIncomingMap() override;
    void addShaderOutgoingMap() override;
    void updateShaderCacheFlags(QSSGShaderCacheProgramFlags &inFlags) override;
};

struct QSSGFragmentShaderGenerator final : public QSSGStageGeneratorBase
{
    QSSGFragmentShaderGenerator();
    void addShaderIncomingMap() override;
    void addShaderOutgoingMap() override;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGProgramGenerator
{
    QAtomicInt ref;
    QSSGRenderContextInterface *m_context;
    QSSGVertexShaderGenerator m_vs;
    QSSGGeometryShaderGenerator m_gs;
    QSSGFragmentShaderGenerator m_fs;

    QSSGShaderGeneratorStageFlags m_enabledStages;

    static constexpr QSSGShaderGeneratorStageFlags defaultFlags() { return QSSGShaderGeneratorStageFlags(QSSGShaderGeneratorStage::Vertex | QSSGShaderGeneratorStage::Fragment); }

    explicit QSSGProgramGenerator(QSSGRenderContextInterface *inContext);
    ~QSSGProgramGenerator() = default;

    void linkStages();

    void beginProgram(QSSGShaderGeneratorStageFlags inEnabledStages = defaultFlags());

    QSSGShaderGeneratorStageFlags getEnabledStages() const;

    QSSGStageGeneratorBase &internalGetStage(QSSGShaderGeneratorStage inStage);
    // get the stage or nullptr if it has not been created.
    QSSGStageGeneratorBase *getStage(QSSGShaderGeneratorStage inStage);

    void registerShaderMetaDataFromSource(QSSGShaderResourceMergeContext *mergeContext,
                                          const QByteArray &contents,
                                          QSSGShaderGeneratorStage stage);

    QSSGRef<QSSGRhiShaderStages> compileGeneratedRhiShader(const QByteArray &inShaderName,
                                                           const QSSGShaderCacheProgramFlags &inFlags,
                                                           const ShaderFeatureSetList &inFeatureSet);

    QSSGRef<QSSGRhiShaderStages> loadBuiltinRhiShader(const QByteArray &inShaderName);
};

QT_END_NAMESPACE
#endif
