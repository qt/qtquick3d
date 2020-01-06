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

#include "qssgrendershadercodegeneratorv2_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderdynamicobjectsystem_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshaderresourcemergecontext_p.h>

#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

namespace {

struct QSSGStageGeneratorBase : public QSSGShaderStageGeneratorInterface
{
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
    bool m_rhiCompatible;

    struct {
        TStrTableStrMap vertexInputs;
        TStrTableStrMap inputs;
        TStrTableStrMap outputs;
        TStrTableStrMap samplers;
        void clear() {
            vertexInputs.clear();
            inputs.clear();
            outputs.clear();
            samplers.clear();
        }
    } m_registeredVars;
    QSSGShaderResourceMergeContext *m_mergeContext = nullptr;

    QSSGStageGeneratorBase(QSSGShaderGeneratorStage inStage, bool rhiCompatible)

        : m_outgoing(nullptr), m_stage(inStage), m_rhiCompatible(rhiCompatible)
    {
    }

    virtual void begin(QSSGShaderGeneratorStageFlags inEnabledStages)
    {
        m_incoming.clear();
        m_outgoing = nullptr;
        m_includes.clear();
        m_uniforms.clear();
        m_constantBuffers.clear();
        m_constantBufferParams.clear();
        m_codeBuilder.clear();
        m_finalBuilder.clear();
        m_enabledStages = inEnabledStages;
        m_addedFunctions.clear();
        m_registeredVars.clear();
        // the shared buffers will be cleared elsewhere.
    }

    void addIncoming(const QByteArray &name, const QByteArray &type) override
    {
        m_incoming.insert(name, type);
    }

    virtual const QByteArray GetIncomingVariableName() { return "in"; }

    void addOutgoing(const QByteArray &name, const QByteArray &type) override
    {
        if (m_outgoing == nullptr) {
            Q_ASSERT(false);
            return;
        }
        m_outgoing->insert(name, type);
    }

    void addUniform(const QByteArray &name, const QByteArray &type) override
    {
        m_uniforms.insert(name, type);
    }

    void addConstantBuffer(const QByteArray &name, const QByteArray &layout) override
    {
        m_constantBuffers.insert(name, layout);
    }
    void addConstantBufferParam(const QByteArray &cbName, const QByteArray &paramName, const QByteArray &type) override
    {
        TParamPair theParamPair(paramName, type);
        TConstantBufferParamPair theBufferParamPair(cbName, theParamPair);
        m_constantBufferParams.push_back(theBufferParamPair);
    }

    QSSGShaderStageGeneratorInterface &operator<<(const QByteArray &data) override
    {
        m_codeBuilder.append(data);
        return *this;
    }
    void append(const QByteArray &data) override
    {
        m_codeBuilder.append(data);
        m_codeBuilder.append("\n");
    }
    QSSGShaderGeneratorStage stage() const override { return m_stage; }

    enum class ShaderItemType {
        VertexInput,
        Input,
        Output,
        Uniform
    };

    void addShaderItemMap(ShaderItemType itemType,
                          const TStrTableStrMap &itemMap,
                          const QByteArray &inItemSuffix = QByteArray())
    {
        m_finalBuilder.append("\n");

        if (m_rhiCompatible) {
            Q_ASSERT(m_mergeContext);
            for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
                const QByteArray name = iter.key() + inItemSuffix;
                switch (itemType) {
                case ShaderItemType::VertexInput:
                    m_registeredVars.vertexInputs.insert(name, iter.value());
                    m_mergeContext->registerVertexInput(iter.value(), name);
                    break;
                case ShaderItemType::Input:
                    m_registeredVars.inputs.insert(name, iter.value());
                    m_mergeContext->registerInput(iter.value(), name);
                    break;
                case ShaderItemType::Output:
                    m_registeredVars.outputs.insert(name, iter.value());
                    m_mergeContext->registerOutput(iter.value(), name);
                    break;
                case ShaderItemType::Uniform:
                    if (iter.value().startsWith(QByteArrayLiteral("sampler"))) {
                        m_registeredVars.samplers.insert(name, iter.value());
                        m_mergeContext->registerSampler(iter.value(), name);
                    } else {
                        m_mergeContext->registerUniformMember(iter.value(), name);
                    }
                    break;
                default:
                    qWarning("Unknown shader item %d", int(itemType));
                    Q_UNREACHABLE();
                }
            }
            m_finalBuilder.append(QByteArrayLiteral("//@@") + QByteArray::number(int(itemType)) + QByteArrayLiteral("\n"));
        } else {
            for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
                switch (itemType) {
                case ShaderItemType::VertexInput:
                    m_finalBuilder.append("attribute");
                    break;
                case ShaderItemType::Input:
                    m_finalBuilder.append("varying");
                    break;
                case ShaderItemType::Output:
                    m_finalBuilder.append("varying");
                    break;
                case ShaderItemType::Uniform:
                    m_finalBuilder.append("uniform");
                    break;
                default:
                    qWarning("Unknown shader item %d", int(itemType));
                    Q_UNREACHABLE();
                }
                m_finalBuilder.append(" ");
                m_finalBuilder.append(iter.value());
                m_finalBuilder.append(" ");
                m_finalBuilder.append(iter.key());
                m_finalBuilder.append(inItemSuffix);
                m_finalBuilder.append(";\n");
            }
        }
    }

    virtual void addShaderIncomingMap()
    {
        addShaderItemMap(ShaderItemType::VertexInput, m_incoming);
    }

    virtual void addShaderUniformMap()
    {
        addShaderItemMap(ShaderItemType::Uniform, m_uniforms);
    }

    virtual void addShaderOutgoingMap()
    {
        if (m_outgoing)
            addShaderItemMap(ShaderItemType::Output, *m_outgoing);
    }

    virtual void addShaderConstantBufferItemMap(const QByteArray &itemType, const TStrTableStrMap &cbMap, TConstantBufferParamArray cbParamsArray)
    {
        m_finalBuilder.append("\n");

        // iterate over all constant buffers
        for (TStrTableStrMap::const_iterator iter = cbMap.begin(), end = cbMap.end(); iter != end; ++iter) {
            m_finalBuilder.append(iter.value());
            m_finalBuilder.append(" ");
            m_finalBuilder.append(itemType);
            m_finalBuilder.append(" ");
            m_finalBuilder.append(iter.key());
            m_finalBuilder.append(" {\n");
            // iterate over all param entries and add match
            for (TConstantBufferParamArray::const_iterator iter1 = cbParamsArray.begin(), end = cbParamsArray.end(); iter1 != end;
                 ++iter1) {
                if (iter1->first == iter.key()) {
                    m_finalBuilder.append(iter1->second.second);
                    m_finalBuilder.append(" ");
                    m_finalBuilder.append(iter1->second.first);
                    m_finalBuilder.append(";\n");
                }
            }

            m_finalBuilder.append("};\n");
        }
    }

    virtual void appendShaderCode() { m_finalBuilder.append(m_codeBuilder); }

    virtual void updateShaderCacheFlags(QSSGShaderCacheProgramFlags &) {}

    void addInclude(const QByteArray &name) override { m_includes.insert(name); }

    void buildShaderSourcePass1(QSSGShaderResourceMergeContext *mergeContext)
    {
        m_mergeContext = mergeContext;
        addShaderIncomingMap();
        addShaderUniformMap();
        addShaderConstantBufferItemMap("uniform", m_constantBuffers, m_constantBufferParams);
        addShaderOutgoingMap();
        m_mergeContext = nullptr;

        auto iter = m_includes.constBegin();
        const auto end = m_includes.constEnd();
        while (iter != end) {
            m_finalBuilder.append("#include \"");
            m_finalBuilder.append(*iter);
            m_finalBuilder.append("\"\n");
            ++iter;
        }

        appendShaderCode();
    }

    QByteArray buildShaderSourcePass2(QSSGShaderResourceMergeContext *mergeContext)
    {
        if (!m_rhiCompatible)
            return m_finalBuilder;

        static const char *prefix = "//@@";
        const int prefixLen = 4;
        const int typeLen = 1;
        int from = 0;
        for (; ;) {
            int pos = m_finalBuilder.indexOf(prefix, from);
            if (pos >= 0) {
                from = pos;
                ShaderItemType itemType = ShaderItemType(m_finalBuilder.mid(pos + prefixLen, typeLen).toInt());
                switch (itemType) {
                case ShaderItemType::VertexInput:
                {
                    QByteArray block;
                    for (TStrTableStrMap::const_iterator iter = m_registeredVars.vertexInputs.cbegin(),
                         end = m_registeredVars.vertexInputs.cend(); iter != end; ++iter)
                    {
                        const QSSGShaderResourceMergeContext::InOutVar &var(mergeContext->m_vertexInputs.value(iter.key()));
                        block += QString::asprintf("layout(location = %d) in %s %s;\n", var.location, var.type.constData(), var.name.constData()).toUtf8();
                    }
                    m_finalBuilder.replace(pos, prefixLen + typeLen, block);
                }
                    break;
                case ShaderItemType::Input:
                {
                    QByteArray block;
                    for (TStrTableStrMap::const_iterator iter = m_registeredVars.inputs.cbegin(),
                         end = m_registeredVars.inputs.cend(); iter != end; ++iter)
                    {
                        const QSSGShaderResourceMergeContext::InOutVar &var(mergeContext->m_inOutVars.value(iter.key()));
                        block += QString::asprintf("layout(location = %d) in %s %s;\n", var.location, var.type.constData(), var.name.constData()).toUtf8();
                    }
                    m_finalBuilder.replace(pos, prefixLen + typeLen, block);
                }
                    break;
                case ShaderItemType::Output:
                {
                    QByteArray block;
                    for (TStrTableStrMap::const_iterator iter = m_registeredVars.outputs.cbegin(),
                         end = m_registeredVars.outputs.cend(); iter != end; ++iter)
                    {
                        const QSSGShaderResourceMergeContext::InOutVar &var(mergeContext->m_inOutVars.value(iter.key()));
                        block += QString::asprintf("layout(location = %d) out %s %s;\n", var.location, var.type.constData(), var.name.constData()).toUtf8();
                    }
                    m_finalBuilder.replace(pos, prefixLen + typeLen, block);
                }
                    break;
                case ShaderItemType::Uniform:
                {
                    QByteArray block;
                    for (TStrTableStrMap::const_iterator iter = m_registeredVars.samplers.cbegin(),
                         end = m_registeredVars.samplers.cend(); iter != end; ++iter)
                    {
                        const QSSGShaderResourceMergeContext::Sampler &var(mergeContext->m_samplers.value(iter.key()));
                        block += QString::asprintf("layout(binding = %d) uniform %s %s;\n", var.binding, var.type.constData(), var.name.constData()).toUtf8();
                    }

                    if (!mergeContext->m_uniformMembers.isEmpty()) {
                        // The layout (offsets of the members) of the main
                        // uniform block cannot be different in the stages.
                        // (f.ex., a given member must be assumed to be at same
                        // offset both in the vertex and the fragment shader)
                        // Therefore we output everything in all stages.
                        block += QByteArrayLiteral("layout(std140, binding = 0) uniform cbMain {\n");
                        for (auto iter = mergeContext->m_uniformMembers.cbegin(), end = mergeContext->m_uniformMembers.cend();
                             iter != end; ++iter)
                        {
                            const QSSGRenderShaderMetadata::Uniform::Condition condType = iter.value().conditionType;
                            if (condType == QSSGRenderShaderMetadata::Uniform::Regular)
                                block += QString::asprintf("#ifdef %s\n", iter.value().conditionName.constData()).toUtf8();
                            else if (condType == QSSGRenderShaderMetadata::Uniform::Negated)
                                block += QString::asprintf("#ifndef %s\n", iter.value().conditionName.constData()).toUtf8();
                            block += QString::asprintf("  %s %s;\n", iter.value().type.constData(), iter.value().name.constData()).toUtf8();
                            if (condType != QSSGRenderShaderMetadata::Uniform::None)
                                block += QByteArrayLiteral("#endif\n");
                        }
                        block += QByteArrayLiteral("};\n");
                    }
                    m_finalBuilder.replace(pos, prefixLen + typeLen, block);
                }
                    break;
                default:
                    Q_UNREACHABLE();
                    return m_finalBuilder;
                }
            } else {
                break;
            }
        }

        return m_finalBuilder;
    }

    void addFunction(const QByteArray &functionName) override
    {
        if (!m_addedFunctions.contains(functionName)) {
            m_addedFunctions.push_back(functionName);
            QByteArray includeName;
            includeName = "func" + functionName + ".glsllib";
            addInclude(includeName);
        }
    }
};

struct QSSGVertexShaderGenerator : public QSSGStageGeneratorBase
{
    QSSGVertexShaderGenerator(bool rhiCompatible)
        : QSSGStageGeneratorBase(QSSGShaderGeneratorStage::Vertex, rhiCompatible)
    {}
};

struct QSSGTessControlShaderGenerator : public QSSGStageGeneratorBase
{
    QSSGTessControlShaderGenerator(bool rhiCompatible)
        : QSSGStageGeneratorBase(QSSGShaderGeneratorStage::TessControl, rhiCompatible)
    {}

    void addShaderIncomingMap() override
    {
        addShaderItemMap(ShaderItemType::VertexInput, m_incoming, "[]");
    }

    void addShaderOutgoingMap() override
    {
        if (m_outgoing)
            addShaderItemMap(ShaderItemType::Output, *m_outgoing, "[]");
    }

    void updateShaderCacheFlags(QSSGShaderCacheProgramFlags &inFlags) override
    {
        inFlags |= ShaderCacheProgramFlagValues::TessellationEnabled;
    }
};

struct QSSGTessEvalShaderGenerator : public QSSGStageGeneratorBase
{
    QSSGTessEvalShaderGenerator(bool rhiCompatible)
        : QSSGStageGeneratorBase(QSSGShaderGeneratorStage::TessEval, rhiCompatible)
    {}

    void addShaderIncomingMap() override
    {
        addShaderItemMap(ShaderItemType::VertexInput, m_incoming, "[]");
    }

    void updateShaderCacheFlags(QSSGShaderCacheProgramFlags &inFlags) override
    {
        inFlags |= ShaderCacheProgramFlagValues::TessellationEnabled;
    }
};

struct QSSGGeometryShaderGenerator : public QSSGStageGeneratorBase
{
    QSSGGeometryShaderGenerator(bool rhiCompatible)
        : QSSGStageGeneratorBase(QSSGShaderGeneratorStage::Geometry, rhiCompatible)
    {}

    void addShaderIncomingMap() override
    {
        addShaderItemMap(ShaderItemType::VertexInput, m_incoming, "[]");
    }

    void addShaderOutgoingMap() override
    {
        if (m_outgoing)
            addShaderItemMap(ShaderItemType::Output, *m_outgoing);
    }

    void updateShaderCacheFlags(QSSGShaderCacheProgramFlags &inFlags) override
    {
        inFlags |= ShaderCacheProgramFlagValues::GeometryShaderEnabled;
    }
};

struct QSSGFragmentShaderGenerator : public QSSGStageGeneratorBase
{
    QSSGFragmentShaderGenerator(bool rhiCompatible)
        : QSSGStageGeneratorBase(QSSGShaderGeneratorStage::Fragment, rhiCompatible)
    {}

    void addShaderIncomingMap() override
    {
        addShaderItemMap(ShaderItemType::Input, m_incoming);
    }

    void addShaderOutgoingMap() override {}
};

struct QSSGShaderGeneratedProgramOutput
{
    // never null; so safe to call strlen on.
    const char *m_vertexShader{ "" };
    const char *m_tessControlShader{ "" };
    const char *m_tessEvalShader{ "" };
    const char *m_geometryShader{ "" };
    const char *m_fragmentShader{ "" };

    QSSGShaderGeneratedProgramOutput() = default;
    QSSGShaderGeneratedProgramOutput(const char *vs, const char *tc, const char *te, const char *gs, const char *fs)
        : m_vertexShader(vs), m_tessControlShader(tc), m_tessEvalShader(te), m_geometryShader(gs), m_fragmentShader(fs)
    {
    }
};

struct QSSGProgramGenerator : public QSSGShaderProgramGeneratorInterface
{
    QSSGRenderContextInterface *m_context;
    bool m_rhiCompatible;
    QSSGVertexShaderGenerator m_vs;
    QSSGTessControlShaderGenerator m_tc;
    QSSGTessEvalShaderGenerator m_te;
    QSSGGeometryShaderGenerator m_gs;
    QSSGFragmentShaderGenerator m_fs;

    QSSGShaderGeneratorStageFlags m_enabledStages;

    QSSGProgramGenerator(QSSGRenderContextInterface *inContext)
        : m_context(inContext),
          m_rhiCompatible(m_context->renderContext()->rhiContext()->isValid()),
          m_vs(m_rhiCompatible),
          m_tc(m_rhiCompatible),
          m_te(m_rhiCompatible),
          m_gs(m_rhiCompatible),
          m_fs(m_rhiCompatible)
    {
    }

    void linkStages()
    {
        // Link stages incoming to outgoing variables.
        QSSGStageGeneratorBase *previous = nullptr;
        quint32 theStageId = 1;
        for (quint32 idx = 0, end = quint32(QSSGShaderGeneratorStage::StageCount); idx < end; ++idx, theStageId = theStageId << 1) {
            QSSGStageGeneratorBase *thisStage = nullptr;
            QSSGShaderGeneratorStage theStageEnum = static_cast<QSSGShaderGeneratorStage>(theStageId);
            if ((m_enabledStages & theStageEnum)) {
                thisStage = &internalGetStage(theStageEnum);
                if (previous)
                    previous->m_outgoing = &thisStage->m_incoming;
                previous = thisStage;
            }
        }
    }

    void beginProgram(QSSGShaderGeneratorStageFlags inEnabledStages) override
    {
        m_vs.begin(inEnabledStages);
        m_tc.begin(inEnabledStages);
        m_te.begin(inEnabledStages);
        m_gs.begin(inEnabledStages);
        m_fs.begin(inEnabledStages);
        m_enabledStages = inEnabledStages;
        linkStages();
    }

    QSSGShaderGeneratorStageFlags getEnabledStages() const override { return m_enabledStages; }

    QSSGStageGeneratorBase &internalGetStage(QSSGShaderGeneratorStage inStage)
    {
        switch (inStage) {
        case QSSGShaderGeneratorStage::Vertex:
            return m_vs;
        case QSSGShaderGeneratorStage::TessControl:
            return m_tc;
        case QSSGShaderGeneratorStage::TessEval:
            return m_te;
        case QSSGShaderGeneratorStage::Geometry:
            return m_gs;
        case QSSGShaderGeneratorStage::Fragment:
            return m_fs;
        default:
            Q_ASSERT(false);
            break;
        }
        return m_vs;
    }
    // get the stage or nullptr if it has not been created.
    QSSGShaderStageGeneratorInterface *getStage(QSSGShaderGeneratorStage inStage) override
    {
        if ((m_enabledStages & inStage))
            return &internalGetStage(inStage);
        return nullptr;
    }

    QSSGRef<QSSGRenderShaderProgram> compileGeneratedShader(const QByteArray &inShaderName,
                                                                const QSSGShaderCacheProgramFlags &inFlags,
                                                                const ShaderFeatureSetList &inFeatureSet,
                                                                bool separableProgram) override
    {
        // No stages enabled
        if (((quint32)m_enabledStages) == 0) {
            Q_ASSERT(false);
            return nullptr;
        }

        QSSGShaderResourceMergeContext mergeContext;

        QSSGRef<QSSGDynamicObjectSystem> theDynamicSystem(m_context->dynamicObjectSystem());
        QSSGShaderCacheProgramFlags theCacheFlags(inFlags);
        for (quint32 stageIdx = 0; stageIdx < static_cast<quint32>(QSSGShaderGeneratorStage::StageCount); ++stageIdx) {
            QSSGShaderGeneratorStage stageName = static_cast<QSSGShaderGeneratorStage>(1 << stageIdx);
            if (m_enabledStages & stageName) {
                QSSGStageGeneratorBase &theStage(internalGetStage(stageName));
                theStage.buildShaderSourcePass1(&mergeContext);
                theStage.updateShaderCacheFlags(theCacheFlags);
            }
        }

        for (quint32 stageIdx = 0; stageIdx < static_cast<quint32>(QSSGShaderGeneratorStage::StageCount); ++stageIdx) {
            QSSGShaderGeneratorStage stageName = static_cast<QSSGShaderGeneratorStage>(1 << stageIdx);
            if (m_enabledStages & stageName) {
                QSSGStageGeneratorBase &theStage(internalGetStage(stageName));
                theDynamicSystem->resolveIncludeFiles(theStage.m_finalBuilder, inShaderName, &mergeContext);
                theStage.buildShaderSourcePass2(&mergeContext);
            }
        }

        const QSSGRef<QSSGShaderCache> &theCache = m_context->shaderCache();
        return theCache->compileProgram(inShaderName,
                                        m_vs.m_finalBuilder,
                                        m_fs.m_finalBuilder,
                                        m_tc.m_finalBuilder,
                                        m_te.m_finalBuilder,
                                        m_gs.m_finalBuilder,
                                        theCacheFlags,
                                        inFeatureSet,
                                        separableProgram);
    }

    QSSGRef<QSSGRhiShaderStages> compileGeneratedRhiShader(const QByteArray &inShaderName,
                                                           const QSSGShaderCacheProgramFlags &inFlags,
                                                           const ShaderFeatureSetList &inFeatureSet) override
    {
        // No stages enabled
        if (((quint32)m_enabledStages) == 0) {
            Q_ASSERT(false);
            return nullptr;
        }

        QSSGShaderResourceMergeContext mergeContext;

        QSSGRef<QSSGDynamicObjectSystem> theDynamicSystem(m_context->dynamicObjectSystem());
        QSSGShaderCacheProgramFlags theCacheFlags(inFlags);
        for (quint32 stageIdx = 0; stageIdx < static_cast<quint32>(QSSGShaderGeneratorStage::StageCount); ++stageIdx) {
            QSSGShaderGeneratorStage stageName = static_cast<QSSGShaderGeneratorStage>(1 << stageIdx);
            if (m_enabledStages & stageName) {
                QSSGStageGeneratorBase &theStage(internalGetStage(stageName));
                theStage.buildShaderSourcePass1(&mergeContext);
                theStage.updateShaderCacheFlags(theCacheFlags);
            }
        }

        for (quint32 stageIdx = 0; stageIdx < static_cast<quint32>(QSSGShaderGeneratorStage::StageCount); ++stageIdx) {
            QSSGShaderGeneratorStage stageName = static_cast<QSSGShaderGeneratorStage>(1 << stageIdx);
            if (m_enabledStages & stageName) {
                QSSGStageGeneratorBase &theStage(internalGetStage(stageName));
                theDynamicSystem->resolveIncludeFiles(theStage.m_finalBuilder, inShaderName, &mergeContext);
                theStage.buildShaderSourcePass2(&mergeContext);
            }
        }

        const QSSGRef<QSSGShaderCache> &theCache = m_context->shaderCache();
        return theCache->compileForRhi(inShaderName,
                                       m_vs.m_finalBuilder,
                                       m_fs.m_finalBuilder,
                                       theCacheFlags,
                                       inFeatureSet);
    }
};
};

QSSGRef<QSSGRenderShaderProgram> QSSGShaderProgramGeneratorInterface::compileGeneratedShader(const QByteArray &inShaderName,
                                                                                                   bool separableProgram)
{
    return compileGeneratedShader(inShaderName, QSSGShaderCacheProgramFlags(), ShaderFeatureSetList(), separableProgram);
}

QSSGRef<QSSGShaderProgramGeneratorInterface> QSSGShaderProgramGeneratorInterface::createProgramGenerator(QSSGRenderContextInterface *inContext)
{
    return QSSGRef<QSSGShaderProgramGeneratorInterface>(new QSSGProgramGenerator(inContext));
}

void QSSGShaderProgramGeneratorInterface::outputParaboloidDepthVertex(QSSGShaderStageGeneratorInterface &vertexShader)
{
    vertexShader.addIncoming("attr_pos", "vec3");
    vertexShader.addInclude("shadowMapping.glsllib");
    vertexShader.addUniform("modelViewProjection", "mat4");
    // vertexShader.AddUniform("model_view", "mat4");
    vertexShader.addUniform("cameraProperties", "vec2");
    // vertexShader.AddOutgoing("view_pos", "vec4");
    vertexShader.addOutgoing("world_pos", "vec4");

    // Project the location onto screen space.
    // This will be horrible if you have a single large polygon.  Tessellation is your friend here!
    vertexShader.append("void main() {\n"
                        "   ParaboloidMapResult data = VertexParaboloidDepth( attr_pos, modelViewProjection );\n"
                        "   gl_Position = data.m_Position;\n"
                        "   world_pos = data.m_WorldPos;\n"
                        "}\n");
}

void QSSGShaderProgramGeneratorInterface::outputParaboloidDepthTessEval(QSSGShaderStageGeneratorInterface &tessEvalShader)
{
    tessEvalShader.addInclude("shadowMapping.glsllib");
    tessEvalShader.addUniform("modelViewProjection", "mat4");
    tessEvalShader.addOutgoing("world_pos", "vec4");
    tessEvalShader.append(
                "   ParaboloidMapResult data = VertexParaboloidDepth( vec3(pos.xyz), modelViewProjection );\n"
                "   gl_Position = data.m_Position;\n"
                "   world_pos = data.m_WorldPos;\n");
}

void QSSGShaderProgramGeneratorInterface::outputParaboloidDepthFragment(QSSGShaderStageGeneratorInterface &fragmentShader)
{
    fragmentShader.addInclude("shadowMappingFragment.glsllib");
    fragmentShader.addUniform("modelViewProjection", "mat4");
    fragmentShader.addUniform("cameraProperties", "vec2");
    fragmentShader.append(
                "void main() {\n"
                "   gl_FragDepth = FragmentParaboloidDepth( world_pos, modelViewProjection, cameraProperties );\n"
                "}"
                );
}

void QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthVertex(QSSGShaderStageGeneratorInterface &vertexShader)
{
    vertexShader.addIncoming("attr_pos", "vec3");
    vertexShader.addUniform("modelMatrix", "mat4");
    vertexShader.addUniform("modelViewProjection", "mat4");

    vertexShader.addOutgoing("raw_pos", "vec4");
    vertexShader.addOutgoing("world_pos", "vec4");

    vertexShader.append("void main() {\n"
                        "   world_pos = modelMatrix * vec4( attr_pos, 1.0 );\n"
                        "   world_pos /= world_pos.w;\n"
                        "   gl_Position = modelViewProjection * vec4( attr_pos, 1.0 );\n"
                        "   raw_pos = vec4( attr_pos, 1.0 );\n"
                        //	vertexShader->Append("   gl_Position = vec4( attr_pos, 1.0 );\n"
                        "}");
}

void QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthGeometry(QSSGShaderStageGeneratorInterface &geometryShader)
{
    geometryShader.append("layout(triangles) in;\n"
                          "layout(triangle_strip, max_vertices = 18) out;");
    // geometryShader->AddUniform("shadow_mvp[6]", "mat4");

    geometryShader.addUniform("shadow_mv0", "mat4");
    geometryShader.addUniform("shadow_mv1", "mat4");
    geometryShader.addUniform("shadow_mv2", "mat4");
    geometryShader.addUniform("shadow_mv3", "mat4");
    geometryShader.addUniform("shadow_mv4", "mat4");
    geometryShader.addUniform("shadow_mv5", "mat4");
    geometryShader.addUniform("projection", "mat4");

    geometryShader.addUniform("modelMatrix", "mat4");
    geometryShader.addOutgoing("world_pos", "vec4");

    geometryShader.append("void main() {\n"
                          "   mat4 layerMVP[6];\n"
                          "   layerMVP[0] = projection * shadow_mv0;\n"
                          "   layerMVP[1] = projection * shadow_mv1;\n"
                          "   layerMVP[2] = projection * shadow_mv2;\n"
                          "   layerMVP[3] = projection * shadow_mv3;\n"
                          "   layerMVP[4] = projection * shadow_mv4;\n"
                          "   layerMVP[5] = projection * shadow_mv5;\n"
                          "   for (int i = 0; i < 6; ++i)\n"
                          "   {\n"
                          "      gl_Layer = i;\n"
                          "      for(int j = 0; j < 3; ++j)\n"
                          "      {\n"
                          "         world_pos = modelMatrix * raw_pos[j];\n"
                          "         world_pos /= world_pos.w;\n"
                          "         gl_Position = layerMVP[j] * raw_pos[j];\n"
                          "         world_pos.w = gl_Position.w;\n"
                          "         EmitVertex();\n"
                          "      }\n"
                          "      EndPrimitive();\n"
                          "   }\n"
                          "}");
}

void QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(QSSGShaderStageGeneratorInterface &fragmentShader)
{
    fragmentShader.addUniform("cameraPosition", "vec3");
    fragmentShader.addUniform("cameraProperties", "vec2");

    fragmentShader.append("void main() {\n"
                          "    vec3 camPos = vec3( cameraPosition.x, cameraPosition.y, -cameraPosition.z );\n"
                          "    float dist = length( world_pos.xyz - camPos );\n"
                          "    dist = (dist - cameraProperties.x) / (cameraProperties.y - cameraProperties.x);\n"
                          // "    gl_FragDepth = dist;\n"
                          "    fragOutput = vec4(dist, dist, dist, 1.0);\n"
                          "}");
}

QSSGShaderStageGeneratorInterface::~QSSGShaderStageGeneratorInterface() = default;

QT_END_NAMESPACE
