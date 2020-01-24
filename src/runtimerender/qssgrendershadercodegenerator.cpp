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
