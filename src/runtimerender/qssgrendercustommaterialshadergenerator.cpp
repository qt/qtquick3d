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

#include "qssgrendermaterialshadergenerator_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderdefaultmaterialshadergenerator_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadercodegeneratorv2_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderableimage_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderimage_p.h>
#include <QtQuick3DRender/private/qssgrendercontext_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrenderlight_p.h>
#include <QtQuick3DRender/private/qssgrendershaderprogram_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershadowmap_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercustommaterial_p.h>
#include "qssgrendercustommaterialsystem_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderlightconstantproperties_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendershaderkeys_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendererimplshaders_gl_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

namespace LegacyGl {
// legacy GL only
struct QSSGShaderLightProperties
{
    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> m_shader;
    QSSGRenderLight::Type m_lightType;
    QSSGLightSourceShader m_lightData;

    QSSGShaderLightProperties(const QSSGRef<QSSGRenderShaderProgram> &inShader)
        : m_shader(inShader), m_lightType(QSSGRenderLight::Type::Directional)
    {
    }

    void set(const QSSGRenderLight *inLight)
    {
        QVector3D dir(0, 0, 1);
        if (inLight->m_lightType == QSSGRenderLight::Type::Directional) {
            dir = inLight->getScalingCorrectDirection();
            // we lit in world sapce
            dir *= -1;
            m_lightData.position = QVector4D(dir, 0.0);
        } else if (inLight->m_lightType == QSSGRenderLight::Type::Area
                   || inLight->m_lightType == QSSGRenderLight::Type::Spot) {
            dir = inLight->getScalingCorrectDirection();
            m_lightData.position = QVector4D(inLight->getGlobalPos(), 1.0);
        } else {
            dir = inLight->getGlobalPos();
            m_lightData.position = QVector4D(dir, 1.0);
        }

        m_lightType = inLight->m_lightType;

        m_lightData.direction = QVector4D(dir, 0.0);

        float normalizedBrightness = aux::translateBrightness(inLight->m_brightness);
        m_lightData.diffuse = QVector4D(inLight->m_diffuseColor * normalizedBrightness, 1.0);
        m_lightData.specular = QVector4D(inLight->m_specularColor * normalizedBrightness, 1.0);

        if (inLight->m_lightType == QSSGRenderLight::Type::Area) {
            m_lightData.width = inLight->m_areaWidth;
            m_lightData.height = inLight->m_areaWidth;

            QMatrix3x3 theDirMatrix(mat44::getUpper3x3(inLight->globalTransform));
            m_lightData.right = QVector4D(mat33::transform(theDirMatrix, QVector3D(1, 0, 0)), inLight->m_areaWidth);
            m_lightData.up = QVector4D(mat33::transform(theDirMatrix, QVector3D(0, 1, 0)), inLight->m_areaHeight);
        } else {
            m_lightData.width = 0.0;
            m_lightData.height = 0.0;
            m_lightData.right = QVector4D();
            m_lightData.up = QVector4D();

            // These components only apply to CG lights
            m_lightData.ambient = QVector4D(inLight->m_ambientColor, 1.0);

            m_lightData.constantAttenuation
                    = aux::translateConstantAttenuation(inLight->m_constantFade);
            m_lightData.linearAttenuation = aux::translateLinearAttenuation(inLight->m_linearFade);
            m_lightData.quadraticAttenuation
                    = aux::translateQuadraticAttenuation(inLight->m_quadraticFade);
            m_lightData.coneAngle = 180.0f;
            if (inLight->m_lightType == QSSGRenderLight::Type::Spot) {
                m_lightData.coneAngle = qCos(qDegreesToRadians(inLight->m_coneAngle));
                float innerConeAngle = inLight->m_innerConeAngle;
                if (inLight->m_innerConeAngle > inLight->m_coneAngle)
                    innerConeAngle = inLight->m_coneAngle;
                m_lightData.innerConeAngle = qCos(qDegreesToRadians(innerConeAngle));
            }
        }

        if (m_lightType == QSSGRenderLight::Type::Point) {
            memcpy(m_lightData.shadowView, QMatrix4x4().constData(), 16 * sizeof(float));
        } else {
            memcpy(m_lightData.shadowView, inLight->globalTransform.constData(), 16 * sizeof(float));
        }
    }

    static QSSGShaderLightProperties createLightEntry(const QSSGRef<QSSGRenderShaderProgram> &inShader)
    {
        return QSSGShaderLightProperties(inShader);
    }
};

/* We setup some shared state on the custom material shaders */
struct QSSGShaderGeneratorGeneratedShader
{
    typedef QHash<QSSGImageMapTypes, QSSGShaderTextureProperties> TCustomMaterialImagMap;

    QAtomicInt ref;
    QSSGRef<QSSGRenderShaderProgram> m_shader;
    // Specific properties we know the shader has to have.
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_modelMatrix;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_viewProjMatrix;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_viewMatrix;
    QSSGRenderCachedShaderProperty<QMatrix3x3> m_normalMatrix;
    QSSGRenderCachedShaderProperty<QVector3D> m_cameraPos;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_projMatrix;
    QSSGRenderCachedShaderProperty<QMatrix4x4> m_viewportMatrix;
    QSSGRenderCachedShaderProperty<QVector2D> m_camProperties;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_depthTexture;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_aoTexture;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_lightProbe;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeProps;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeOpts;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeRot;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbeOfs;
    QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *> m_lightProbe2;
    QSSGRenderCachedShaderProperty<QVector4D> m_lightProbe2Props;
    QSSGRenderCachedShaderProperty<qint32> m_lightCount;
    QSSGRenderCachedShaderProperty<qint32> m_areaLightCount;
    QSSGRenderCachedShaderProperty<qint32> m_shadowMapCount;
    QSSGRenderCachedShaderProperty<qint32> m_shadowCubeCount;
    QSSGRenderCachedShaderProperty<float> m_opacity;
    QSSGRenderCachedShaderBuffer<QSSGRenderShaderConstantBuffer> m_aoShadowParams;
    QSSGRenderCachedShaderBuffer<QSSGRenderShaderConstantBuffer> m_lightsBuffer;
    QSSGRenderCachedShaderBuffer<QSSGRenderShaderConstantBuffer> m_areaLightsBuffer;

    QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader> *m_lightsProperties;
    QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader> *m_areaLightsProperties;

    typedef QSSGRenderCachedShaderPropertyArray<QSSGRenderTexture2D *, QSSG_MAX_NUM_SHADOWS_PER_TYPE> ShadowMapPropertyArray;
    typedef QSSGRenderCachedShaderPropertyArray<QSSGRenderTextureCube *, QSSG_MAX_NUM_SHADOWS_PER_TYPE> ShadowCubePropertyArray;

    ShadowMapPropertyArray m_shadowMaps;
    ShadowCubePropertyArray m_shadowCubes;

    // Cache the image property name lookups
    TCustomMaterialImagMap m_images; // Images external to custom material usage

    explicit QSSGShaderGeneratorGeneratedShader(const QSSGRef<QSSGRenderShaderProgram> &inShader)
        : m_shader(inShader)
        , m_modelMatrix("modelMatrix", inShader)
        , m_viewProjMatrix("modelViewProjection", inShader)
        , m_viewMatrix("viewMatrix", inShader)
        , m_normalMatrix("normalMatrix", inShader)
        , m_cameraPos("cameraPosition", inShader)
        , m_projMatrix("viewProjectionMatrix", inShader)
        , m_viewportMatrix("viewportMatrix", inShader)
        , m_camProperties("cameraProperties", inShader)
        , m_depthTexture("depthTexture", inShader)
        , m_aoTexture("aoTexture", inShader)
        , m_lightProbe("lightProbe", inShader)
        , m_lightProbeProps("lightProbeProperties", inShader)
        , m_lightProbeOpts("lightProbeOptions", inShader)
        , m_lightProbeRot("lightProbeRotation", inShader)
        , m_lightProbeOfs("lightProbeOffset", inShader)
        , m_lightProbe2("lightProbe2", inShader)
        , m_lightProbe2Props("lightProbe2Properties", inShader)
        , m_lightCount("lightCount", inShader)
        , m_areaLightCount("areaLightCount", inShader)
        , m_shadowMapCount("shadowMapCount", inShader)
        , m_shadowCubeCount("shadowCubeCount", inShader)
        , m_opacity("objectOpacity", inShader)
        , m_aoShadowParams("aoShadow", inShader)
        , m_lightsBuffer("lightsBuffer", inShader)
        , m_areaLightsBuffer("areaLightsBuffer", inShader)
        , m_lightsProperties(nullptr)
        , m_areaLightsProperties(nullptr)
        , m_shadowMaps("shadowMaps[0]", inShader)
        , m_shadowCubes("shadowCubes[0]", inShader)
    {
    }

    ~QSSGShaderGeneratorGeneratedShader()
    {
        delete m_lightsProperties;
        delete m_areaLightsProperties;
    }

    QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader> *getLightProperties(int count)
    {
        if (!m_lightsProperties || m_areaLightsProperties->m_lightCountInt < count) {
            if (m_lightsProperties)
                delete m_lightsProperties;
            m_lightsProperties = new QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader>("lights", "lightCount", this, false, count);
        }
        return m_lightsProperties;
    }
    QSSGLightConstantProperties<QSSGShaderGeneratorGeneratedShader> *getAreaLightProperties(int count)
    {
        if (!m_areaLightsProperties || m_areaLightsProperties->m_lightCountInt < count) {
            if (m_areaLightsProperties)
                delete m_areaLightsProperties;
            m_areaLightsProperties = new QSSGLightConstantProperties<
                    QSSGShaderGeneratorGeneratedShader>("areaLights", "areaLightCount", this, false, count);
        }
        return m_areaLightsProperties;
    }
};
} // namespace LegacyGl

namespace {
struct QSSGShaderGenerator : public QSSGMaterialShaderGeneratorInterface
{
    typedef QPair<qint32, QSSGRef<LegacyGl::QSSGShaderLightProperties>> TCustomMaterialLightEntry;
    typedef QPair<qint32, QSSGRenderCachedShaderProperty<QSSGRenderTexture2D *>> TShadowMapEntry;
    typedef QPair<qint32, QSSGRenderCachedShaderProperty<QSSGRenderTextureCube *>> TShadowCubeEntry;

    typedef QHash<QSSGRef<QSSGRenderShaderProgram>, QSSGRef<LegacyGl::QSSGShaderGeneratorGeneratedShader>> ProgramToShaderMap;
    ProgramToShaderMap m_programToShaderMap;

    const QSSGRenderCustomMaterial *m_currentMaterial;

    QByteArray m_imageSampler;
    QByteArray m_imageFragCoords;
    QByteArray m_imageRotScale;
    QByteArray m_imageOffset;

    QVector<TCustomMaterialLightEntry> m_lightEntries;

    explicit QSSGShaderGenerator(QSSGRenderContextInterface *inRc)
        : QSSGMaterialShaderGeneratorInterface (inRc)
        , m_currentMaterial(nullptr)
    {
    }

    const QSSGRef<QSSGShaderProgramGeneratorInterface> &programGenerator() { return m_programGenerator; }
    QSSGDefaultMaterialVertexPipelineInterface &vertexGenerator() { return *m_currentPipeline; }
    QSSGShaderStageGeneratorInterface &fragmentGenerator()
    {
        return *m_programGenerator->getStage(QSSGShaderGeneratorStage::Fragment);
    }
    const QSSGRenderCustomMaterial &material() { return *m_currentMaterial; }
    bool hasTransparency() const { return m_hasTransparency; }

    quint32 convertTextureTypeValue(QSSGImageMapTypes inType)
    {
        QSSGRenderTextureTypeValue retVal = QSSGRenderTextureTypeValue::Unknown;

        switch (inType) {
        case QSSGImageMapTypes::LightmapIndirect:
            retVal = QSSGRenderTextureTypeValue::LightmapIndirect;
            break;
        case QSSGImageMapTypes::LightmapRadiosity:
            retVal = QSSGRenderTextureTypeValue::LightmapRadiosity;
            break;
        case QSSGImageMapTypes::LightmapShadow:
            retVal = QSSGRenderTextureTypeValue::LightmapShadow;
            break;
        case QSSGImageMapTypes::Bump:
            retVal = QSSGRenderTextureTypeValue::Bump;
            break;
        case QSSGImageMapTypes::Diffuse:
            retVal = QSSGRenderTextureTypeValue::Diffuse;
            break;
        case QSSGImageMapTypes::Displacement:
            retVal = QSSGRenderTextureTypeValue::Displace;
            break;
        default:
            retVal = QSSGRenderTextureTypeValue::Unknown;
            break;
        }

        Q_ASSERT(retVal != QSSGRenderTextureTypeValue::Unknown);

        return static_cast<quint32>(retVal);
    }

    ImageVariableNames getImageVariableNames(uint imageIdx) override
    {
        // convert to QSSGRenderTextureTypeValue
        QSSGRenderTextureTypeValue texType = QSSGRenderTextureTypeValue(imageIdx);
        QByteArray imageStem = toString(texType);
        imageStem.append("_");
        m_imageSampler = imageStem;
        m_imageSampler.append("sampler");
        m_imageFragCoords = imageStem;
        m_imageFragCoords.append("uv_coords");
        m_imageRotScale = imageStem;
        m_imageRotScale.append("rot_scale");
        m_imageOffset = imageStem;
        m_imageOffset.append("offset");

        ImageVariableNames retVal;
        retVal.m_imageSampler = m_imageSampler;
        retVal.m_imageFragCoords = m_imageFragCoords;
        return retVal;
    }

    void setImageShaderVariables(const QSSGRef<LegacyGl::QSSGShaderGeneratorGeneratedShader> &inShader, QSSGRenderableImage &inImage)
    {
        // skip displacement and emissive mask maps which are handled differently
        if (inImage.m_mapType == QSSGImageMapTypes::Displacement || inImage.m_mapType == QSSGImageMapTypes::Emissive)
            return;

        LegacyGl::QSSGShaderGeneratorGeneratedShader::TCustomMaterialImagMap::iterator iter = inShader->m_images.find(inImage.m_mapType);
        if (iter == inShader->m_images.end()) {
            ImageVariableNames names = getImageVariableNames(convertTextureTypeValue(inImage.m_mapType));
            inShader->m_images.insert(inImage.m_mapType,
                                      QSSGShaderTextureProperties(inShader->m_shader, names.m_imageSampler, m_imageOffset, m_imageRotScale));
            iter = inShader->m_images.find(inImage.m_mapType);
        }

        QSSGShaderTextureProperties &theShaderProps = iter.value();
        const QMatrix4x4 &textureTransform = inImage.m_image.m_textureTransform;
        const float *dataPtr(textureTransform.constData());
        QVector3D offsets(dataPtr[12], dataPtr[13], 0.0f);
        // Grab just the upper 2x2 rotation matrix from the larger matrix.
        QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

        // The image horizontal and vertical tiling modes need to be set here, before we set texture
        // on the shader.
        // because setting the image on the texture forces the textue to bind and immediately apply
        // any tex params.
        inImage.m_image.m_textureData.m_texture->setTextureWrapS(inImage.m_image.m_horizontalTilingMode);
        inImage.m_image.m_textureData.m_texture->setTextureWrapT(inImage.m_image.m_verticalTilingMode);

        theShaderProps.sampler.set(inImage.m_image.m_textureData.m_texture.data());
        theShaderProps.offsets.set(offsets);
        theShaderProps.rotations.set(rotations);
    }

    void generateImageUVCoordinates(QSSGShaderStageGeneratorInterface &, quint32, quint32, QSSGRenderableImage &) override
    {
    }

    ///< get the light constant buffer and generate if necessary
    QSSGRef<QSSGRenderConstantBuffer> getLightConstantBuffer(const QByteArray &name, qint32 inLightCount)
    {
        const QSSGRef<QSSGRenderContext> &theContext(m_renderContext->renderContext());

        // we assume constant buffer support
        Q_ASSERT(inLightCount >= 0);
        Q_ASSERT(theContext->supportsConstantBuffer());
        // we only create if if we have lights
        if (!inLightCount || !theContext->supportsConstantBuffer())
            return nullptr;

        QSSGRef<QSSGRenderConstantBuffer> pCB = theContext->getConstantBuffer(name);
        if (pCB)
            return pCB;

        // create with size of all structures + int for light count
        const size_t size = sizeof(QSSGLightSourceShader) * QSSG_MAX_NUM_LIGHTS + (4 * sizeof(qint32));
        quint8 stackData[size];
        memset(stackData, 0, 4 * sizeof(qint32));
        new (stackData + 4*sizeof(qint32)) QSSGLightSourceShader[QSSG_MAX_NUM_LIGHTS];
        QSSGByteView cBuffer(stackData, size);
        pCB = *m_constantBuffers.insert(name, new QSSGRenderConstantBuffer(theContext, name, QSSGRenderBufferUsageType::Static, cBuffer));
        if (Q_UNLIKELY(!pCB)) {
            Q_ASSERT(false);
            return nullptr;
        }

        return pCB;
    }

    bool generateVertexShader(QSSGShaderDefaultMaterialKey &, const QByteArray &inShaderPathName)
    {
        const QSSGRef<QSSGDynamicObjectSystem> &theDynamicSystem(m_renderContext->dynamicObjectSystem());
        QByteArray vertSource = theDynamicSystem->getShaderSource(inShaderPathName);

        Q_ASSERT(!vertSource.isEmpty());

        // Check if the vertex shader portion already contains a main function
        // The same string contains both the vertex and the fragment shader
        // The last "#ifdef FRAGMENT_SHADER" should mark the start of the fragment shader
        int fragmentDefStart = vertSource.indexOf("#ifdef FRAGMENT_SHADER");
        int nextIndex = fragmentDefStart;
        while (nextIndex != -1) {
            nextIndex = vertSource.indexOf("#ifdef FRAGMENT_SHADER", nextIndex + 1);
            if (nextIndex != -1)
                fragmentDefStart = nextIndex;
        }
        const int mainStart = vertSource.indexOf("void main()");

        auto &vertGenerator = vertexGenerator();

        if (mainStart != -1 && (fragmentDefStart == -1 || mainStart < fragmentDefStart)) {
            programGenerator()->beginProgram();
            vertGenerator << "#define VERTEX_SHADER\n\n";
            vertGenerator << vertSource;
            return true;
        }
        // vertex displacement
        quint32 imageIdx = 0;
        QSSGRenderableImage *displacementImage = nullptr;
        quint32 displacementImageIdx = 0;

        for (QSSGRenderableImage *img = m_firstImage; img != nullptr; img = img->m_nextImage, ++imageIdx) {
            if (img->m_mapType == QSSGImageMapTypes::Displacement) {
                displacementImage = img;
                displacementImageIdx = imageIdx;
                break;
            }
        }

        // the pipeline opens/closes up the shaders stages
        vertexGenerator().beginVertexGeneration(displacementImageIdx, displacementImage);
        return false;
    }

    QSSGRef<LegacyGl::QSSGShaderGeneratorGeneratedShader> getShaderForProgram(const QSSGRef<QSSGRenderShaderProgram> &inProgram)
    {
        auto inserter = m_programToShaderMap.constFind(inProgram);
        if (inserter == m_programToShaderMap.constEnd())
            inserter = m_programToShaderMap.insert(inProgram,
                                                   QSSGRef<LegacyGl::QSSGShaderGeneratorGeneratedShader>(
                                                           new LegacyGl::QSSGShaderGeneratorGeneratedShader(inProgram)));

        return *inserter;
    }

    virtual QSSGRef<LegacyGl::QSSGShaderLightProperties> setLight(const QSSGRef<QSSGRenderShaderProgram> &inShader,
                                                                  qint32 lightIdx,
                                                                  qint32 /*shadeIdx*/,
                                                                  const QSSGRenderLight *inLight,
                                                                  QSSGShadowMapEntry *inShadow,
                                                                  qint32 shadowIdx,
                                                                  float shadowDist)
    {
        auto it = m_lightEntries.cbegin();
        const auto end = m_lightEntries.cend();
        for (; it != end; ++it) {
            if (it->first == lightIdx && it->second->m_shader == inShader && it->second->m_lightType == inLight->m_lightType)
                break;
        }

        if (it == end) {
            // create a new name
#if 0
            QString lightName;
            if (inLight->m_lightType == QSSGRenderLight::Type::Area)
                lightName = QStringLiteral("arealights");
            else
                lightName = QStringLiteral("lights");
            char buf[16];
            qsnprintf(buf, 16, "[%d]", int(shadeIdx));
            lightName.append(QString::fromLocal8Bit(buf));
#endif

            m_lightEntries.push_back(TCustomMaterialLightEntry(lightIdx, new LegacyGl::QSSGShaderLightProperties(LegacyGl::QSSGShaderLightProperties::createLightEntry(inShader))));
            it = m_lightEntries.cend() - 1;
        }
        it->second->set(inLight);
        it->second->m_lightData.shadowControls = QVector4D(inLight->m_shadowBias, inLight->m_shadowFactor, shadowDist, 0.0);
        it->second->m_lightData.shadowIdx = (inShadow) ? shadowIdx : -1;

        return it->second;
    }

    void setShadowMaps(const QSSGRef<QSSGRenderShaderProgram> &inProgram,
                       QSSGShadowMapEntry *inShadow,
                       qint32 &numShadowMaps,
                       qint32 &numShadowCubes,
                       bool shadowMap,
                       LegacyGl::QSSGShaderGeneratorGeneratedShader::ShadowMapPropertyArray &shadowMaps,
                       LegacyGl::QSSGShaderGeneratorGeneratedShader::ShadowCubePropertyArray &shadowCubes)
    {
        Q_UNUSED(inProgram)
        if (inShadow) {
            if (!shadowMap && inShadow->m_depthCube && (numShadowCubes < QSSG_MAX_NUM_SHADOWS_PER_TYPE)) {
                shadowCubes.m_array[numShadowCubes] = inShadow->m_depthCube.data();
                ++numShadowCubes;
            } else if (shadowMap && inShadow->m_depthMap && (numShadowMaps < QSSG_MAX_NUM_SHADOWS_PER_TYPE)) {
                shadowMaps.m_array[numShadowMaps] = inShadow->m_depthMap.data();
                ++numShadowMaps;
            }
        }
    }

    void setGlobalProperties(const QSSGRef<QSSGRenderShaderProgram> &inProgram,
                             const QSSGRenderLayer & /*inLayer*/,
                             QSSGRenderCamera &inCamera,
                             const QVector3D &,
                             const QVector<QSSGRenderLight *> &inLights,
                             const QVector<QVector3D> &,
                             const QSSGRef<QSSGRenderShadowMap> &inShadowMaps,
                             bool receivesShadows = true)
    {
        const QSSGRef<LegacyGl::QSSGShaderGeneratorGeneratedShader> &theShader(getShaderForProgram(inProgram));
        m_renderContext->renderContext()->setActiveShader(inProgram);

        QSSGRenderCamera &theCamera(inCamera);

        QVector2D camProps(theCamera.clipNear, theCamera.clipFar);
        theShader->m_camProperties.set(camProps);
        theShader->m_cameraPos.set(theCamera.getGlobalPos());

        if (theShader->m_viewMatrix.isValid())
            theShader->m_viewMatrix.set(theCamera.globalTransform.inverted());

        if (theShader->m_projMatrix.isValid()) {
            QMatrix4x4 vProjMat;
            inCamera.calculateViewProjectionMatrix(vProjMat);
            theShader->m_projMatrix.set(vProjMat);
        }

        // set lights separate for area lights
        qint32 cgLights = 0, areaLights = 0;
        qint32 numShadowMaps = 0, numShadowCubes = 0;

        // this call setup the constant buffer for ambient occlusion and shadow
        theShader->m_aoShadowParams.set();

        if (m_renderContext->renderContext()->supportsConstantBuffer()) {
            // Count area lights before processing
            for (int lightIdx = 0; lightIdx < inLights.size(); ++lightIdx) {
                if (inLights[lightIdx]->m_lightType == QSSGRenderLight::Type::Area)
                    areaLights++;
                else
                    cgLights++;
            }

            const QSSGRef<QSSGRenderConstantBuffer> &pLightCb
                        = getLightConstantBuffer(QByteArrayLiteral("lightsBuffer"),
                                                 cgLights);
            const QSSGRef<QSSGRenderConstantBuffer> &pAreaLightCb
                        = getLightConstantBuffer(QByteArrayLiteral("areaLightsBuffer"),
                                                 areaLights);

            areaLights = 0;
            cgLights = 0;
            // Split the count between CG lights and area lights
            for (int lightIdx = 0; lightIdx < inLights.size(); ++lightIdx) {
                QSSGShadowMapEntry *theShadow = nullptr;
                qint32 shdwIdx = 0;

                if (receivesShadows) {
                    if (inShadowMaps && inLights[lightIdx]->m_castShadow)
                        theShadow = inShadowMaps->getShadowMapEntry(lightIdx);

                    shdwIdx = (inLights[lightIdx]->m_lightType != QSSGRenderLight::Type::Directional) ? numShadowCubes : numShadowMaps;
                    setShadowMaps(inProgram,
                                  theShadow,
                                  numShadowMaps,
                                  numShadowCubes,
                                  inLights[lightIdx]->m_lightType == QSSGRenderLight::Type::Directional,
                                  theShader->m_shadowMaps,
                                  theShader->m_shadowCubes);
                }
                if (inLights[lightIdx]->m_lightType == QSSGRenderLight::Type::Area) {
                    const QSSGRef<LegacyGl::QSSGShaderLightProperties> &theAreaLightEntry = setLight(inProgram,
                                                                                               lightIdx,
                                                                                               areaLights,
                                                                                               inLights[lightIdx],
                                                                                               theShadow,
                                                                                               shdwIdx,
                                                                                               inCamera.clipFar);

                    if (theAreaLightEntry && pAreaLightCb) {
                        pAreaLightCb->updateRaw(areaLights * sizeof(QSSGLightSourceShader) + (4 * sizeof(qint32)),
                                                toByteView(theAreaLightEntry->m_lightData));
                    }
                    areaLights++;
                } else {
                    const QSSGRef<LegacyGl::QSSGShaderLightProperties> &theLightEntry = setLight(inProgram,
                                                                                           lightIdx,
                                                                                           cgLights,
                                                                                           inLights[lightIdx],
                                                                                           theShadow,
                                                                                           shdwIdx,
                                                                                           inCamera.clipFar);

                    if (theLightEntry && pLightCb) {
                        pLightCb->updateRaw(cgLights * sizeof(QSSGLightSourceShader) + (4 * sizeof(qint32)),
                                            toByteView(theLightEntry->m_lightData));
                    }

                    cgLights++;
                }
            }

            if (pLightCb) {
                pLightCb->updateRaw(0, toByteView(cgLights));
                theShader->m_lightsBuffer.set();
            }
            if (pAreaLightCb) {
                pAreaLightCb->updateRaw(0, toByteView(areaLights));
                theShader->m_areaLightsBuffer.set();
            }

            theShader->m_lightCount.set(cgLights);
            theShader->m_areaLightCount.set(areaLights);
        } else {
            QVector<QSSGRef<LegacyGl::QSSGShaderLightProperties>> lprop;
            QVector<QSSGRef<LegacyGl::QSSGShaderLightProperties>> alprop;
            for (int lightIdx = 0; lightIdx < inLights.size(); ++lightIdx) {

                QSSGShadowMapEntry *theShadow = nullptr;
                qint32 shdwIdx = 0;

                if (receivesShadows) {
                    if (inShadowMaps && inLights[lightIdx]->m_castShadow)
                        theShadow = inShadowMaps->getShadowMapEntry(lightIdx);

                    shdwIdx = (inLights[lightIdx]->m_lightType != QSSGRenderLight::Type::Directional) ? numShadowCubes : numShadowMaps;
                    setShadowMaps(inProgram,
                                  theShadow,
                                  numShadowMaps,
                                  numShadowCubes,
                                  inLights[lightIdx]->m_lightType == QSSGRenderLight::Type::Directional,
                                  theShader->m_shadowMaps,
                                  theShader->m_shadowCubes);
                }

                const QSSGRef<LegacyGl::QSSGShaderLightProperties> &p = setLight(inProgram, lightIdx, areaLights, inLights[lightIdx], theShadow, shdwIdx, inCamera.clipFar);
                if (inLights[lightIdx]->m_lightType == QSSGRenderLight::Type::Area)
                    alprop.push_back(p);
                else
                    lprop.push_back(p);
            }
            QSSGLightConstantProperties<LegacyGl::QSSGShaderGeneratorGeneratedShader> *lightProperties = theShader->getLightProperties(
                    lprop.size());
            QSSGLightConstantProperties<LegacyGl::QSSGShaderGeneratorGeneratedShader> *areaLightProperties = theShader->getAreaLightProperties(
                    alprop.size());

            lightProperties->updateLights(lprop);
            areaLightProperties->updateLights(alprop);

            theShader->m_lightCount.set(lprop.size());
            theShader->m_areaLightCount.set(alprop.size());
        }
        for (int i = numShadowMaps; i < QSSG_MAX_NUM_SHADOWS_PER_TYPE; ++i)
            theShader->m_shadowMaps.m_array[i] = nullptr;
        for (int i = numShadowCubes; i < QSSG_MAX_NUM_SHADOWS_PER_TYPE; ++i)
            theShader->m_shadowCubes.m_array[i] = nullptr;
        theShader->m_shadowMaps.set(numShadowMaps);
        theShader->m_shadowCubes.set(numShadowCubes);
        theShader->m_shadowMapCount.set(numShadowMaps);
        theShader->m_shadowCubeCount.set(numShadowCubes);
    }

    void setMaterialProperties(const QSSGRef<QSSGRenderShaderProgram> &inProgram,
                               const QSSGRenderCustomMaterial &inMaterial,
                               const QVector2D &,
                               const QMatrix4x4 &inModelViewProjection,
                               const QMatrix3x3 &inNormalMatrix,
                               const QMatrix4x4 &inGlobalTransform,
                               QSSGRenderableImage *inFirstImage,
                               float inOpacity,
                               const QSSGRef<QSSGRenderTexture2D> &inDepthTexture,
                               const QSSGRef<QSSGRenderTexture2D> &inSSaoTexture,
                               QSSGRenderImage *inLightProbe,
                               QSSGRenderImage *inLightProbe2,
                               float inProbeHorizon,
                               float inProbeBright,
                               float inProbe2Window,
                               float inProbe2Pos,
                               float inProbe2Fade,
                               float inProbeFOV)
    {
        const QSSGRef<QSSGMaterialSystem> &theMaterialSystem(m_renderContext->customMaterialSystem());
        const QSSGRef<LegacyGl::QSSGShaderGeneratorGeneratedShader> &theShader(getShaderForProgram(inProgram));

        theShader->m_viewProjMatrix.set(inModelViewProjection);
        theShader->m_normalMatrix.set(inNormalMatrix);
        theShader->m_modelMatrix.set(inGlobalTransform);

        theShader->m_depthTexture.set(inDepthTexture.data());
        theShader->m_aoTexture.set(inSSaoTexture.data());

        theShader->m_opacity.set(inOpacity);

        QSSGRenderImage *theLightProbe = inLightProbe;
        QSSGRenderImage *theLightProbe2 = inLightProbe2;

        if (inMaterial.m_iblProbe && inMaterial.m_iblProbe->m_textureData.m_texture) {
            theLightProbe = inMaterial.m_iblProbe;
        }

        if (theLightProbe) {
            if (theLightProbe->m_textureData.m_texture) {
                QSSGRenderTextureCoordOp theHorzLightProbeTilingMode = QSSGRenderTextureCoordOp::Repeat;
                QSSGRenderTextureCoordOp theVertLightProbeTilingMode = theLightProbe->m_verticalTilingMode;
                theLightProbe->m_textureData.m_texture->setTextureWrapS(theHorzLightProbeTilingMode);
                theLightProbe->m_textureData.m_texture->setTextureWrapT(theVertLightProbeTilingMode);

                const QMatrix4x4 &textureTransform = theLightProbe->m_textureTransform;
                // We separate rotational information from offset information so that just maybe the
                // shader
                // will attempt to push less information to the card.
                const float *dataPtr(textureTransform.constData());
                // The third member of the offsets contains a flag indicating if the texture was
                // premultiplied or not.
                // We use this to mix the texture alpha.
                // lightProbeOffsets.w is now no longer being used to enable/disable fast IBL,
                // (it's now the only option)
                // So now, it's storing the number of mip levels in the IBL image.
                QVector4D offsets(dataPtr[12],
                                  dataPtr[13],
                                  theLightProbe->m_textureData.m_textureFlags.isPreMultiplied() ? 1.0f : 0.0f,
                                  float(theLightProbe->m_textureData.m_texture->numMipmaps()));
                // Fast IBL is always on;
                // inRenderContext.m_Layer.m_FastIbl ? 1.0f : 0.0f );
                // Grab just the upper 2x2 rotation matrix from the larger matrix.
                QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

                theShader->m_lightProbeRot.set(rotations);
                theShader->m_lightProbeOfs.set(offsets);

                if ((!inMaterial.m_iblProbe) && (inProbeFOV < 180.f)) {
                    theShader->m_lightProbeOpts.set(QVector4D(0.01745329251994329547f * inProbeFOV, 0.0f, 0.0f, 0.0f));
                }

                // Also make sure to add the secondary texture, but it should only be added if the
                // primary
                // (i.e. background) texture is also there.
                if (theLightProbe2 && theLightProbe2->m_textureData.m_texture) {
                    theLightProbe2->m_textureData.m_texture->setTextureWrapS(theHorzLightProbeTilingMode);
                    theLightProbe2->m_textureData.m_texture->setTextureWrapT(theVertLightProbeTilingMode);
                    theShader->m_lightProbe2.set(theLightProbe2->m_textureData.m_texture.data());
                    theShader->m_lightProbe2Props.set(QVector4D(inProbe2Window, inProbe2Pos, inProbe2Fade, 1.0f));

                    const QMatrix4x4 &xform2 = theLightProbe2->m_textureTransform;
                    const float *dataPtr(xform2.constData());

                    theShader->m_lightProbeProps.set(QVector4D(dataPtr[12], dataPtr[13], inProbeHorizon, inProbeBright * 0.01f));
                } else {
                    theShader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
                    theShader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, inProbeHorizon, inProbeBright * 0.01f));
                }
            } else {
                theShader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, -1.0f, 0.0f));
                theShader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
            }

            theShader->m_lightProbe.set(theLightProbe->m_textureData.m_texture.data());

        } else {
            theShader->m_lightProbeProps.set(QVector4D(0.0f, 0.0f, -1.0f, 0.0f));
            theShader->m_lightProbe2Props.set(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
        }

        // finally apply custom material shader properties
        theMaterialSystem->applyShaderPropertyValues(inMaterial, inProgram);

        // additional textures
        for (QSSGRenderableImage *theImage = inFirstImage; theImage; theImage = theImage->m_nextImage)
            setImageShaderVariables(theShader, *theImage);
    }

    void setMaterialProperties(const QSSGRef<QSSGRenderShaderProgram> &inProgram,
                               const QSSGRenderGraphObject &inMaterial,
                               const QVector2D &inCameraVec,
                               const QMatrix4x4 &inModelViewProjection,
                               const QMatrix3x3 &inNormalMatrix,
                               const QMatrix4x4 &inGlobalTransform,
                               QSSGRenderableImage *inFirstImage,
                               float inOpacity,
                               const QSSGLayerGlobalRenderProperties &inRenderProperties,
                               bool receivesShadows) override
    {
        const QSSGRenderCustomMaterial &theCustomMaterial = static_cast<const QSSGRenderCustomMaterial &>(inMaterial);
        Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::CustomMaterial);

        setGlobalProperties(inProgram,
                            inRenderProperties.layer,
                            inRenderProperties.camera,
                            inRenderProperties.cameraDirection,
                            inRenderProperties.lights,
                            inRenderProperties.lightDirections,
                            inRenderProperties.shadowMapManager,
                            receivesShadows);

        setMaterialProperties(inProgram,
                              theCustomMaterial,
                              inCameraVec,
                              inModelViewProjection,
                              inNormalMatrix,
                              inGlobalTransform,
                              inFirstImage,
                              inOpacity,
                              inRenderProperties.depthTexture,
                              inRenderProperties.ssaoTexture,
                              inRenderProperties.lightProbe,
                              inRenderProperties.lightProbe2,
                              inRenderProperties.probeHorizon,
                              inRenderProperties.probeBright,
                              inRenderProperties.probe2Window,
                              inRenderProperties.probe2Pos,
                              inRenderProperties.probe2Fade,
                              inRenderProperties.probeFOV);
    }

    void setRhiLightBufferData(QSSGLightSourceShader *lightData, QSSGRenderLight *light, float clipFar, int shadowIdx)
    {
        QVector3D dir(0, 0, 1);
        if (light->m_lightType == QSSGRenderLight::Type::Directional) {
            dir = light->getScalingCorrectDirection();
            // we lit in world sapce
            dir *= -1;
            lightData->position = QVector4D(dir, 0.0);
        } else if (light->m_lightType == QSSGRenderLight::Type::Area
                   || light->m_lightType == QSSGRenderLight::Type::Spot) {
            dir = light->getScalingCorrectDirection();
            lightData->position = QVector4D(light->getGlobalPos(), 1.0);
        } else {
            dir = light->getGlobalPos();
            lightData->position = QVector4D(dir, 1.0);
        }

        lightData->direction = QVector4D(dir, 0.0);

        float normalizedBrightness = aux::translateBrightness(light->m_brightness);
        lightData->diffuse = QVector4D(light->m_diffuseColor * normalizedBrightness, 1.0);
        lightData->specular = QVector4D(light->m_specularColor * normalizedBrightness, 1.0);

        if (light->m_lightType == QSSGRenderLight::Type::Area) {
            lightData->width = light->m_areaWidth;
            lightData->height = light->m_areaWidth;

            QMatrix3x3 theDirMatrix(mat44::getUpper3x3(light->globalTransform));
            lightData->right = QVector4D(mat33::transform(theDirMatrix, QVector3D(1, 0, 0)), light->m_areaWidth);
            lightData->up = QVector4D(mat33::transform(theDirMatrix, QVector3D(0, 1, 0)), light->m_areaHeight);
        } else {
            lightData->width = 0.0;
            lightData->height = 0.0;
            lightData->right = QVector4D();
            lightData->up = QVector4D();

            // These components only apply to CG lights
            lightData->ambient = QVector4D(light->m_ambientColor, 1.0);

            lightData->constantAttenuation
                    = aux::translateConstantAttenuation(light->m_constantFade);
            lightData->linearAttenuation = aux::translateLinearAttenuation(light->m_linearFade);
            lightData->quadraticAttenuation
                    = aux::translateQuadraticAttenuation(light->m_quadraticFade);
            lightData->coneAngle = 180.0f;
            if (light->m_lightType == QSSGRenderLight::Type::Spot) {
                lightData->coneAngle = qCos(qDegreesToRadians(light->m_coneAngle));
                float innerConeAngle = light->m_innerConeAngle;
                if (light->m_innerConeAngle < 0)
                    innerConeAngle = light->m_coneAngle * 0.7f;
                else if (light->m_innerConeAngle > light->m_coneAngle)
                    innerConeAngle = light->m_coneAngle;
                lightData->innerConeAngle = qCos(qDegreesToRadians(innerConeAngle));
            }
        }

        if (light->m_lightType == QSSGRenderLight::Type::Point) {
            QMatrix4x4 ident;
            memcpy(lightData->shadowView, ident.constData(), 16 * sizeof(float));
        } else {
            memcpy(lightData->shadowView, light->globalTransform.constData(), 16 * sizeof(float));
        }

        lightData->shadowControls = QVector4D(light->m_shadowBias, light->m_shadowFactor, clipFar, 0.0);
        lightData->shadowIdx = shadowIdx;
    }

    void setRhiMaterialProperties(QSSGRef<QSSGRhiShaderStagesWithResources> &shaders,
                                  QSSGRhiGraphicsPipelineState *inPipelineState,
                                  const QSSGRenderGraphObject &inMaterial,
                                  const QVector2D &inCameraVec,
                                  const QMatrix4x4 &inModelViewProjection,
                                  const QMatrix3x3 &inNormalMatrix,
                                  const QMatrix4x4 &inGlobalTransform,
                                  QSSGRenderableImage *inFirstImage,
                                  float inOpacity,
                                  const QSSGLayerGlobalRenderProperties &inRenderProperties,
                                  bool receivesShadows) override
    {
        Q_UNUSED(inPipelineState);
        Q_UNUSED(inCameraVec);
        Q_UNUSED(inFirstImage);
        Q_UNUSED(inOpacity);
        Q_UNUSED(inRenderProperties);
        Q_UNUSED(receivesShadows);

        const QSSGRenderCustomMaterial &material(static_cast<const QSSGRenderCustomMaterial &>(inMaterial));
        QSSGMaterialSystem *materialSystem = m_renderContext->customMaterialSystem().data();

        materialSystem->applyRhiShaderPropertyValues(material, shaders);

        QSSGRenderCamera &theCamera(inRenderProperties.camera);

        const QVector3D camGlobalPos = theCamera.getGlobalPos();
        shaders->setUniform(QByteArrayLiteral("cameraPosition"), &camGlobalPos, 3 * sizeof(float));
        shaders->setUniform(QByteArrayLiteral("cameraDirection"), &inRenderProperties.cameraDirection, 3 * sizeof(float));
        QVector2D camProps(theCamera.clipNear, theCamera.clipFar);
        shaders->setUniform(QByteArrayLiteral("cameraProperties"), &camProps, 2 * sizeof(float));

        const QMatrix4x4 clipSpaceCorrMatrix = m_renderContext->renderContext()->rhiContext()->rhi()->clipSpaceCorrMatrix();
        QMatrix4x4 viewProj;
        theCamera.calculateViewProjectionMatrix(viewProj);
        viewProj = clipSpaceCorrMatrix * viewProj;
        shaders->setUniform(QByteArrayLiteral("viewProjectionMatrix"), viewProj.constData(), 16 * sizeof(float));

        const QMatrix4x4 viewMatrix = theCamera.globalTransform.inverted();
        shaders->setUniform(QByteArrayLiteral("viewMatrix"), viewMatrix.constData(), 16 * sizeof(float));

        const QMatrix4x4 mvp = clipSpaceCorrMatrix * inModelViewProjection;
        shaders->setUniform(QByteArrayLiteral("modelViewProjection"), mvp.constData(), 16 * sizeof(float));

        // mat3 is still 4 floats per column in the uniform buffer (but there
        // is no 4th column), so 48 bytes altogether, not 36 or 64.
        float normalMatrix[12];
        memcpy(normalMatrix, inNormalMatrix.constData(), 3 * sizeof(float));
        memcpy(normalMatrix + 4, inNormalMatrix.constData() + 3, 3 * sizeof(float));
        memcpy(normalMatrix + 8, inNormalMatrix.constData() + 6, 3 * sizeof(float));
        shaders->setUniform(QByteArrayLiteral("normalMatrix"), normalMatrix, 12 * sizeof(float));

        shaders->setUniform(QByteArrayLiteral("modelMatrix"), inGlobalTransform.constData(), 16 * sizeof(float));

        shaders->setUniform(QByteArrayLiteral("objectOpacity"), &inOpacity, sizeof(float));

        shaders->setDepthTexture(inRenderProperties.rhiDepthTexture);
        shaders->setSsaoTexture(inRenderProperties.rhiSsaoTexture);

        const QSSGRhiShaderStagesWithResources::LightBufferSlot nonAreaLightBuf = QSSGRhiShaderStagesWithResources::LightBuffer0;
        const QSSGRhiShaderStagesWithResources::LightBufferSlot areaLightBuf = QSSGRhiShaderStagesWithResources::LightBuffer1;
        shaders->resetLights(nonAreaLightBuf);
        shaders->resetLights(areaLightBuf);

        qint32 nonAreaLightCount = 0;
        qint32 areaLightCount = 0;

        qint32 shadowMapCount = 0;
        qint32 shadowCubeCount = 0;

        QVarLengthArray<QRhiTexture *, QSSG_MAX_NUM_SHADOWS_PER_TYPE> shadowMapTextures;
        QVarLengthArray<QRhiTexture *, QSSG_MAX_NUM_SHADOWS_PER_TYPE> shadowCubeTextures;

        for (quint32 lightIdx = 0, lightEnd = inRenderProperties.lights.size();
             lightIdx < lightEnd && lightIdx < QSSG_MAX_NUM_LIGHTS; ++lightIdx)
        {
            QSSGRenderLight *light(inRenderProperties.lights[lightIdx]);
            int shadowIdx = -1;
            if (receivesShadows) {
                QSSGShadowMapEntry *shadowMapEntry = nullptr;
                if (inRenderProperties.shadowMapManager && light->m_castShadow)
                    shadowMapEntry = inRenderProperties.shadowMapManager->getShadowMapEntry(lightIdx);
                const bool isDirectional = light->m_lightType == QSSGRenderLight::Type::Directional;
                if (shadowMapEntry) {
                    if (!isDirectional && shadowMapEntry->m_rhiDepthCube && shadowCubeCount < QSSG_MAX_NUM_SHADOWS_PER_TYPE) {
                        shadowCubeTextures.append(shadowMapEntry->m_rhiDepthCube);
                        shadowIdx = shadowCubeCount++;
                    } else if (isDirectional && shadowMapEntry->m_rhiDepthMap && shadowMapCount < QSSG_MAX_NUM_SHADOWS_PER_TYPE) {
                        shadowMapTextures.append(shadowMapEntry->m_rhiDepthMap);
                        shadowIdx = shadowMapCount++;
                    }
                }
            }
            if (light->m_lightType == QSSGRenderLight::Type::Area) {
                ::QSSGShaderLightProperties &lightProperties(shaders->addLight(areaLightBuf));
                setRhiLightBufferData(&lightProperties.lightData, light, theCamera.clipFar, shadowIdx);
                ++areaLightCount;
            } else {
                ::QSSGShaderLightProperties &lightProperties(shaders->addLight(nonAreaLightBuf));
                setRhiLightBufferData(&lightProperties.lightData, light, theCamera.clipFar, shadowIdx);
                ++nonAreaLightCount;
            }
        }

        shaders->setUniform(QByteArrayLiteral("lightCount"), &nonAreaLightCount, sizeof(qint32));
        shaders->setUniform(QByteArrayLiteral("areaLightCount"), &areaLightCount, sizeof(qint32));

        shaders->setUniform(QByteArrayLiteral("shadowMapCount"), &shadowMapCount, sizeof(qint32));
        shaders->setUniform(QByteArrayLiteral("shadowCubeCount"), &shadowCubeCount, sizeof(qint32));

        shaders->resetShadowMapArrays();
        if (!shadowMapTextures.isEmpty()) {
            QSSGRhiShadowMapArrayProperties &p(shaders->addShadowMapArray());
            p.shadowMapArrayUniformName = QByteArrayLiteral("shadowMaps");
            p.isCubemap = false;
            for (QRhiTexture *texture : shadowMapTextures)
                p.shadowMapTextures.append(texture);
        }
        if (!shadowCubeTextures.isEmpty()) {
            QSSGRhiShadowMapArrayProperties &p(shaders->addShadowMapArray());
            p.shadowMapArrayUniformName = QByteArrayLiteral("shadowCubes");
            p.isCubemap = true;
            for (QRhiTexture *texture : shadowCubeTextures)
                p.shadowMapTextures.append(texture);
        }

        QSSGRenderImage *theLightProbe = inRenderProperties.lightProbe;
        //QSSGRenderImage *theLightProbe2 = inRenderProperties.lightProbe2;

        if (material.m_iblProbe && material.m_iblProbe->m_textureData.m_rhiTexture)
            theLightProbe = material.m_iblProbe;

        if (theLightProbe && theLightProbe->m_textureData.m_rhiTexture) {
            QSSGRenderTextureCoordOp theHorzLightProbeTilingMode = theLightProbe->m_horizontalTilingMode;
            QSSGRenderTextureCoordOp theVertLightProbeTilingMode = theLightProbe->m_verticalTilingMode;
            const QMatrix4x4 &textureTransform = theLightProbe->m_textureTransform;
            // We separate rotational information from offset information so that just maybe the
            // shader
            // will attempt to push less information to the card.
            const float *dataPtr(textureTransform.constData());
            // The third member of the offsets contains a flag indicating if the texture was
            // premultiplied or not.
            // We use this to mix the texture alpha.

            // The fourth member claims to be the number of mipmaps, but the directGL code path
            // actually uses the maximum mipmap level
            // TODO: clean up this!
            int maxMipLevel = theLightProbe->m_textureData.m_mipmaps - 1;

            QVector4D offsets(dataPtr[12],
                              dataPtr[13],
                              theLightProbe->m_textureData.m_textureFlags.isPreMultiplied() ? 1.0f : 0.0f,
                              float(maxMipLevel));

            // Grab just the upper 2x2 rotation matrix from the larger matrix.
            QVector4D rotations(dataPtr[0], dataPtr[4], dataPtr[1], dataPtr[5]);

            shaders->setUniform(QByteArrayLiteral("lightProbeRotation"), &rotations, 4 * sizeof(float));
            shaders->setUniform(QByteArrayLiteral("lightProbeOffset"), &offsets, 4 * sizeof(float));

            if (!material.m_iblProbe && inRenderProperties.probeFOV < 180.f) {
                QVector4D opts(0.01745329251994329547f * inRenderProperties.probeFOV, 0.0f, 0.0f, 0.0f);
                shaders->setUniform(QByteArrayLiteral("lightProbeOptions"), &opts, 4 * sizeof(float));
            }

            QVector4D emptyProps2(0.0f, 0.0f, 0.0f, 0.0f);
            shaders->setUniform(QByteArrayLiteral("lightProbe2Properties"), &emptyProps2, 4 * sizeof(float));

            QVector4D props(0.0f, 0.0f, inRenderProperties.probeHorizon, inRenderProperties.probeBright * 0.01f);
            shaders->setUniform(QByteArrayLiteral("lightProbeProperties"), &props, 4 * sizeof(float));
            shaders->setLightProbeTexture(theLightProbe->m_textureData.m_rhiTexture, theHorzLightProbeTilingMode, theVertLightProbeTilingMode);
        } else {
            // no lightprobe
            QVector4D emptyProps(0.0f, 0.0f, -1.0f, 0.0f);
            shaders->setUniform(QByteArrayLiteral("lightProbeProperties"), &emptyProps, 4 * sizeof(float));

            QVector4D emptyProps2(0.0f, 0.0f, 0.0f, 0.0f);
            shaders->setUniform(QByteArrayLiteral("lightProbe2Properties"), &emptyProps2, 4 * sizeof(float));

            shaders->setLightProbeTexture(nullptr);
        }
    }

    void generateLightmapIndirectFunc(QSSGShaderStageGeneratorInterface &inFragmentShader, QSSGRenderImage *pEmissiveLightmap)
    {
        inFragmentShader << "\n"
                            "vec3 computeMaterialLightmapIndirect()\n{\n"
                            "  vec4 indirect = vec4( 0.0, 0.0, 0.0, 0.0 );\n";
        if (pEmissiveLightmap) {
            ImageVariableNames names = getImageVariableNames(convertTextureTypeValue(QSSGImageMapTypes::LightmapIndirect));
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  indirect = evalIndirectLightmap( " << m_imageSampler << ", varTexCoord1, "
                             << m_imageRotScale << ", "
                             << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return indirect.rgb;\n"
                            "}\n\n";
    }

    void generateLightmapRadiosityFunc(QSSGShaderStageGeneratorInterface &inFragmentShader, QSSGRenderImage *pRadiosityLightmap)
    {
        inFragmentShader << "\n"
                            "vec3 computeMaterialLightmapRadiosity()\n{\n"
                            "  vec4 radiosity = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pRadiosityLightmap) {
            ImageVariableNames names = getImageVariableNames(convertTextureTypeValue(QSSGImageMapTypes::LightmapRadiosity));
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  radiosity = evalRadiosityLightmap( " << m_imageSampler << ", varTexCoord1, "
                             << m_imageRotScale << ", "
                             << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return radiosity.rgb;\n"
                            "}\n\n";
    }

    void generateLightmapShadowFunc(QSSGShaderStageGeneratorInterface &inFragmentShader, QSSGRenderImage *pBakedShadowMap)
    {
        inFragmentShader << "\n"
                            "vec4 computeMaterialLightmapShadow()\n{\n"
                            "  vec4 shadowMask = vec4( 1.0, 1.0, 1.0, 1.0 );\n";
        if (pBakedShadowMap) {
            ImageVariableNames names = getImageVariableNames(static_cast<quint32>(QSSGRenderTextureTypeValue::LightmapShadow));
            // Add uniforms
            inFragmentShader.addUniform(names.m_imageSampler, "sampler2D");
            inFragmentShader.addUniform(m_imageOffset, "vec3");
            inFragmentShader.addUniform(m_imageRotScale, "vec4");

            inFragmentShader << "\n  shadowMask = evalShadowLightmap( " << m_imageSampler << ", texCoord0, "
                             << m_imageRotScale << ", "
                             << m_imageOffset << " );\n\n";
        }

        inFragmentShader << "  return shadowMask;\n"
                            "}\n\n";
    }

    void generateLightmapIndirectSetupCode(QSSGShaderStageGeneratorInterface &inFragmentShader,
                                           QSSGRenderableImage *pIndirectLightmap,
                                           QSSGRenderableImage *pRadiosityLightmap)
    {
        if (!pIndirectLightmap && !pRadiosityLightmap)
            return;

        QByteArray finalValue;

        inFragmentShader << "\n"
                            "void initializeLayerVariablesWithLightmap(void)\n{\n";
        if (pIndirectLightmap) {
            inFragmentShader << "  vec3 lightmapIndirectValue = computeMaterialLightmapIndirect( );\n";
            finalValue.append("vec4(lightmapIndirectValue, 1.0)");
        }
        if (pRadiosityLightmap) {
            inFragmentShader << "  vec3 lightmapRadisoityValue = computeMaterialLightmapRadiosity( );\n";
            if (finalValue.isEmpty())
                finalValue.append("vec4(lightmapRadisoityValue, 1.0)");
            else
                finalValue.append(" + vec4(lightmapRadisoityValue, 1.0)");
        }

        finalValue.append(";\n");

        inFragmentShader << "  layer.base += " << finalValue;
        inFragmentShader << "  layer.layer += " << finalValue;

        inFragmentShader << "}\n\n";
    }

    void generateLightmapShadowCode(QSSGShaderStageGeneratorInterface &inFragmentShader, QSSGRenderableImage *pBakedShadowMap)
    {
        if (pBakedShadowMap) {
            inFragmentShader << " tmpShadowTerm *= computeMaterialLightmapShadow( );\n\n";
        }
    }

    void applyEmissiveMask(QSSGShaderStageGeneratorInterface &inFragmentShader, QSSGRenderImage *pEmissiveMaskMap)
    {
        inFragmentShader << "\n"
                            "vec3 computeMaterialEmissiveMask()\n{\n"
                            "  vec3 emissiveMask = vec3( 1.0, 1.0, 1.0 );\n";
        if (pEmissiveMaskMap) {
            inFragmentShader << "  texture_coordinate_info tci;\n"
                                "  texture_coordinate_info transformed_tci;\n"
                                "  tci = textureCoordinateInfo( texCoord0, tangent, binormal );\n"
                                "  transformed_tci = transformCoordinate( "
                                "rotationTranslationScale( vec3( 0.000000, 0.000000, 0.000000 ), "
                                "vec3( 0.000000, 0.000000, 0.000000 ), vec3( 1.000000, 1.000000, "
                                "1.000000 ) ), tci );\n"
                                "  emissiveMask = fileTexture( " << pEmissiveMaskMap->m_imageShaderName.toUtf8()
                             << ", vec3( 0, 0, 0 ), vec3( 1, 1, 1 ), mono_alpha, transformed_tci, "
                             << "vec2( 0.000000, 1.000000 ), vec2( 0.000000, 1.000000 ), "
                                "wrap_repeat, wrap_repeat, gamma_default ).tint;\n";
        }

        inFragmentShader << "  return emissiveMask;\n"
                            "}\n\n";
    }

    void registerNonSnippetUnconditionalUniforms(QSSGShaderStageGeneratorInterface &fs)
    {
        fs.addUniform("modelMatrix", "mat4");
        fs.addUniform("modelViewProjection", "mat4");
        fs.addUniform("viewMatrix", "mat4");
        fs.addUniform("normalMatrix", "mat3");
        fs.addUniform("cameraPosition", "vec3");
        fs.addUniform("viewProjectionMatrix", "mat4");
        fs.addUniform("viewportMatrix", "mat4");
        fs.addUniform("cameraProperties", "vec2");
        fs.addUniform("lightCount", "int");
        fs.addUniform("areaLightCount", "int");
        fs.addUniform("objectOpacity", "float");
    }

    bool generateFragmentShader(QSSGShaderDefaultMaterialKey &inKey,
                                const QByteArray &inShaderPathName,
                                bool hasCustomVertShader)
    {
        const QSSGRef<QSSGDynamicObjectSystem> &theDynamicSystem(m_renderContext->dynamicObjectSystem());
        QByteArray fragSource = theDynamicSystem->getShaderSource(inShaderPathName);

        Q_ASSERT(!fragSource.isEmpty());

        // light maps
        bool hasLightmaps = false;
        QSSGRenderableImage *lightmapShadowImage = nullptr;
        QSSGRenderableImage *lightmapIndirectImage = nullptr;
        QSSGRenderableImage *lightmapRadisoityImage = nullptr;

        for (QSSGRenderableImage *img = m_firstImage; img != nullptr; img = img->m_nextImage) {
            if (img->m_mapType == QSSGImageMapTypes::LightmapIndirect) {
                lightmapIndirectImage = img;
                hasLightmaps = true;
            } else if (img->m_mapType == QSSGImageMapTypes::LightmapRadiosity) {
                lightmapRadisoityImage = img;
                hasLightmaps = true;
            } else if (img->m_mapType == QSSGImageMapTypes::LightmapShadow) {
                lightmapShadowImage = img;
            }
        }

        if (!hasCustomVertShader) {
            vertexGenerator().generateUVCoords(0, key());
            // for lightmaps we expect a second set of uv coordinates
            if (hasLightmaps)
                vertexGenerator().generateUVCoords(1, key());
        }

        QSSGDefaultMaterialVertexPipelineInterface &vertexShader(vertexGenerator());
        QSSGShaderStageGeneratorInterface &fragmentShader(fragmentGenerator());

        QByteArray srcString(fragSource);

        if (m_renderContext->renderContext()->renderContextType() == QSSGRenderContextType::GLES2) {
            QString::size_type pos = 0;
            while ((pos = srcString.indexOf("out vec4 fragColor", pos)) != -1) {
                srcString.insert(pos, "//");
                pos += int(strlen("//out vec4 fragColor"));
            }
        }

        fragmentShader << "#define FRAGMENT_SHADER\n\n";

        const bool hasCustomFragShader = srcString.contains("void main()");

        if (!hasCustomFragShader)
            fragmentShader.addInclude("evalLightmaps.glsllib");

        // check dielectric materials
        if (!material().isDielectric())
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 1\n\n";
        else
            fragmentShader << "#define MATERIAL_IS_NON_DIELECTRIC 0\n\n";

        fragmentShader << "#define QSSG_ENABLE_RNM 0\n\n";

        fragmentShader << srcString << "\n";

        // If a "main()" is already
        // written, we'll assume that the
        // shader
        // pass is already written out and we don't need to add anything.
        // Nothing beyond the basics, anyway
        if (hasCustomFragShader) {
            fragmentShader << "#define FRAGMENT_SHADER\n\n";
            if (!hasCustomVertShader) {
                vertexShader.generateWorldNormal(inKey);
                vertexShader.generateVarTangentAndBinormal(inKey);
                vertexShader.generateWorldPosition();

                vertexShader.generateViewVector();
            }
            return true;
        }

        if (material().hasLighting() && lightmapIndirectImage) {
            generateLightmapIndirectFunc(fragmentShader, &lightmapIndirectImage->m_image);
        }
        if (material().hasLighting() && lightmapRadisoityImage) {
            generateLightmapRadiosityFunc(fragmentShader, &lightmapRadisoityImage->m_image);
        }
        if (material().hasLighting() && lightmapShadowImage) {
            generateLightmapShadowFunc(fragmentShader, &lightmapShadowImage->m_image);
        }

        if (material().hasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            generateLightmapIndirectSetupCode(fragmentShader, lightmapIndirectImage, lightmapRadisoityImage);

        if (material().hasLighting()) {
            applyEmissiveMask(fragmentShader, material().m_emissiveMap);
        }

        if (m_renderContext->renderContext()->rhiContext()->isValid()) {
            // Unlike the direct OpenGL path, with RHI "built-in" uniforms that are
            // not declared in snippets (or handled by the above branches) must
            // also go through the addUniform() mechanism.
            registerNonSnippetUnconditionalUniforms(fragmentShader);
        }

        // setup main
        vertexGenerator().beginFragmentGeneration();

        // since we do pixel lighting we always need this if lighting is enabled
        // We write this here because the functions below may also write to
        // the fragment shader
        if (material().hasLighting()) {
            vertexShader.generateWorldNormal(inKey);
            vertexShader.generateVarTangentAndBinormal(inKey);
            vertexShader.generateWorldPosition();

            if (material().isSpecularEnabled()) {
                vertexShader.generateViewVector();
            }
        }

        fragmentShader << "  initializeBaseFragmentVariables();\n"
                          "  computeTemporaries();\n"
                          "  normal = normalize( computeNormal() );\n"
                          "  initializeLayerVariables();\n"
                          "  float alpha = clamp( evalCutout(), 0.0, 1.0 );\n";

        if (material().isCutOutEnabled()) {
            fragmentShader << "  if ( alpha <= 0.0f )\n"
                              "    discard;\n";
        }

        // indirect / direct lightmap init
        if (material().hasLighting() && (lightmapIndirectImage || lightmapRadisoityImage))
            fragmentShader << "  initializeLayerVariablesWithLightmap();\n";

        // shadow map
        generateLightmapShadowCode(fragmentShader, lightmapShadowImage);

        // main Body
        fragmentShader << "#include \"customMaterialFragBodyAO.glsllib\"\n";

        // for us right now transparency means we render a glass style material
        if (m_hasTransparency && !material().isTransmissive())
            fragmentShader << " rgba = computeGlass( normal, materialIOR, alpha, rgba );\n";
        if (material().isTransmissive())
            fragmentShader << " rgba = computeOpacity( rgba );\n";

        if (vertexGenerator().hasActiveWireframe()) {
            fragmentShader.append("vec3 edgeDistance = varEdgeDistance * gl_FragCoord.w;\n"
                                  "    float d = min(min(edgeDistance.x, edgeDistance.y), edgeDistance.z);\n"
                                  "    float mixVal = smoothstep(0.0, 1.0, d);\n" // line width 1.0
                                  "    rgba = mix( vec4(0.0, 1.0, 0.0, 1.0), rgba, mixVal);");
        }
        fragmentShader << "  rgba.a *= objectOpacity;\n";
        if (m_renderContext->renderContext()->renderContextType() == QSSGRenderContextType::GLES2)
            fragmentShader << "  gl_FragColor = rgba;\n";
        else
            fragmentShader << "  fragColor = rgba;\n";
        return false;
    }

    QSSGRef<QSSGRenderShaderProgram> generateCustomMaterialShader(const QByteArray &inShaderPrefix, const QByteArray &inCustomMaterialName)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.
        QByteArray generatedShaderString;
        generatedShaderString = inShaderPrefix;
        generatedShaderString.append(inCustomMaterialName);
        QSSGShaderDefaultMaterialKey theKey(key());
        theKey.toString(generatedShaderString, m_defaultMaterialShaderKeyProperties);

        const bool hasCustomVertShader = generateVertexShader(theKey, inCustomMaterialName);
        const bool hasCustomFragShader = generateFragmentShader(theKey, inCustomMaterialName, hasCustomVertShader);

        vertexGenerator().endVertexGeneration(hasCustomVertShader);
        vertexGenerator().endFragmentGeneration(hasCustomFragShader);

        return programGenerator()->compileGeneratedShader(generatedShaderString, QSSGShaderCacheProgramFlags(), m_currentFeatureSet);
    }

    QSSGRef<QSSGRenderShaderProgram> generateShader(const QSSGRenderGraphObject &inMaterial,
                                                        QSSGShaderDefaultMaterialKey inShaderDescription,
                                                        QSSGShaderStageGeneratorInterface &inVertexPipeline,
                                                        const ShaderFeatureSetList &inFeatureSet,
                                                        const QVector<QSSGRenderLight *> &inLights,
                                                        QSSGRenderableImage *inFirstImage,
                                                        bool inHasTransparency,
                                                        const QByteArray &inShaderPrefix,
                                                        const QByteArray &inCustomMaterialName) override
    {
        Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::CustomMaterial);
        m_currentMaterial = static_cast<const QSSGRenderCustomMaterial *>(&inMaterial);
        m_currentKey = &inShaderDescription;
        m_currentPipeline = static_cast<QSSGDefaultMaterialVertexPipelineInterface *>(&inVertexPipeline);
        m_currentFeatureSet = inFeatureSet;
        m_lights = inLights;
        m_firstImage = inFirstImage;
        m_hasTransparency = inHasTransparency;

        return generateCustomMaterialShader(inShaderPrefix, inCustomMaterialName);
    }

    QSSGRef<QSSGRhiShaderStages> generateCustomMaterialRhiShader(const QByteArray &inShaderPrefix,
                                                                 const QByteArray &inCustomMaterialName)
    {
        // build a string that allows us to print out the shader we are generating to the log.
        // This is time consuming but I feel like it doesn't happen all that often and is very
        // useful to users
        // looking at the log file.
        QByteArray generatedShaderString;
        generatedShaderString = inShaderPrefix;
        generatedShaderString.append(inCustomMaterialName);
        QSSGShaderDefaultMaterialKey theKey(key());
        theKey.toString(generatedShaderString, m_defaultMaterialShaderKeyProperties);

        const bool hasCustomVertShader = generateVertexShader(theKey, inCustomMaterialName);
        const bool hasCustomFragShader = generateFragmentShader(theKey, inCustomMaterialName, hasCustomVertShader);

        vertexGenerator().endVertexGeneration(hasCustomVertShader);
        vertexGenerator().endFragmentGeneration(hasCustomFragShader);

        return programGenerator()->compileGeneratedRhiShader(generatedShaderString, QSSGShaderCacheProgramFlags(), m_currentFeatureSet);
    }

    QSSGRef<QSSGRhiShaderStages> generateRhiShaderStages(const QSSGRenderGraphObject &inMaterial,
                                                         QSSGShaderDefaultMaterialKey inShaderDescription,
                                                         QSSGShaderStageGeneratorInterface &inVertexPipeline,
                                                         const ShaderFeatureSetList &inFeatureSet,
                                                         const QVector<QSSGRenderLight *> &inLights,
                                                         QSSGRenderableImage *inFirstImage,
                                                         bool inHasTransparency,
                                                         const QByteArray &inShaderPrefix,
                                                         const QByteArray &inCustomMaterialName) override
    {
        Q_ASSERT(inMaterial.type == QSSGRenderGraphObject::Type::CustomMaterial);
        m_currentMaterial = static_cast<const QSSGRenderCustomMaterial *>(&inMaterial);
        m_currentKey = &inShaderDescription;
        m_currentPipeline = static_cast<QSSGDefaultMaterialVertexPipelineInterface *>(&inVertexPipeline);
        m_currentFeatureSet = inFeatureSet;
        m_lights = inLights;
        m_firstImage = inFirstImage;
        m_hasTransparency = inHasTransparency;

        return generateCustomMaterialRhiShader(inShaderPrefix, inCustomMaterialName);
    }
};
} // namespace

QSSGRef<QSSGMaterialShaderGeneratorInterface> QSSGMaterialShaderGeneratorInterface::createCustomMaterialShaderGenerator(QSSGRenderContextInterface *inRc)
{
    return QSSGRef<QSSGMaterialShaderGeneratorInterface>(new QSSGShaderGenerator(inRc));
}

QT_END_NAMESPACE
