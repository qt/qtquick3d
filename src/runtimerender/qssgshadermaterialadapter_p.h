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

#ifndef QSSG_SHADER_MATERIAL_ADAPTER_H
#define QSSG_SHADER_MATERIAL_ADAPTER_H

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

#include <QtQuick3DRuntimeRender/private/qssgrendermaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderlibrarymanager_p.h>

QT_BEGIN_NAMESPACE

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderMaterialAdapter
{
    static QSSGShaderMaterialAdapter *create(const QSSGRenderGraphObject &materialNode);
    virtual ~QSSGShaderMaterialAdapter();

    virtual bool isPrincipled() = 0;
    virtual bool isMetalnessEnabled() = 0;
    virtual bool isSpecularEnabled() = 0;
    virtual bool isVertexColorsEnabled() = 0;
    virtual bool hasLighting() = 0;
    virtual QSSGRenderDefaultMaterial::MaterialSpecularModel specularModel() = 0;
    virtual QSSGRenderDefaultMaterial::MaterialAlphaMode alphaMode() = 0;

    virtual QSSGRenderImage *iblProbe() = 0;
    virtual QVector3D emissiveColor() = 0;
    virtual QVector4D color() = 0;
    virtual QVector3D specularTint() = 0;
    virtual float ior() = 0;
    virtual float fresnelPower() = 0;
    virtual float metalnessAmount() = 0;
    virtual float specularAmount() = 0;
    virtual float specularRoughness() = 0;
    virtual float bumpAmount() = 0;
    virtual float translucentFallOff() = 0;
    virtual float diffuseLightWrap() = 0;
    virtual float occlusionAmount() = 0;
    virtual float alphaCutOff() = 0;
    virtual float pointSize() = 0;
    virtual float lineWidth() = 0;

    virtual bool isUnshaded();
    virtual bool hasCustomShaderSnippet(QSSGShaderCache::ShaderType type);
    virtual QByteArray customShaderSnippet(QSSGShaderCache::ShaderType type,
                                           const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager);
    virtual bool hasCustomShaderFunction(QSSGShaderCache::ShaderType shaderType,
                                         const QByteArray &funcName,
                                         const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager);
    virtual void setCustomPropertyUniforms(char *ubufData,
                                           QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                           const QSSGRenderContextInterface &context);
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderDefaultMaterialAdapter final : public QSSGShaderMaterialAdapter
{
    QSSGShaderDefaultMaterialAdapter(const QSSGRenderDefaultMaterial &material);

    bool isPrincipled() override;
    bool isMetalnessEnabled() override;
    bool isSpecularEnabled() override;
    bool isVertexColorsEnabled() override;
    bool hasLighting() override;
    QSSGRenderDefaultMaterial::MaterialSpecularModel specularModel() override;
    QSSGRenderDefaultMaterial::MaterialAlphaMode alphaMode() override;

    QSSGRenderImage *iblProbe() override;
    QVector3D emissiveColor() override;
    QVector4D color() override;
    QVector3D specularTint() override;
    float ior() override;
    float fresnelPower() override;
    float metalnessAmount() override;
    float specularAmount() override;
    float specularRoughness() override;
    float bumpAmount() override;
    float translucentFallOff() override;
    float diffuseLightWrap() override;
    float occlusionAmount() override;
    float alphaCutOff() override;
    float pointSize() override;
    float lineWidth() override;

private:
    const QSSGRenderDefaultMaterial &m_material;
};

struct Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderCustomMaterialAdapter final : public QSSGShaderMaterialAdapter
{
    QSSGShaderCustomMaterialAdapter(const QSSGRenderCustomMaterial &material);

    bool isPrincipled() override;
    bool isMetalnessEnabled() override;
    bool isSpecularEnabled() override;
    bool isVertexColorsEnabled() override;
    bool hasLighting() override;
    QSSGRenderDefaultMaterial::MaterialSpecularModel specularModel() override;
    QSSGRenderDefaultMaterial::MaterialAlphaMode alphaMode() override;

    QSSGRenderImage *iblProbe() override;
    QVector3D emissiveColor() override;
    QVector4D color() override;
    QVector3D specularTint() override;
    float ior() override;
    float fresnelPower() override;
    float metalnessAmount() override;
    float specularAmount() override;
    float specularRoughness() override;
    float bumpAmount() override;
    float translucentFallOff() override;
    float diffuseLightWrap() override;
    float occlusionAmount() override;
    float alphaCutOff() override;
    float pointSize() override;
    float lineWidth() override;

    bool isUnshaded() override;
    bool hasCustomShaderSnippet(QSSGShaderCache::ShaderType type) override;
    QByteArray customShaderSnippet(QSSGShaderCache::ShaderType type,
                                   const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager) override;
    bool hasCustomShaderFunction(QSSGShaderCache::ShaderType shaderType,
                                 const QByteArray &funcName,
                                 const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager) override;
    void setCustomPropertyUniforms(char *ubufData,
                                   QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                   const QSSGRenderContextInterface &context) override;

    using StringPair = QPair<QByteArray, QByteArray>;
    using StringPairList = QVarLengthArray<StringPair, 16>;
    using ShaderCodeAndMetaData = QPair<QByteArray, QSSGCustomShaderMetaData>;
    static ShaderCodeAndMetaData prepareCustomShader(QByteArray &dst,
                                                     const QByteArray &shaderCode,
                                                     QSSGShaderCache::ShaderType type,
                                                     const StringPairList &baseUniforms,
                                                     const StringPairList &baseInputs = StringPairList(),
                                                     const StringPairList &baseOutputs = StringPairList());

private:
    const QSSGRenderCustomMaterial &m_material;
};

struct QSSGCustomMaterialVariableSubstitution
{
    QByteArray builtin;
    QByteArray actualName;
};

QT_END_NAMESPACE

#endif
