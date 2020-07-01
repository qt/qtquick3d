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

#ifndef QSSG_RENDER_CUSTOM_MATERIAL_SYSTEM_H
#define QSSG_RENDER_CUSTOM_MATERIAL_SYSTEM_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>
#include <QtQuick3DRuntimeRender/private/qssgvertexpipelineimpl_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableobjects_p.h>

#include <QtCore/qhash.h>

#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h> // Make it possible to forward declare the nested TextureProperty

QT_BEGIN_NAMESPACE

struct QSSGCustomMaterialRenderContext;
struct QSSGRenderCustomMaterial;
class QSSGMaterialSystem;
struct QSSGRenderSubset;
struct QSSGShaderMapKey;
struct QSSGCustomMaterialTextureData;
struct QSSGRenderModel;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGMaterialSystem
{
    Q_DISABLE_COPY(QSSGMaterialSystem)
public:
    QAtomicInt ref;

private:
    typedef QHash<QSSGShaderMapKey, QSSGRef<QSSGRhiShaderStagesWithResources>> RhiShaderMap;
    typedef QPair<QByteArray, QByteArray> TStrStrPair;

    QSSGRenderContextInterface *context = nullptr;
    RhiShaderMap rhiShaderMap;

    QSSGLayerGlobalRenderProperties getLayerGlobalRenderProperties(QSSGCustomMaterialRenderContext &inRenderContext);

    QSSGRef<QSSGRhiShaderStagesWithResources> prepareRhiShader(const QSSGRenderContextInterface &renderContext,
                                                               QSSGCustomMaterialRenderContext &inRenderContext,
                                                               const QSSGRenderCustomMaterial &inMaterial,
                                                               const ShaderFeatureSetList &inFeatureSet);

    void setShaderResources(const QSSGRenderCustomMaterial &inMaterial,
                            const QByteArray &inPropertyName,
                            const QVariant &propertyValue,
                            QSSGRenderShaderDataType inPropertyType,
                            const QSSGRef<QSSGRhiShaderStagesWithResources> &shaderPipeline);

public:
    QSSGMaterialSystem(QSSGRenderContextInterface *ct);

    ~QSSGMaterialSystem();

    void setRenderContextInterface(QSSGRenderContextInterface *inContext);

    // Returns true if the material is dirty and thus will produce a different render result
    // than previously.  This effects things like progressive AA.
    bool prepareForRender(const QSSGRenderModel &inModel,
                          const QSSGRenderSubset &inSubset,
                          QSSGRenderCustomMaterial &inMaterial);

    void prepareRhiSubset(QSSGCustomMaterialRenderContext &customMaterialContext,
                          const ShaderFeatureSetList &featureSet,
                          QSSGRhiGraphicsPipelineState *ps,
                          QSSGCustomMaterialRenderable &renderable);
    void applyRhiShaderPropertyValues(const QSSGRenderCustomMaterial &inMaterial,
                                      const QSSGRef<QSSGRhiShaderStagesWithResources> &shaderPipeline);
    void renderRhiSubset(QSSGRhiContext *rhiCtx,
                         QSSGCustomMaterialRenderable &renderable,
                         QSSGLayerRenderData &inData,
                         bool *needsSetViewport);
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGCustomMaterialVertexPipeline : public QSSGVertexPipelineBase
{
    QSSGRenderContextInterface *m_context;

    QSSGCustomMaterialVertexPipeline(QSSGRenderContextInterface *inContext);

    // Responsible for beginning all vertex and fragment generation (void main() { etc).
    virtual void beginVertexGeneration() override;
    // The fragment shader expects a floating point constant, objectOpacity to be defined
    // post this method.
    virtual void beginFragmentGeneration() override;
    // Output variables may be mangled in some circumstances so the shader generation
    // system needs an abstraction mechanism around this.
    virtual void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValue) override;
    virtual void generateEnvMapReflection(const QSSGShaderDefaultMaterialKey &) override {}
    virtual void generateViewVector() override {}
    virtual void generateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey) override;
    virtual void generateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey) override;
    virtual void generateObjectNormal() override;
    virtual void generateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey) override;
    virtual void generateWorldPosition() override;
    // responsible for closing all vertex and fragment generation
    virtual void endVertexGeneration(bool customShader) override;
    virtual void endFragmentGeneration(bool customShader) override;
    virtual QSSGStageGeneratorBase &activeStage() override;
    virtual void addInterpolationParameter(const QByteArray &inName, const QByteArray &inType) override;
    virtual void doGenerateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey) override;
    virtual void doGenerateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey) override;
    virtual void doGenerateObjectNormal() override;
    virtual void doGenerateWorldPosition() override;
    virtual void doGenerateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey) override;
    virtual void doGenerateVertexColor(const QSSGShaderDefaultMaterialKey &inKey) override;
};
QT_END_NAMESPACE
#endif
