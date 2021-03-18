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

QT_BEGIN_NAMESPACE

QSSGShaderCodeGeneratorBase::QSSGShaderCodeGeneratorBase(const QSSGRenderContextType &ctxType)
    : m_renderContextType(ctxType)
{
}

QSSGShaderCodeGeneratorBase::~QSSGShaderCodeGeneratorBase() = default;
void QSSGShaderCodeGeneratorBase::begin()
{
    m_uniforms.clear();
    getVaryings().clear();
    m_attributes.clear();
    m_includes.clear();
    m_codes.clear();
    m_finalShaderBuilder.clear();
    m_codeBuilder.clear();
    m_constantBuffers.clear();
    m_constantBufferParams.clear();
}
void QSSGShaderCodeGeneratorBase::append(const QByteArray &data)
{
    m_codeBuilder.append(data);
    m_codeBuilder.append("\n");
}

void QSSGShaderCodeGeneratorBase::addUniform(const QByteArray &name, const QByteArray &type)
{
    m_uniforms.insert(name, type);
}
void QSSGShaderCodeGeneratorBase::addConstantBuffer(const QByteArray &name, const QByteArray &layout)
{
    m_constantBuffers.insert(name, layout);
}
void QSSGShaderCodeGeneratorBase::addConstantBufferParam(const QByteArray &cbName, const QByteArray &paramName, const QByteArray &type)
{
    TParamPair theParamPair(paramName, type);
    TConstantBufferParamPair theBufferParamPair(cbName, theParamPair);
    m_constantBufferParams.push_back(theBufferParamPair);
}

void QSSGShaderCodeGeneratorBase::addAttribute(const QByteArray &name, const QByteArray &type)
{
    m_attributes.insert(name, type);
}

void QSSGShaderCodeGeneratorBase::addVarying(const QByteArray &name, const QByteArray &type)
{
    getVaryings().insert(name, type);
}

void QSSGShaderCodeGeneratorBase::addLocalVariable(const QByteArray &name, const QByteArray &type, int tabCount)
{
    for (; tabCount >= 0; --tabCount)
        m_codeBuilder.append("    ");
    m_codeBuilder.append(type);
    m_codeBuilder.append(" ");
    m_codeBuilder.append(name);
    m_codeBuilder.append(";\n");
}

void QSSGShaderCodeGeneratorBase::addInclude(const QByteArray &name)
{
    m_includes.insert(name);
}

bool QSSGShaderCodeGeneratorBase::hasCode(Enum value)
{
    return m_codes.contains(value);
}
void QSSGShaderCodeGeneratorBase::setCode(Enum value)
{
    m_codes.insert(quint32(value));
}

void QSSGShaderCodeGeneratorBase::setupWorldPosition()
{
    if (!hasCode(WorldPosition)) {
        setCode(WorldPosition);
        addUniform("modelMatrix", "mat4");
        append("    vec3 varWorldPos = (modelMatrix * vec4(attr_pos, 1.0)).xyz;");
    }
}

void QSSGShaderCodeGeneratorBase::generateViewVector()
{
    if (!hasCode(ViewVector)) {
        setCode(ViewVector);
        setupWorldPosition();
        addInclude("viewProperties.glsllib");
        append("    vec3 view_vector = normalize(cameraPosition - varWorldPos);");
    }
}

void QSSGShaderCodeGeneratorBase::generateWorldNormal()
{
    if (!hasCode(WorldNormal)) {
        setCode(WorldNormal);
        addAttribute("attr_norm", "vec3");
        addUniform("normalMatrix", "mat3");
        append("    vec3 world_normal = normalize(normalMatrix * objectNormal).xyz;");
    }
}

void QSSGShaderCodeGeneratorBase::generateEnvMapReflection(QSSGShaderCodeGeneratorBase &inFragmentShader)
{
    if (!hasCode(EnvMapReflection)) {
        setCode(EnvMapReflection);
        setupWorldPosition();
        generateWorldNormal();
        addInclude("viewProperties.glsllib");
        addVarying("var_object_to_camera", "vec3");
        append("    var_object_to_camera = normalize( varWorldPos - cameraPosition );");
        // World normal cannot be relied upon in the vertex shader because of bump maps.
        inFragmentShader.append("    vec3 environment_map_reflection = reflect("
                                    "vec3(var_object_to_camera.x, var_object_to_camera.y, "
                                    "var_object_to_camera.z), world_normal.xyz );\n"
                                "    environment_map_reflection *= vec3( 0.5, 0.5, 0 );\n"
                                "    environment_map_reflection += vec3( 0.5, 0.5, 1.0 );");
    }
}

void QSSGShaderCodeGeneratorBase::generateUVCoords()
{
    if (!hasCode(UVCoords)) {
        setCode(UVCoords);
        addAttribute("attr_uv0", "vec2");
        append("    vec2 uv_coords = attr_uv0;");
    }
}

void QSSGShaderCodeGeneratorBase::generateTextureSwizzle(QSSGRenderTextureSwizzleMode swizzleMode,
                                                           QByteArray &texSwizzle,
                                                           QByteArray &lookupSwizzle)
{
    QSSGRenderContextTypes deprecatedContextFlags(QSSGRenderContextType::GL2 | QSSGRenderContextType::GLES2);

    if (!(deprecatedContextFlags & m_renderContextType)) {
        switch (swizzleMode) {
        case QSSGRenderTextureSwizzleMode::L8toR8:
        case QSSGRenderTextureSwizzleMode::L16toR16:
            texSwizzle.append(".rgb");
            lookupSwizzle.append(".rrr");
            break;
        case QSSGRenderTextureSwizzleMode::L8A8toRG8:
            texSwizzle.append(".rgba");
            lookupSwizzle.append(".rrrg");
            break;
        case QSSGRenderTextureSwizzleMode::A8toR8:
            texSwizzle.append(".a");
            lookupSwizzle.append(".r");
            break;
        default:
            break;
        }
    }
}

void QSSGShaderCodeGeneratorBase::generateShadedWireframeBase()
{
    // how this all work see
    // http://developer.download.nvidia.com/SDK/10.5/direct3d/Source/SolidWireframe/Doc/SolidWireframe.pdf
    append("// project points to screen space\n"
           "    vec3 p0 = vec3(viewportMatrix * (gl_in[0].gl_Position / gl_in[0].gl_Position.w));\n"
           "    vec3 p1 = vec3(viewportMatrix * (gl_in[1].gl_Position / gl_in[1].gl_Position.w));\n"
           "    vec3 p2 = vec3(viewportMatrix * (gl_in[2].gl_Position / gl_in[2].gl_Position.w));\n"
           "// compute triangle heights\n"
           "    float e1 = length(p1 - p2);\n"
           "    float e2 = length(p2 - p0);\n"
           "    float e3 = length(p1 - p0);\n"
           "    float alpha = acos( (e2*e2 + e3*e3 - e1*e1) / (2.0*e2*e3) );\n"
           "    float beta = acos( (e1*e1 + e3*e3 - e2*e2) / (2.0*e1*e3) );\n"
           "    float ha = abs( e3 * sin( beta ) );\n"
           "    float hb = abs( e3 * sin( alpha ) );\n"
           "    float hc = abs( e2 * sin( alpha ) );\n");
}

void QSSGShaderCodeGeneratorBase::addShaderItemMap(const QByteArray &itemType, const TStrTableStrMap &itemMap)
{
    m_finalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
        m_finalShaderBuilder.append(itemType);
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.value());
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.key());
        m_finalShaderBuilder.append(";\n");
    }
}

void QSSGShaderCodeGeneratorBase::addShaderConstantBufferItemMap(const QByteArray &itemType,
                                                                   const TStrTableStrMap &cbMap,
                                                                   TConstantBufferParamArray cbParamsArray)
{
    m_finalShaderBuilder.append("\n");

    // iterate over all constant buffers
    for (TStrTableStrMap::const_iterator iter = cbMap.begin(), end = cbMap.end(); iter != end; ++iter) {
        m_finalShaderBuilder.append(iter.value());
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(itemType);
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.key());
        m_finalShaderBuilder.append(" {\n");
        // iterate over all param entries and add match
        for (TConstantBufferParamArray::const_iterator iter1 = cbParamsArray.begin(), end = cbParamsArray.end(); iter1 != end; ++iter1) {
            if (iter1->first == iter.key()) {
                m_finalShaderBuilder.append(iter1->second.second);
                m_finalShaderBuilder.append(" ");
                m_finalShaderBuilder.append(iter1->second.first);
                m_finalShaderBuilder.append(";\n");
            }
        }

        m_finalShaderBuilder.append("};\n");
    }
}

QByteArray QSSGShaderCodeGeneratorBase::buildShaderSource()
{
    for (auto iter = m_includes.constBegin(), end = m_includes.constEnd(); iter != end; ++iter) {
        m_finalShaderBuilder.append("#include \"");
        m_finalShaderBuilder.append(*iter);
        m_finalShaderBuilder.append("\"\n");
    }
    addShaderItemMap("attribute", m_attributes);
    addShaderItemMap("uniform", m_uniforms);
    addShaderConstantBufferItemMap("uniform", m_constantBuffers, m_constantBufferParams);
    addShaderItemMap("varying", getVaryings());
    m_finalShaderBuilder.append("\n");
    m_finalShaderBuilder.append(m_codeBuilder);
    return m_finalShaderBuilder;
}

QSSGShaderCodeGeneratorBase &QSSGShaderCodeGeneratorBase::operator<<(const QByteArray &data)
{
    m_codeBuilder.append(data);
    return *this;
}

QSSGShaderVertexCodeGenerator::QSSGShaderVertexCodeGenerator(const QSSGRenderContextType &ctxType)
    : QSSGShaderCodeGeneratorBase(ctxType)
{
}
TStrTableStrMap &QSSGShaderVertexCodeGenerator::getVaryings()
{
    return m_varyings;
}

QSSGShaderTessControlCodeGenerator::QSSGShaderTessControlCodeGenerator(QSSGShaderVertexCodeGenerator &vert,
                                                                           const QSSGRenderContextType &ctxType)
    : QSSGShaderCodeGeneratorBase(ctxType), m_vertGenerator(vert)
{
}

// overwritten from base
void QSSGShaderTessControlCodeGenerator::addShaderItemMap(const QByteArray &itemType, const TStrTableStrMap &itemMap)
{
    QByteArray extVtx("");
    QByteArray extTC("");
    QByteArray type(itemType);
    if (type != QByteArrayLiteral("varying")) {
        extVtx = "[]";
        extTC = "TC[]";
        type = "attribute";
    }

    m_finalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
        m_finalShaderBuilder.append(type);
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.value());
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.key());
        m_finalShaderBuilder.append(extVtx);
        m_finalShaderBuilder.append(";\n");
    }

    // if this is varyings write output of tess control shader
    if (!extVtx.isEmpty()) {
        m_finalShaderBuilder.append("\n");
        type = "varying";

        for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
            m_finalShaderBuilder.append(type);
            m_finalShaderBuilder.append(" ");
            m_finalShaderBuilder.append(iter.value());
            m_finalShaderBuilder.append(" ");
            m_finalShaderBuilder.append(iter.key());
            m_finalShaderBuilder.append(extTC);
            m_finalShaderBuilder.append(";\n");
        }
    }
}
TStrTableStrMap &QSSGShaderTessControlCodeGenerator::getVaryings()
{
    return m_vertGenerator.m_varyings;
}

QSSGShaderTessEvalCodeGenerator::QSSGShaderTessEvalCodeGenerator(QSSGShaderTessControlCodeGenerator &tc,
                                                                     const QSSGRenderContextType &ctxType)
    : QSSGShaderCodeGeneratorBase(ctxType), m_tessControlGenerator(tc), m_hasGeometryStage(false)
{
}
// overwritten from base
void QSSGShaderTessEvalCodeGenerator::addShaderItemMap(const QByteArray &itemType, const TStrTableStrMap &itemMap)
{
    QByteArray extTC("");
    QByteArray extTE("");
    QByteArray type(itemType);
    if (type != QByteArrayLiteral("varying")) {
        extTC = "TC[]";
        type = "attribute";
    }
    if (m_hasGeometryStage) {
        extTE = "TE";
    }

    m_finalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
        m_finalShaderBuilder.append(type);
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.value());
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.key());
        m_finalShaderBuilder.append(extTC);
        m_finalShaderBuilder.append(";\n");
    }

    // if this are varyings write output of tess eval shader
    if (!extTC.isEmpty()) {
        m_finalShaderBuilder.append("\n");
        type = "varying";

        for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
            m_finalShaderBuilder.append(type);
            m_finalShaderBuilder.append(" ");
            m_finalShaderBuilder.append(iter.value());
            m_finalShaderBuilder.append(" ");
            m_finalShaderBuilder.append(iter.key());
            m_finalShaderBuilder.append(extTE);
            m_finalShaderBuilder.append(";\n");
        }
    }
}
TStrTableStrMap &QSSGShaderTessEvalCodeGenerator::getVaryings()
{
    return m_tessControlGenerator.m_vertGenerator.getVaryings();
}
void QSSGShaderTessEvalCodeGenerator::setGeometryStage(bool hasGeometryStage)
{
    m_hasGeometryStage = hasGeometryStage;
}

QSSGShaderGeometryCodeGenerator::QSSGShaderGeometryCodeGenerator(QSSGShaderVertexCodeGenerator &vert,
                                                                     const QSSGRenderContextType &ctxType)
    : QSSGShaderCodeGeneratorBase(ctxType), m_vertGenerator(vert)
{
}

// overwritten from base
void QSSGShaderGeometryCodeGenerator::addShaderItemMap(const QByteArray &itemType, const TStrTableStrMap &itemMap)
{
    QByteArray inExt("");
    QByteArray type(itemType);
    if (type != QByteArrayLiteral("varying")) {
        type = "attribute";
        if (m_hasTessellationStage)
            inExt = "TE[]";
        else
            inExt = "[]";
    }

    m_finalShaderBuilder.append("\n");

    for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
        m_finalShaderBuilder.append(type);
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.value());
        m_finalShaderBuilder.append(" ");
        m_finalShaderBuilder.append(iter.key());
        m_finalShaderBuilder.append(inExt);
        m_finalShaderBuilder.append(";\n");
    }

    // if this are varyings write output of geometry shader
    if (itemType != QByteArrayLiteral("varying")) {
        m_finalShaderBuilder.append("\n");
        type = "varying";

        for (TStrTableStrMap::const_iterator iter = itemMap.begin(), end = itemMap.end(); iter != end; ++iter) {
            m_finalShaderBuilder.append(type);
            m_finalShaderBuilder.append(" ");
            m_finalShaderBuilder.append(iter.value());
            m_finalShaderBuilder.append(" ");
            m_finalShaderBuilder.append(iter.key());
            m_finalShaderBuilder.append(";\n");
        }
    }
}
TStrTableStrMap &QSSGShaderGeometryCodeGenerator::getVaryings()
{
    return m_vertGenerator.m_varyings;
}
void QSSGShaderGeometryCodeGenerator::setTessellationStage(bool hasTessellationStage)
{
    m_hasTessellationStage = hasTessellationStage;
}

QSSGShaderFragmentCodeGenerator::QSSGShaderFragmentCodeGenerator(QSSGShaderVertexCodeGenerator &vert,
                                                                     const QSSGRenderContextType &ctxType)
    : QSSGShaderCodeGeneratorBase(ctxType), m_vertGenerator(vert)
{
}
TStrTableStrMap &QSSGShaderFragmentCodeGenerator::getVaryings()
{
    return m_vertGenerator.m_varyings;
}

QT_END_NAMESPACE
