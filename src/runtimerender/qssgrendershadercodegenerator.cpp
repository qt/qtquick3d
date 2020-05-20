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

#include "qssgrendershadercodegenerator_p.h"

#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgshaderresourcemergecontext_p.h>

#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

template<typename T>
static inline void addStartCond(QByteArray &block, const T &var)
{
    // must use #if not #ifdef, as we test for the value, because featureset flags
    // are written out even when 0, think for example #define QSSG_ENABLE_SSM 0
    if (var.conditionType == QSSGRenderShaderMetadata::Uniform::Regular)
        block += QString::asprintf("#if %s\n", var.conditionName.constData()).toUtf8();
    else if (var.conditionType == QSSGRenderShaderMetadata::Uniform::Negated)
        block += QString::asprintf("#if !%s\n", var.conditionName.constData()).toUtf8();
}

template<typename T>
static inline void addEndCond(QByteArray &block, const T &var)
{
    if (var.conditionType != QSSGRenderShaderMetadata::Uniform::None)
        block += QByteArrayLiteral("#endif\n");
}

struct QSSGVertexShaderGenerator : public QSSGStageGeneratorBase
{
    QSSGVertexShaderGenerator(bool rhiCompatible)
        : QSSGStageGeneratorBase(QSSGShaderGeneratorStage::Vertex, rhiCompatible)
    {}
};

struct QSSGGeometryShaderGenerator : public QSSGStageGeneratorBase
{
    QSSGGeometryShaderGenerator(bool rhiCompatible)
        : QSSGStageGeneratorBase(QSSGShaderGeneratorStage::Geometry, rhiCompatible)
    {}

    void addShaderIncomingMap() override
    {
        addShaderItemMap(ShaderItemType::VertexInput, m_incoming, "[]");
        addShaderPass2Marker(ShaderItemType::VertexInput);
    }

    void addShaderOutgoingMap() override
    {
        if (m_outgoing)
            addShaderItemMap(ShaderItemType::Output, *m_outgoing);

        addShaderPass2Marker(ShaderItemType::Output);
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
        addShaderPass2Marker(ShaderItemType::Input);
    }

    void addShaderOutgoingMap() override
    {
        addShaderPass2Marker(ShaderItemType::Output);
    }
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
    QSSGGeometryShaderGenerator m_gs;
    QSSGFragmentShaderGenerator m_fs;

    QSSGShaderGeneratorStageFlags m_enabledStages;

    QSSGProgramGenerator(QSSGRenderContextInterface *inContext)
        : m_context(inContext),
          m_rhiCompatible(m_context->rhiContext()->isValid()),
          m_vs(m_rhiCompatible),
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
    QSSGStageGeneratorBase *getStage(QSSGShaderGeneratorStage inStage) override
    {
        if ((m_enabledStages & inStage))
            return &internalGetStage(inStage);
        return nullptr;
    }

    void registerShaderMetaDataFromSource(QSSGShaderResourceMergeContext *mergeContext,
                                          const QByteArray &contents,
                                          QSSGShaderGeneratorStage stage)
    {
        QSSGRenderShaderMetadata::ShaderMetaData meta = QSSGRenderShaderMetadata::getShaderMetaData(contents);

        for (const QSSGRenderShaderMetadata::Uniform &u : qAsConst(meta.uniforms)) {
            if (u.type.startsWith(QByteArrayLiteral("sampler")))
                mergeContext->registerSampler(u.type, u.name, u.condition, u.conditionName);
            else
                mergeContext->registerUniformMember(u.type, u.name, u.condition, u.conditionName);
        }

        for (const QSSGRenderShaderMetadata::InputOutput &inputVar : qAsConst(meta.inputs)) {
            if (inputVar.stage == stage)
                mergeContext->registerInput(stage, inputVar.type, inputVar.name);
        }

        for (const QSSGRenderShaderMetadata::InputOutput &outputVar : qAsConst(meta.outputs)) {
            if (outputVar.stage == stage)
                mergeContext->registerOutput(stage, outputVar.type, outputVar.name);
        }
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

        const QSSGRef<QSSGShaderLibraryManger> &shaderLibraryManager(m_context->shaderLibraryManger());
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
                shaderLibraryManager->resolveIncludeFiles(theStage.m_finalBuilder, inShaderName);
                registerShaderMetaDataFromSource(&mergeContext, theStage.m_finalBuilder, stageName);
            }
        }

        for (quint32 stageIdx = 0; stageIdx < static_cast<quint32>(QSSGShaderGeneratorStage::StageCount); ++stageIdx) {
            QSSGShaderGeneratorStage stageName = static_cast<QSSGShaderGeneratorStage>(1 << stageIdx);
            if (m_enabledStages & stageName) {
                QSSGStageGeneratorBase &theStage(internalGetStage(stageName));
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

    QSSGRef<QSSGRhiShaderStages> loadBuiltinRhiShader(const QByteArray &inShaderName) override
    {
        return m_context->shaderCache()->loadBuiltinForRhi(inShaderName);
    }

};

QSSGRef<QSSGShaderProgramGeneratorInterface> QSSGShaderProgramGeneratorInterface::createProgramGenerator(QSSGRenderContextInterface *inContext)
{
    return QSSGRef<QSSGShaderProgramGeneratorInterface>(new QSSGProgramGenerator(inContext));
}

// outputXxxx are legacy GL only

void QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthVertex(QSSGStageGeneratorBase &vertexShader)
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

void QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthGeometry(QSSGStageGeneratorBase &geometryShader)
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

void QSSGShaderProgramGeneratorInterface::outputCubeFaceDepthFragment(QSSGStageGeneratorBase &fragmentShader)
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

QT_END_NAMESPACE

QSSGStageGeneratorBase::QSSGStageGeneratorBase(QSSGShaderGeneratorStage inStage, bool rhiCompatible)

    : m_outgoing(nullptr), m_stage(inStage), m_rhiCompatible(rhiCompatible)
{
}

void QSSGStageGeneratorBase::begin(QSSGShaderGeneratorStageFlags inEnabledStages)
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
    // the shared buffers will be cleared elsewhere.
}

void QSSGStageGeneratorBase::addIncoming(const QByteArray &name, const QByteArray &type)
{
    m_incoming.insert(name, type);
}

const QByteArray QSSGStageGeneratorBase::GetIncomingVariableName() { return "in"; }

void QSSGStageGeneratorBase::addOutgoing(const QByteArray &name, const QByteArray &type)
{
    if (m_outgoing == nullptr) {
        Q_ASSERT(false);
        return;
    }
    m_outgoing->insert(name, type);
}

void QSSGStageGeneratorBase::addUniform(const QByteArray &name, const QByteArray &type)
{
    m_uniforms.insert(name, type);
}

void QSSGStageGeneratorBase::addConstantBuffer(const QByteArray &name, const QByteArray &layout)
{
    m_constantBuffers.insert(name, layout);
}

void QSSGStageGeneratorBase::addConstantBufferParam(const QByteArray &cbName, const QByteArray &paramName, const QByteArray &type)
{
    TParamPair theParamPair(paramName, type);
    TConstantBufferParamPair theBufferParamPair(cbName, theParamPair);
    m_constantBufferParams.push_back(theBufferParamPair);
}

QSSGStageGeneratorBase &QSSGStageGeneratorBase::operator<<(const QByteArray &data)
{
    m_codeBuilder.append(data);
    return *this;
}

void QSSGStageGeneratorBase::append(const QByteArray &data)
{
    m_codeBuilder.append(data);
    m_codeBuilder.append("\n");
}

QSSGShaderGeneratorStage QSSGStageGeneratorBase::stage() const { return m_stage; }

void QSSGStageGeneratorBase::addShaderPass2Marker(QSSGStageGeneratorBase::ShaderItemType itemType)
{
    if (m_rhiCompatible) {
        Q_ASSERT(m_mergeContext);
        m_finalBuilder.append(QByteArrayLiteral("//@@") + QByteArray::number(int(itemType)) + QByteArrayLiteral("\n"));
    }
}

void QSSGStageGeneratorBase::addShaderItemMap(QSSGStageGeneratorBase::ShaderItemType itemType, const TStrTableStrMap &itemMap, const QByteArray &inItemSuffix)
{
    m_finalBuilder.append("\n");

    if (m_rhiCompatible) {
        Q_ASSERT(m_mergeContext);
        for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
            const QByteArray name = iter.key() + inItemSuffix;
            switch (itemType) {
            case ShaderItemType::VertexInput:
                m_mergeContext->registerInput(QSSGShaderGeneratorStage::Vertex, iter.value(), name);
                break;
            case ShaderItemType::Input:
                m_mergeContext->registerInput(m_stage, iter.value(), name);
                break;
            case ShaderItemType::Output:
                m_mergeContext->registerOutput(m_stage, iter.value(), name);
                break;
            case ShaderItemType::Uniform:
                if (iter.value().startsWith(QByteArrayLiteral("sampler")))
                    m_mergeContext->registerSampler(iter.value(), name);
                else
                    m_mergeContext->registerUniformMember(iter.value(), name);
                break;
            default:
                qWarning("Unknown shader item %d", int(itemType));
                Q_UNREACHABLE();
            }
        }
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

void QSSGStageGeneratorBase::addShaderIncomingMap()
{
    addShaderItemMap(ShaderItemType::VertexInput, m_incoming);
    addShaderPass2Marker(ShaderItemType::VertexInput);
}

void QSSGStageGeneratorBase::addShaderUniformMap()
{
    addShaderItemMap(ShaderItemType::Uniform, m_uniforms);
    addShaderPass2Marker(ShaderItemType::Uniform);
}

void QSSGStageGeneratorBase::addShaderOutgoingMap()
{
    if (m_outgoing)
        addShaderItemMap(ShaderItemType::Output, *m_outgoing);

    addShaderPass2Marker(ShaderItemType::Output);
}

void QSSGStageGeneratorBase::addShaderConstantBufferItemMap(const QByteArray &itemType, const TStrTableStrMap &cbMap, TConstantBufferParamArray cbParamsArray)
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

void QSSGStageGeneratorBase::appendShaderCode() { m_finalBuilder.append(m_codeBuilder); }

void QSSGStageGeneratorBase::updateShaderCacheFlags(QSSGShaderCacheProgramFlags &) {}

void QSSGStageGeneratorBase::addInclude(const QByteArray &name) { m_includes.insert(name); }

void QSSGStageGeneratorBase::buildShaderSourcePass1(QSSGShaderResourceMergeContext *mergeContext)
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

QByteArray QSSGStageGeneratorBase::buildShaderSourcePass2(QSSGShaderResourceMergeContext *mergeContext)
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
                if (m_stage == QSSGShaderGeneratorStage::Vertex) {
                    QByteArray block;
                    for (const QSSGShaderResourceMergeContext::InOutVar &var : mergeContext->m_inOutVars) {
                        if (var.stagesInputIn.testFlag(m_stage))
                            block += QString::asprintf("layout(location = %d) in %s %s;\n", var.location, var.type.constData(), var.name.constData()).toUtf8();
                    }
                    m_finalBuilder.replace(pos, prefixLen + typeLen, block);
                }
                break;
            case ShaderItemType::Input:
            {
                QByteArray block;
                for (const QSSGShaderResourceMergeContext::InOutVar &var : mergeContext->m_inOutVars) {
                    if (var.stagesInputIn.testFlag(m_stage))
                        block += QString::asprintf("layout(location = %d) in %s %s;\n", var.location, var.type.constData(), var.name.constData()).toUtf8();
                }
                m_finalBuilder.replace(pos, prefixLen + typeLen, block);
            }
                break;
            case ShaderItemType::Output:
            {
                QByteArray block;
                for (const QSSGShaderResourceMergeContext::InOutVar &var : mergeContext->m_inOutVars) {
                    if (var.stageOutputFrom.testFlag(m_stage))
                        block += QString::asprintf("layout(location = %d) out %s %s;\n", var.location, var.type.constData(), var.name.constData()).toUtf8();
                }
                m_finalBuilder.replace(pos, prefixLen + typeLen, block);
            }
                break;
            case ShaderItemType::Uniform:
            {
                QByteArray block;

                for (const auto &sampler : qAsConst(mergeContext->m_samplers)) {
                    addStartCond(block, sampler);
                    block += QString::asprintf("layout(binding = %d) uniform %s %s;\n",
                                               sampler.binding,
                                               sampler.type.constData(),
                                               sampler.name.constData()).toUtf8();
                    addEndCond(block, sampler);
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
                        addStartCond(block, iter.value());
                        block += QString::asprintf("  %s %s;\n", iter.value().type.constData(), iter.value().name.constData()).toUtf8();
                        addEndCond(block, iter.value());
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

void QSSGStageGeneratorBase::addFunction(const QByteArray &functionName)
{
    if (!m_addedFunctions.contains(functionName)) {
        m_addedFunctions.push_back(functionName);
        QByteArray includeName;
        includeName = "func" + functionName + ".glsllib";
        addInclude(includeName);
    }
}
