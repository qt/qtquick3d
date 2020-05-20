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

#ifndef QSSG_RENDER_DEFAULT_MATERIAL_SHADER_GENERATOR_H
#define QSSG_RENDER_DEFAULT_MATERIAL_SHADER_GENERATOR_H

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

QT_BEGIN_NAMESPACE

class QSSGRenderShadowMap;
struct QSSGRenderableImage;

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGDefaultMaterialVertexPipelineInterface : public QSSGStageGeneratorBase
{
protected:
    virtual ~QSSGDefaultMaterialVertexPipelineInterface();

public:
    // Responsible for beginning all vertex and fragment generation (void main() { etc).
    virtual void beginVertexGeneration() = 0;
    // The fragment shader expects a floating point constant, objectOpacity to be defined
    // post this method.
    virtual void beginFragmentGeneration() = 0;
    // Output variables may be mangled in some circumstances so the shader generation system
    // needs an abstraction
    // mechanism around this.
    virtual void assignOutput(const QByteArray &inVarName, const QByteArray &inVarValueExpr) = 0;

    /**
     * @brief Generates UV coordinates in shader code
     *
     * @param[in] inUVSet		index of UV data set
     *
     * @return no return
     */
    virtual void generateUVCoords(quint32 inUVSet, const QSSGShaderDefaultMaterialKey &inKey) = 0;

    virtual void generateEnvMapReflection(const QSSGShaderDefaultMaterialKey &inKey) = 0;
    virtual void generateViewVector() = 0;

    // fragment shader expects varying vertex normal
    // lighting in vertex pipeline expects world_normal
    virtual void generateWorldNormal(const QSSGShaderDefaultMaterialKey &inKey) = 0; // world_normal in both vert and frag shader
    virtual void generateObjectNormal() = 0; // object_normal in both vert and frag shader
    virtual void generateWorldPosition() = 0; // model_world_position in both vert and frag shader
    virtual void generateVarTangentAndBinormal(const QSSGShaderDefaultMaterialKey &inKey) = 0;
    virtual void generateVertexColor(const QSSGShaderDefaultMaterialKey &inKey) = 0;

    // responsible for closing all vertex and fragment generation
    virtual void endVertexGeneration(bool customShader) = 0;
    virtual void endFragmentGeneration(bool customShader) = 0;
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGDefaultMaterialShaderGeneratorInterface : public QSSGMaterialShaderGeneratorInterface
{
public:
    QSSGDefaultMaterialShaderGeneratorInterface(QSSGRenderContextInterface *renderContext)
        : QSSGMaterialShaderGeneratorInterface(renderContext)
    {}

    virtual ~QSSGDefaultMaterialShaderGeneratorInterface() override {}

    static QSSGRef<QSSGDefaultMaterialShaderGeneratorInterface> createDefaultMaterialShaderGenerator(QSSGRenderContextInterface *inRenderContext);
};
QT_END_NAMESPACE
#endif
