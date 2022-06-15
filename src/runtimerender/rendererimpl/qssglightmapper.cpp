// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssglightmapper_p.h"
#include <QtQuick3DRuntimeRender/private/qssgrenderer_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrhiquadrenderer_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#ifdef QT_QUICK3D_HAS_LIGHTMAPPER
#include <QtCore/qfuture.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QRandomGenerator>
#include <qsimd.h>
#include <embree3/rtcore.h>
#include <tinyexr.h>
#endif

QT_BEGIN_NAMESPACE

// References:
//   https://ndotl.wordpress.com/2018/08/29/baking-artifact-free-lightmaps/
//   https://www.scratchapixel.com/lessons/3d-basic-rendering/global-illumination-path-tracing/
//   https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/gdc2018-precomputedgiobalilluminationinfrostbite.pdf
//   https://therealmjp.github.io/posts/new-blog-series-lightmap-baking-and-spherical-gaussians/
//   https://computergraphics.stackexchange.com/questions/2316/is-russian-roulette-really-the-answer
//   https://computergraphics.stackexchange.com/questions/4664/does-cosine-weighted-hemisphere-sampling-still-require-ndotl-when-calculating-co
//   https://www.rorydriscoll.com/2009/01/07/better-sampling/
//   https://github.com/TheRealMJP/BakingLab
//   https://github.com/candycat1992/LightmapperToy
//   https://github.com/godotengine/
//   https://github.com/jpcy/xatlas

#ifdef QT_QUICK3D_HAS_LIGHTMAPPER

struct QSSGLightmapperPrivate
{
    QSSGLightmapperOptions options;
    QSSGRhiContext *rhiCtx;
    QSSGRenderer *renderer;

    QVector<QSSGBakedLightingModel> bakedLightingModels;

    struct SubMeshInfo {
        quint32 offset = 0;
        quint32 count = 0;
        unsigned int geomId = RTC_INVALID_GEOMETRY_ID;
        QVector4D baseColor;
        QSSGRenderImage *baseColorNode = nullptr;
        QRhiTexture *baseColorMap = nullptr;
        QVector3D emissiveFactor;
        QSSGRenderImage *emissiveNode = nullptr;
        QRhiTexture *emissiveMap = nullptr;
        float opacity = 0.0f;
    };
    using SubMeshInfoList = QVector<SubMeshInfo>;
    QVector<SubMeshInfoList> subMeshInfos;

    struct DrawInfo {
        QSize lightmapSize;
        QByteArray vertexData;
        quint32 vertexStride;
        QByteArray indexData;
        QRhiCommandBuffer::IndexFormat indexFormat = QRhiCommandBuffer::IndexUInt32;
        quint32 positionOffset = UINT_MAX;
        QRhiVertexInputAttribute::Format positionFormat = QRhiVertexInputAttribute::Float;
        quint32 normalOffset = UINT_MAX;
        QRhiVertexInputAttribute::Format normalFormat = QRhiVertexInputAttribute::Float;
        quint32 uvOffset = UINT_MAX;
        QRhiVertexInputAttribute::Format uvFormat = QRhiVertexInputAttribute::Float;
        quint32 lightmapUVOffset = UINT_MAX;
        QRhiVertexInputAttribute::Format lightmapUVFormat = QRhiVertexInputAttribute::Float;
        QSSGMesh::Mesh meshWithLightmapUV; // only set when model->hasLightmap() == true
    };
    QVector<DrawInfo> drawInfos;

    struct Light {
        enum {
            Directional,
            Point,
            Spot
        } type;
        bool indirectOnly;
        QVector3D direction;
        QVector3D color;
        QVector3D worldPos;
        float cosConeAngle;
        float cosInnerConeAngle;
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
    };
    QVector<Light> lights;

    RTCDevice rdev = nullptr;
    RTCScene rscene = nullptr;

    struct LightmapEntry {
        QSize pixelSize;
        QVector3D worldPos;
        QVector3D normal;
        QVector4D baseColor; // static color * texture map value (both linear)
        QVector3D emission; // static factor * emission map value
        bool isValid() const { return !worldPos.isNull() && !normal.isNull(); }
        QVector3D directLight;
        QVector3D allLight;
    };
    struct Lightmap {
        Lightmap(const QSize &pixelSize) : pixelSize(pixelSize) {
            entries.resize(pixelSize.width() * pixelSize.height());
        }
        QSize pixelSize;
        QVector<LightmapEntry> entries;
        QByteArray imageFP32;
        bool hasBaseColorTransparency = false;
    };
    QVector<Lightmap> lightmaps;
    QVector<int> geomLightmapMap; // [geomId] -> index in lightmaps (NB lightmap is per-model, geomId is per-submesh)
    QVector<float> subMeshOpacityMap; // [geomId] -> opacity

    inline const LightmapEntry &texelForLightmapUV(unsigned int geomId, float u, float v) const
    {
        // find the hit texel in the lightmap for the model to which the submesh with geomId belongs
        const Lightmap &hitLightmap(lightmaps[geomLightmapMap[geomId]]);
        u = qBound(0.0f, u, 1.0f);
        // flip V, CPU-side data is top-left based
        v = 1.0f - qBound(0.0f, v, 1.0f);

        const int w = hitLightmap.pixelSize.width();
        const int h = hitLightmap.pixelSize.height();
        const int x = qBound(0, int(w * u), w - 1);
        const int y = qBound(0, int(h * v), h - 1);

        return hitLightmap.entries[x + y * w];
    }

    bool commitGeometry();
    bool prepareLightmaps();
    void computeDirectLight();
    void computeIndirectLight();
    bool postProcess();
    bool storeLightmaps();
};

static const int LM_SEAM_BLEND_ITER_COUNT = 4;

QSSGLightmapper::QSSGLightmapper(QSSGRhiContext *rhiCtx, QSSGRenderer *renderer)
    : d(new QSSGLightmapperPrivate)
{
    d->rhiCtx = rhiCtx;
    d->renderer = renderer;

#ifdef __SSE2__
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
}

QSSGLightmapper::~QSSGLightmapper()
{
    reset();
    delete d;

#ifdef __SSE2__
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);
#endif
}

void QSSGLightmapper::reset()
{
    d->bakedLightingModels.clear();
    d->subMeshInfos.clear();
    d->drawInfos.clear();
    d->lights.clear();
    d->lightmaps.clear();
    d->geomLightmapMap.clear();
    d->subMeshOpacityMap.clear();

    if (d->rscene) {
        rtcReleaseScene(d->rscene);
        d->rscene = nullptr;
    }
    if (d->rdev) {
        rtcReleaseDevice(d->rdev);
        d->rdev = nullptr;
    }
}

void QSSGLightmapper::setOptions(const QSSGLightmapperOptions &options)
{
    d->options = options;
}

qsizetype QSSGLightmapper::add(const QSSGBakedLightingModel &model)
{
    d->bakedLightingModels.append(model);
    return d->bakedLightingModels.count() - 1;
}

static void embreeErrFunc(void *, RTCError error, const char *str)
{
    qWarning("lm: Embree error: %d: %s", error, str);
}

static const unsigned int NORMAL_SLOT = 0;
static const unsigned int LIGHTMAP_UV_SLOT = 1;

static void embreeFilterFunc(const RTCFilterFunctionNArguments *args)
{
    RTCHit *hit = reinterpret_cast<RTCHit *>(args->hit);
    QSSGLightmapperPrivate *d = static_cast<QSSGLightmapperPrivate *>(args->geometryUserPtr);
    RTCGeometry geom = rtcGetGeometry(d->rscene, hit->geomID);

    // convert from barycentric and overwrite u and v in hit with the result
    rtcInterpolate0(geom, hit->primID, hit->u, hit->v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, LIGHTMAP_UV_SLOT, &hit->u, 2);

    const float opacity = d->subMeshOpacityMap[hit->geomID];
    if (opacity < 1.0f || d->lightmaps[d->geomLightmapMap[hit->geomID]].hasBaseColorTransparency) {
        const QSSGLightmapperPrivate::LightmapEntry &texel(d->texelForLightmapUV(hit->geomID, hit->u, hit->v));

        // In addition to material.opacity, take at least the base color (both
        // the static color and the value from the base color map, if there is
        // one) into account. Opacity map, alpha cutoff, etc. are ignored.
        const float alpha = opacity * texel.baseColor.w();

        // Ignore the hit if the alpha is low enough. This is not exactly perfect,
        // but better than nothing. An object with an opacity lower than the
        // threshold will act is if it was not there, as far as the intersection is
        // concerned. So then the object won't cast shadows for example.
        if (alpha < d->options.opacityThreshold)
            args->valid[0] = 0;
    }
}

bool QSSGLightmapperPrivate::commitGeometry()
{
    if (bakedLightingModels.isEmpty()) {
        qWarning("lm: No models with usedInBakedLighting, cannot bake");
        return false;
    }

    QElapsedTimer geomPrepTimer;
    geomPrepTimer.start();

    const QSSGRef<QSSGBufferManager> &bufferManager(renderer->contextInterface()->bufferManager());

    const int bakedLightingModelCount = bakedLightingModels.count();
    subMeshInfos.resize(bakedLightingModelCount);
    drawInfos.resize(bakedLightingModelCount);

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);
        if (lm.renderables.isEmpty()) {
            qWarning() << "lm: No submeshes, model" << lm.model << "cannot be lightmapped";
            return false;
        }
        if (lm.model->boneCount > 0) {
            qWarning() << "lm: Skinned models not supported" << lm.model;
            return false;
        }

        subMeshInfos[lmIdx].reserve(lm.renderables.count());
        for (const QSSGRenderableObjectHandle &handle : qAsConst(lm.renderables)) {
            Q_ASSERT(handle.obj->renderableFlags.isDefaultMaterialMeshSubset()
                     || handle.obj->renderableFlags.isCustomMaterialMeshSubset());
            QSSGSubsetRenderable *renderableObj = static_cast<QSSGSubsetRenderable *>(handle.obj);
            SubMeshInfo info;
            info.offset = renderableObj->subset.offset;
            info.count = renderableObj->subset.count;
            info.opacity = renderableObj->opacity;
            if (handle.obj->renderableFlags.isDefaultMaterialMeshSubset()) {
                const QSSGRenderDefaultMaterial *defMat = static_cast<const QSSGRenderDefaultMaterial *>(&renderableObj->material);
                info.baseColor = defMat->color;
                info.emissiveFactor = defMat->emissiveColor;
                if (defMat->colorMap) {
                    info.baseColorNode = defMat->colorMap;
                    QSSGRenderImageTexture texture = bufferManager->loadRenderImage(defMat->colorMap,
                                                                                    defMat->colorMap->m_generateMipmaps ? QSSGBufferManager::MipModeGenerated : QSSGBufferManager::MipModeNone);
                    info.baseColorMap = texture.m_texture;
                }
                if (defMat->emissiveMap) {
                    info.emissiveNode = defMat->emissiveMap;
                    QSSGRenderImageTexture texture = bufferManager->loadRenderImage(defMat->emissiveMap);
                    info.emissiveMap = texture.m_texture;
                }
            } else {
                info.baseColor = QVector4D(1.0f, 1.0f, 1.0f, 1.0f);
                info.emissiveFactor = QVector3D(0.0f, 0.0f, 0.0f);
            }
            subMeshInfos[lmIdx].append(info);
        }

        QMatrix4x4 worldTransform;
        QMatrix3x3 normalMatrix;
        QSSGSubsetRenderable *renderableObj = static_cast<QSSGSubsetRenderable *>(lm.renderables.first().obj);
        worldTransform = renderableObj->globalTransform;
        normalMatrix = renderableObj->modelContext.normalMatrix;

        DrawInfo &drawInfo(drawInfos[lmIdx]);
        QSSGMesh::Mesh mesh;

        if (lm.model->geometry)
            mesh = bufferManager->loadMeshData(lm.model->geometry);
        else
            mesh = bufferManager->loadMeshData(lm.model->meshPath);

        if (!mesh.isValid()) {
            qWarning("lm: Failed to load geometry for model");
            return false;
        }

        if (!mesh.hasLightmapUVChannel()) {
            QElapsedTimer unwrapTimer;
            unwrapTimer.start();
            if (!mesh.createLightmapUVChannel(lm.model->lightmapBaseResolution)) {
                qWarning("lm: Failed to do lightmap UV unwrapping");
                return false;
            }
            qDebug() << "lm: Lightmap UV unwrap done for model" << lm.model << "in" << unwrapTimer.elapsed() << "ms";

            if (lm.model->hasLightmap())
                drawInfo.meshWithLightmapUV = mesh;
        } else {
            qDebug() << "lm: Model" << lm.model << "already has a lightmap UV channel";
        }

        drawInfo.lightmapSize = mesh.subsets().first().lightmapSizeHint;
        if (drawInfo.lightmapSize.isEmpty()) {
            qWarning() << "lm: No lightmap size hint found for model" << lm.model << ", defaulting to 1024x1024";
            drawInfo.lightmapSize = QSize(1024, 1024);
        }

        drawInfo.vertexData = mesh.vertexBuffer().data;
        drawInfo.vertexStride = mesh.vertexBuffer().stride;
        drawInfo.indexData = mesh.indexBuffer().data;

        if (drawInfo.vertexData.isEmpty()) {
            qWarning() << "lm: No vertex data for model" << lm.model;
            return false;
        }
        if (drawInfo.indexData.isEmpty()) {
            qWarning() << "lm: No index data for model" << lm.model;
            return false;
        }

        switch (mesh.indexBuffer().componentType) {
        case QSSGMesh::Mesh::ComponentType::UnsignedInt16:
            drawInfo.indexFormat = QRhiCommandBuffer::IndexUInt16;
            break;
        case QSSGMesh::Mesh::ComponentType::UnsignedInt32:
            drawInfo.indexFormat = QRhiCommandBuffer::IndexUInt32;
            break;
        default:
            qWarning() << "lm: Unknown index component type" << int(mesh.indexBuffer().componentType)
                       << "for model" << lm.model;
            break;
        }

        for (const QSSGMesh::Mesh::VertexBufferEntry &vbe : mesh.vertexBuffer().entries) {
            if (vbe.name == QSSGMesh::MeshInternal::getPositionAttrName()) {
                drawInfo.positionOffset = vbe.offset;
                drawInfo.positionFormat = QSSGRhiInputAssemblerState::toVertexInputFormat(QSSGRenderComponentType(vbe.componentType), vbe.componentCount);
            } else if (vbe.name == QSSGMesh::MeshInternal::getNormalAttrName()) {
                drawInfo.normalOffset = vbe.offset;
                drawInfo.normalFormat = QSSGRhiInputAssemblerState::toVertexInputFormat(QSSGRenderComponentType(vbe.componentType), vbe.componentCount);
            } else if (vbe.name == QSSGMesh::MeshInternal::getUV0AttrName()) {
                drawInfo.uvOffset = vbe.offset;
                drawInfo.uvFormat = QSSGRhiInputAssemblerState::toVertexInputFormat(QSSGRenderComponentType(vbe.componentType), vbe.componentCount);
            } else if (vbe.name == QSSGMesh::MeshInternal::getLightmapUVAttrName()) {
                drawInfo.lightmapUVOffset = vbe.offset;
                drawInfo.lightmapUVFormat = QSSGRhiInputAssemblerState::toVertexInputFormat(QSSGRenderComponentType(vbe.componentType), vbe.componentCount);
            }
        }

        if (!(drawInfo.positionOffset != UINT_MAX && drawInfo.normalOffset != UINT_MAX)) {
            qWarning() << "lm: Could not figure out position and normal attribute offsets for model"
                       << lm.model;
            return false;
        }

        // We will manually access and massage the data, so cannot just work with arbitrary formats.
        if (!(drawInfo.positionFormat == QRhiVertexInputAttribute::Float3
              && drawInfo.normalFormat == QRhiVertexInputAttribute::Float3))
        {
            qWarning() << "lm: position or normal attribute format is not as expected (float3)" << lm.model;
            return false;
        }

        if (drawInfo.lightmapUVOffset == UINT_MAX) {
            qWarning() << "lm: Could not figure out lightmap UV attribute offset for model" << lm.model;
            return false;
        }

        if (drawInfo.lightmapUVFormat != QRhiVertexInputAttribute::Float2) {
            qWarning() << "lm: Lightmap UV attribute format is not as expected (float2) for model" << lm.model;
            return false;
        }

        // UV0 is optional
        if (drawInfo.uvOffset != UINT_MAX) {
            if (drawInfo.uvFormat != QRhiVertexInputAttribute::Float2) {
                qWarning() << "lm: UV0 attribute format is not as expected (float2)" << lm.model;
                return false;
            }
        }

        if (drawInfo.indexFormat == QRhiCommandBuffer::IndexUInt16) {
            drawInfo.indexFormat = QRhiCommandBuffer::IndexUInt32;
            QByteArray newIndexData(drawInfo.indexData.size() * 2, Qt::Uninitialized);
            const quint16 *s = reinterpret_cast<const quint16 *>(drawInfo.indexData.constData());
            size_t sz = drawInfo.indexData.size() / 2;
            quint32 *p = reinterpret_cast<quint32 *>(newIndexData.data());
            while (sz--)
                *p++ = *s++;
            drawInfo.indexData = newIndexData;
        }

        // Bake in the world transform.
        {
            char *vertexBase = drawInfo.vertexData.data();
            const qsizetype sz = drawInfo.vertexData.size();
            for (qsizetype offset = 0; offset < sz; offset += drawInfo.vertexStride) {
                char *posPtr = vertexBase + offset + drawInfo.positionOffset;
                float *fPosPtr = reinterpret_cast<float *>(posPtr);
                QVector3D pos(fPosPtr[0], fPosPtr[1], fPosPtr[2]);
                char *normalPtr = vertexBase + offset + drawInfo.normalOffset;
                float *fNormalPtr = reinterpret_cast<float *>(normalPtr);
                QVector3D normal(fNormalPtr[0], fNormalPtr[1], fNormalPtr[2]);
                pos = worldTransform.map(pos);
                normal = mat33::transform(normalMatrix, normal).normalized();
                *fPosPtr++ = pos.x();
                *fPosPtr++ = pos.y();
                *fPosPtr++ = pos.z();
                *fNormalPtr++ = normal.x();
                *fNormalPtr++ = normal.y();
                *fNormalPtr++ = normal.z();
            }
        }
    } // end loop over models used in the lightmap

    qDebug() << "lm: Found" << bakedLightingModelCount << "models for the lightmapped scene";

    // All subsets for a model reference the same QSSGShaderLight list,
    // take the first one, but filter it based on the bake flag.
    for (const QSSGShaderLight &sl : static_cast<QSSGSubsetRenderable *>(bakedLightingModels.first().renderables.first().obj)->lights) {
        if (!sl.enabled || !sl.light->m_bakingEnabled)
            continue;

        Light light;
        light.indirectOnly = !sl.light->m_fullyBaked;
        light.direction = sl.direction;

        const float brightness = sl.light->m_brightness;
        light.color = QVector3D(sl.light->m_diffuseColor.x() * brightness,
                                sl.light->m_diffuseColor.y() * brightness,
                                sl.light->m_diffuseColor.z() * brightness);

        if (sl.light->type == QSSGRenderLight::Type::PointLight
                || sl.light->type == QSSGRenderLight::Type::SpotLight)
        {
            light.worldPos = sl.light->getGlobalPos();
            if (sl.light->type == QSSGRenderLight::Type::SpotLight) {
                light.type = Light::Spot;
                light.cosConeAngle = qCos(qDegreesToRadians(sl.light->m_coneAngle));
                light.cosInnerConeAngle = qCos(qDegreesToRadians(
                                                   qMin(sl.light->m_innerConeAngle, sl.light->m_coneAngle)));
            } else {
                light.type = Light::Point;
            }
            light.constantAttenuation = aux::translateConstantAttenuation(sl.light->m_constantFade);
            light.linearAttenuation = aux::translateLinearAttenuation(sl.light->m_linearFade);
            light.quadraticAttenuation = aux::translateQuadraticAttenuation(sl.light->m_quadraticFade);
        } else {
            light.type = Light::Directional;
        }

        lights.append(light);
    }

    qDebug() << "lm: Found" << lights.count() << "lights enabled for baking";

    rdev = rtcNewDevice(nullptr);
    if (!rdev) {
        qWarning("Failed to create Embree device");
        return false;
    }

    rtcSetDeviceErrorFunction(rdev, embreeErrFunc, nullptr);

    rscene = rtcNewScene(rdev);

    unsigned int geomId = 1;

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);

        // While Light.castsShadow and Model.receivesShadows are irrelevant for
        // baked lighting (they are effectively ignored, shadows are always
        // there with baked direct lighting), Model.castsShadows is something
        // we can and should take into account.
        if (!lm.model->castsShadows)
            continue;

        const DrawInfo &drawInfo(drawInfos[lmIdx]);
        const char *vbase = drawInfo.vertexData.constData();
        const quint32 *ibase = reinterpret_cast<const quint32 *>(drawInfo.indexData.constData());

        for (SubMeshInfo &subMeshInfo : subMeshInfos[lmIdx]) {
            RTCGeometry geom = rtcNewGeometry(rdev, RTC_GEOMETRY_TYPE_TRIANGLE);
            rtcSetGeometryVertexAttributeCount(geom, 2);
            quint32 *ip = static_cast<quint32 *>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(uint32_t), subMeshInfo.count / 3));
            for (quint32 i = 0; i < subMeshInfo.count; ++i)
                *ip++ = i;
            float *vp = static_cast<float *>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, 3 * sizeof(float), subMeshInfo.count));
            for (quint32 i = 0; i < subMeshInfo.count; ++i) {
                const quint32 idx = *(ibase + subMeshInfo.offset + i);
                const float *src = reinterpret_cast<const float *>(vbase + idx * drawInfo.vertexStride + drawInfo.positionOffset);
                *vp++ = *src++;
                *vp++ = *src++;
                *vp++ = *src++;
            }
            vp = static_cast<float *>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, NORMAL_SLOT, RTC_FORMAT_FLOAT3, 3 * sizeof(float), subMeshInfo.count));
            for (quint32 i = 0; i < subMeshInfo.count; ++i) {
                const quint32 idx = *(ibase + subMeshInfo.offset + i);
                const float *src = reinterpret_cast<const float *>(vbase + idx * drawInfo.vertexStride + drawInfo.normalOffset);
                *vp++ = *src++;
                *vp++ = *src++;
                *vp++ = *src++;
            }
            vp = static_cast<float *>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, LIGHTMAP_UV_SLOT, RTC_FORMAT_FLOAT2, 2 * sizeof(float), subMeshInfo.count));
            for (quint32 i = 0; i < subMeshInfo.count; ++i) {
                const quint32 idx = *(ibase + subMeshInfo.offset + i);
                const float *src = reinterpret_cast<const float *>(vbase + idx * drawInfo.vertexStride + drawInfo.lightmapUVOffset);
                *vp++ = *src++;
                *vp++ = *src++;
            }
            rtcCommitGeometry(geom);
            rtcSetGeometryIntersectFilterFunction(geom, embreeFilterFunc);
            rtcSetGeometryUserData(geom, this);
            rtcAttachGeometryByID(rscene, geom, geomId);
            subMeshInfo.geomId = geomId++;
            rtcReleaseGeometry(geom);
        }
    }

    rtcCommitScene(rscene);

    RTCBounds bounds;
    rtcGetSceneBounds(rscene, &bounds);
    QVector3D lowerBound(bounds.lower_x, bounds.lower_y, bounds.lower_z);
    QVector3D upperBound(bounds.upper_x, bounds.upper_y, bounds.upper_z);
    qDebug() << "lm: Bounds in world space for raytracing scene:" << lowerBound << upperBound;

    const unsigned int geomIdBasedMapSize = geomId;
    // Need fast lookup, hence indexing by geomId here. geomId starts from 1,
    // meaning index 0 will be unused, but that's ok.
    geomLightmapMap.fill(-1, geomIdBasedMapSize);
    subMeshOpacityMap.fill(0.0f, geomIdBasedMapSize);

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);
        if (!lm.model->castsShadows) // only matters if it's in the raytracer scene
            continue;
        for (SubMeshInfo &subMeshInfo : subMeshInfos[lmIdx])
            subMeshOpacityMap[subMeshInfo.geomId] = subMeshInfo.opacity;
    }

    qDebug() << "lm: Lightmapper geometry setup took" << geomPrepTimer.elapsed() << "ms";
    return true;
}

bool QSSGLightmapperPrivate::prepareLightmaps()
{
    QRhi *rhi = rhiCtx->rhi();
    if (!rhi->isTextureFormatSupported(QRhiTexture::RGBA32F)) {
        qWarning("lm: FP32 textures not supported, cannot bake");
        return false;
    }
    if (rhi->resourceLimit(QRhi::MaxColorAttachments) < 4) {
        qWarning("lm: Multiple render targets not supported, cannot bake");
        return false;
    }
    if (!rhi->isFeatureSupported(QRhi::NonFillPolygonMode)) {
        qWarning("lm: Line polygon mode not supported, cannot bake");
        return false;
    }

    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    const int bakedLightingModelCount = bakedLightingModels.count();
    Q_ASSERT(drawInfos.count() == bakedLightingModelCount);
    Q_ASSERT(subMeshInfos.count() == bakedLightingModelCount);

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        QElapsedTimer rasterizeTimer;
        rasterizeTimer.start();

        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);

        const DrawInfo &bakeModelDrawInfo(drawInfos[lmIdx]);
        const bool hasUV0 = bakeModelDrawInfo.uvOffset != UINT_MAX;
        const QSize outputSize = bakeModelDrawInfo.lightmapSize;

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ QRhiVertexInputBinding(bakeModelDrawInfo.vertexStride) });

        std::unique_ptr<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, bakeModelDrawInfo.vertexData.size()));
        if (!vbuf->create()) {
            qWarning("lm: Failed to create vertex buffer");
            return false;
        }
        std::unique_ptr<QRhiBuffer> ibuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, bakeModelDrawInfo.indexData.size()));
        if (!ibuf->create()) {
            qWarning("lm: Failed to create index buffer");
            return false;
        }
        QRhiResourceUpdateBatch *resUpd = rhi->nextResourceUpdateBatch();
        resUpd->uploadStaticBuffer(vbuf.get(), bakeModelDrawInfo.vertexData.constData());
        resUpd->uploadStaticBuffer(ibuf.get(), bakeModelDrawInfo.indexData.constData());
        cb->resourceUpdate(resUpd);

        std::unique_ptr<QRhiTexture> positionData(rhi->newTexture(QRhiTexture::RGBA32F, outputSize, 1,
                                                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        if (!positionData->create()) {
            qWarning("lm: Failed to create FP32 texture for positions");
            return false;
        }
        std::unique_ptr<QRhiTexture> normalData(rhi->newTexture(QRhiTexture::RGBA32F, outputSize, 1,
                                                                QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        if (!normalData->create()) {
            qWarning("lm: Failed to create FP32 texture for normals");
            return false;
        }
        std::unique_ptr<QRhiTexture> baseColorData(rhi->newTexture(QRhiTexture::RGBA32F, outputSize, 1,
                                                                   QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        if (!baseColorData->create()) {
            qWarning("lm: Failed to create FP32 texture for base color");
            return false;
        }
        std::unique_ptr<QRhiTexture> emissionData(rhi->newTexture(QRhiTexture::RGBA32F, outputSize, 1,
                                                                  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        if (!emissionData->create()) {
            qWarning("lm: Failed to create FP32 texture for emissive color");
            return false;
        }

        std::unique_ptr<QRhiRenderBuffer> ds(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, outputSize));
        if (!ds->create()) {
            qWarning("lm: Failed to create depth-stencil buffer");
            return false;
        }

        QRhiColorAttachment posAtt(positionData.get());
        QRhiColorAttachment normalAtt(normalData.get());
        QRhiColorAttachment baseColorAtt(baseColorData.get());
        QRhiColorAttachment emissionAtt(emissionData.get());
        QRhiTextureRenderTargetDescription rtDesc;
        rtDesc.setColorAttachments({ posAtt, normalAtt, baseColorAtt, emissionAtt });
        rtDesc.setDepthStencilBuffer(ds.get());

        std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
        std::unique_ptr<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.get());
        if (!rt->create()) {
            qWarning("lm: Failed to create texture render target");
            return false;
        }

        static const int UBUF_SIZE = 32;
        const int subMeshCount = subMeshInfos[lmIdx].count();
        const int alignedUbufSize = rhi->ubufAligned(UBUF_SIZE);
        const int totalUbufSize = alignedUbufSize * subMeshCount;
        std::unique_ptr<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, totalUbufSize));
        if (!ubuf->create()) {
            qWarning("lm: Failed to create uniform buffer of size %d", totalUbufSize);
            return false;
        }

        // Must ensure that the final image is identical with all graphics APIs,
        // regardless of how the Y axis goes in the image and normalized device
        // coordinate systems.
        qint32 flipY = rhi->isYUpInFramebuffer() ? 0 : 1;
        if (rhi->isYUpInNDC())
            flipY = 1 - flipY;

        char *ubufData = ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
        for (int subMeshIdx = 0; subMeshIdx != subMeshCount; ++subMeshIdx) {
            const SubMeshInfo &subMeshInfo(subMeshInfos[lmIdx][subMeshIdx]);
            char *p = ubufData + subMeshIdx * alignedUbufSize;
            memcpy(p, &subMeshInfo.baseColor, 4 * sizeof(float));
            memcpy(p + 16, &subMeshInfo.emissiveFactor, 3 * sizeof(float));
            memcpy(p + 28, &flipY, sizeof(qint32));
        }
        ubuf->endFullDynamicBufferUpdateForCurrentFrame();

        auto setupPipeline = [rhi, &rpDesc](QSSGRhiShaderPipeline *shaderPipeline,
                QRhiShaderResourceBindings *srb,
                const QRhiVertexInputLayout &inputLayout)
        {
            QRhiGraphicsPipeline *ps = rhi->newGraphicsPipeline();
            ps->setTopology(QRhiGraphicsPipeline::Triangles);
            ps->setDepthTest(true);
            ps->setDepthWrite(true);
            ps->setDepthOp(QRhiGraphicsPipeline::Less);
            ps->setShaderStages(shaderPipeline->cbeginStages(), shaderPipeline->cendStages());
            ps->setTargetBlends({ {}, {}, {}, {} });
            ps->setRenderPassDescriptor(rpDesc.get());
            ps->setVertexInputLayout(inputLayout);
            ps->setShaderResourceBindings(srb);
            return ps;
        };

        QVector<QRhiGraphicsPipeline *> ps;
        // Everything is going to be rendered twice (but note depth testing), first
        // with polygon mode fill, then line.
        QVector<QRhiGraphicsPipeline *> psLine;

        for (int subMeshIdx = 0; subMeshIdx != subMeshCount; ++subMeshIdx) {
            const SubMeshInfo &subMeshInfo(subMeshInfos[lmIdx][subMeshIdx]);
            QVarLengthArray<QRhiVertexInputAttribute, 4> vertexAttrs;
            vertexAttrs << QRhiVertexInputAttribute(0, 0, bakeModelDrawInfo.positionFormat, bakeModelDrawInfo.positionOffset)
                        << QRhiVertexInputAttribute(0, 1, bakeModelDrawInfo.normalFormat, bakeModelDrawInfo.normalOffset)
                        << QRhiVertexInputAttribute(0, 2, bakeModelDrawInfo.lightmapUVFormat, bakeModelDrawInfo.lightmapUVOffset);

            bool hasBaseColorMap = subMeshInfo.baseColorMap != nullptr;
            bool hasEmissiveMap = subMeshInfo.emissiveMap != nullptr;
            QSSGRenderer::LightmapUVRasterizationShaderMode shaderVariant = QSSGRenderer::LightmapUVRasterizationShaderMode::Default;
            if (hasBaseColorMap && hasEmissiveMap)
                shaderVariant = QSSGRenderer::LightmapUVRasterizationShaderMode::BaseColorAndEmissiveMaps;
            else if (hasEmissiveMap)
                shaderVariant = QSSGRenderer::LightmapUVRasterizationShaderMode::EmissiveMap;
            else if (hasBaseColorMap)
                shaderVariant = QSSGRenderer::LightmapUVRasterizationShaderMode::BaseColorMap;

            QSSGRef<QSSGRhiShaderPipeline> lmUvRastShaderPipeline = renderer->getRhiLightmapUVRasterizationShader(shaderVariant);
            if (!lmUvRastShaderPipeline) {
                qWarning("lm: Failed to load shaders");
                return false;
            }

            // Vertex inputs (just like the sampler uniforms) must match exactly on
            // the shader and the application side, cannot just leave out or have
            // unused inputs.
            if (hasUV0 && (hasBaseColorMap || hasEmissiveMap))
                vertexAttrs << QRhiVertexInputAttribute(0, 3, bakeModelDrawInfo.uvFormat, bakeModelDrawInfo.uvOffset);

            inputLayout.setAttributes(vertexAttrs.cbegin(), vertexAttrs.cend());

            QSSGRhiShaderResourceBindingList bindings;
            bindings.addUniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf.get(),
                                      subMeshIdx * alignedUbufSize, UBUF_SIZE);
            if (hasBaseColorMap) {
                const bool mipmapped = subMeshInfo.baseColorMap->flags().testFlag(QRhiTexture::MipMapped);
                QRhiSampler *sampler = rhiCtx->sampler({ toRhi(subMeshInfo.baseColorNode->m_minFilterType),
                                                         toRhi(subMeshInfo.baseColorNode->m_magFilterType),
                                                         mipmapped ? toRhi(subMeshInfo.baseColorNode->m_mipFilterType) : QRhiSampler::None,
                                                         toRhi(subMeshInfo.baseColorNode->m_horizontalTilingMode),
                                                         toRhi(subMeshInfo.baseColorNode->m_verticalTilingMode),
                                                         QRhiSampler::Repeat
                                                       });
                bindings.addTexture(1, QRhiShaderResourceBinding::FragmentStage, subMeshInfo.baseColorMap, sampler);
            }
            if (hasEmissiveMap) {
                const bool mipmapped = subMeshInfo.emissiveMap->flags().testFlag(QRhiTexture::MipMapped);
                QRhiSampler *sampler = rhiCtx->sampler({ toRhi(subMeshInfo.emissiveNode->m_minFilterType),
                                                         toRhi(subMeshInfo.emissiveNode->m_magFilterType),
                                                         mipmapped ? toRhi(subMeshInfo.emissiveNode->m_mipFilterType) : QRhiSampler::None,
                                                         toRhi(subMeshInfo.emissiveNode->m_horizontalTilingMode),
                                                         toRhi(subMeshInfo.emissiveNode->m_verticalTilingMode),
                                                         QRhiSampler::Repeat
                                                       });
                bindings.addTexture(2, QRhiShaderResourceBinding::FragmentStage, subMeshInfo.emissiveMap, sampler);
            }
            QRhiShaderResourceBindings *srb = rhiCtx->srb(bindings);

            QRhiGraphicsPipeline *pipeline = setupPipeline(lmUvRastShaderPipeline.data(), srb, inputLayout);
            if (!pipeline->create()) {
                qWarning("lm: Failed to create graphics pipeline (mesh %d submesh %d)",
                         lmIdx, subMeshIdx);
                qDeleteAll(ps);
                qDeleteAll(psLine);
                return false;
            }
            ps.append(pipeline);
            pipeline = setupPipeline(lmUvRastShaderPipeline.data(), srb, inputLayout);
            pipeline->setPolygonMode(QRhiGraphicsPipeline::Line);
            if (!pipeline->create()) {
                qWarning("lm: Failed to create graphics pipeline with line fill mode (mesh %d submesh %d)",
                         lmIdx, subMeshIdx);
                qDeleteAll(ps);
                qDeleteAll(psLine);
                return false;
            }
            psLine.append(pipeline);
        }

        QRhiCommandBuffer::VertexInput vertexBuffers = { vbuf.get(), 0 };
        const QRhiViewport viewport(0, 0, float(outputSize.width()), float(outputSize.height()));
        bool hadViewport = false;

        cb->beginPass(rt.get(), Qt::black, { 1.0f, 0 });
        for (int subMeshIdx = 0; subMeshIdx != subMeshCount; ++subMeshIdx) {
            const SubMeshInfo &subMeshInfo(subMeshInfos[lmIdx][subMeshIdx]);
            cb->setGraphicsPipeline(ps[subMeshIdx]);
            if (!hadViewport) {
                cb->setViewport(viewport);
                hadViewport = true;
            }
            cb->setShaderResources();
            cb->setVertexInput(0, 1, &vertexBuffers, ibuf.get(), 0, QRhiCommandBuffer::IndexUInt32);
            cb->drawIndexed(subMeshInfo.count, 1, subMeshInfo.offset);
            cb->setGraphicsPipeline(psLine[subMeshIdx]);
            cb->setShaderResources();
            cb->drawIndexed(subMeshInfo.count, 1, subMeshInfo.offset);
        }

        resUpd = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult posReadResult;
        QRhiReadbackResult normalReadResult;
        QRhiReadbackResult baseColorReadResult;
        QRhiReadbackResult emissionReadResult;
        resUpd->readBackTexture({ positionData.get() }, &posReadResult);
        resUpd->readBackTexture({ normalData.get() }, &normalReadResult);
        resUpd->readBackTexture({ baseColorData.get() }, &baseColorReadResult);
        resUpd->readBackTexture({ emissionData.get() }, &emissionReadResult);
        cb->endPass(resUpd);

        // Submit and wait for completion.
        rhi->finish();

        qDeleteAll(ps);
        qDeleteAll(psLine);

        Lightmap lightmap(outputSize);

        // The readback results are tightly packed (which is supposed to be ensured
        // by each rhi backend), so one line is 16 * width bytes.
        if (posReadResult.data.size() < lightmap.entries.count() * 16) {
            qWarning("lm: Position data is smaller than expected");
            return false;
        }
        if (normalReadResult.data.size() < lightmap.entries.count() * 16) {
            qWarning("lm: Normal data is smaller than expected");
            return false;
        }
        if (baseColorReadResult.data.size() < lightmap.entries.count() * 16) {
            qWarning("lm: Base color data is smaller than expected");
            return false;
        }
        if (emissionReadResult.data.size() < lightmap.entries.count() * 16) {
            qWarning("lm: Emission data is smaller than expected");
            return false;
        }
        const float *lmPosPtr = reinterpret_cast<const float *>(posReadResult.data.constData());
        const float *lmNormPtr = reinterpret_cast<const float *>(normalReadResult.data.constData());
        const float *lmBaseColorPtr = reinterpret_cast<const float *>(baseColorReadResult.data.constData());
        const float *lmEmissionPtr = reinterpret_cast<const float *>(emissionReadResult.data.constData());
        int unusedEntries = 0;
        for (qsizetype i = 0, ie = lightmap.entries.count(); i != ie; ++i) {
            LightmapEntry &lmPix(lightmap.entries[i]);

            float x = *lmPosPtr++;
            float y = *lmPosPtr++;
            float z = *lmPosPtr++;
            lmPosPtr++;
            lmPix.worldPos = QVector3D(x, y, z);

            x = *lmNormPtr++;
            y = *lmNormPtr++;
            z = *lmNormPtr++;
            lmNormPtr++;
            lmPix.normal = QVector3D(x, y, z);

            float r = *lmBaseColorPtr++;
            float g = *lmBaseColorPtr++;
            float b = *lmBaseColorPtr++;
            float a = *lmBaseColorPtr++;
            lmPix.baseColor = QVector4D(r, g, b, a);
            if (a < 1.0f)
                lightmap.hasBaseColorTransparency = true;

            r = *lmEmissionPtr++;
            g = *lmEmissionPtr++;
            b = *lmEmissionPtr++;
            lmEmissionPtr++;
            lmPix.emission = QVector3D(r, g, b);

            if (!lmPix.isValid())
                ++unusedEntries;
        }

        qDebug() << "lm: Rasterized" << (lightmap.entries.count() - unusedEntries) << "lightmap texels (total"
                 << lightmap.entries.count() << "unused" << unusedEntries << "semi-trans.basecolor" << lightmap.hasBaseColorTransparency
                 << ") for model" << lm.model
                 << "with lightmap size" << outputSize << "in" << rasterizeTimer.elapsed() << "ms";

        lightmaps.append(lightmap);

        for (const SubMeshInfo &subMeshInfo : qAsConst(subMeshInfos[lmIdx]))
            geomLightmapMap[subMeshInfo.geomId] = lightmaps.count() - 1;
    }

    return true;
}

struct RayHit
{
    RayHit(const QVector3D &org, const QVector3D &dir, float tnear = 0.0f, float tfar = std::numeric_limits<float>::infinity()) {
        rayhit.ray.org_x = org.x();
        rayhit.ray.org_y = org.y();
        rayhit.ray.org_z = org.z();
        rayhit.ray.dir_x = dir.x();
        rayhit.ray.dir_y = dir.y();
        rayhit.ray.dir_z = dir.z();
        rayhit.ray.tnear = tnear;
        rayhit.ray.tfar = tfar;
        rayhit.hit.u = 0.0f;
        rayhit.hit.v = 0.0f;
        rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }

    RTCRayHit rayhit;

    bool intersect(RTCScene scene)
    {
        RTCIntersectContext ctx;
        rtcInitIntersectContext(&ctx);
        rtcIntersect1(scene, &ctx, &rayhit);
        return rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID;
    }
};

static inline QVector3D vectorSign(const QVector3D &v)
{
    return QVector3D(v.x() < 1.0f ? -1.0f : 1.0f,
                     v.y() < 1.0f ? -1.0f : 1.0f,
                     v.z() < 1.0f ? -1.0f : 1.0f);
}

static inline QVector3D vectorAbs(const QVector3D &v)
{
    return QVector3D(std::abs(v.x()),
                     std::abs(v.y()),
                     std::abs(v.z()));
}

void QSSGLightmapperPrivate::computeDirectLight()
{
    QElapsedTimer fullDirectLightTimer;
    fullDirectLightTimer.start();

    const int bakedLightingModelCount = bakedLightingModels.count();
    Q_ASSERT(lightmaps.count() == bakedLightingModelCount);

    QVector<QFuture<void>> futures;

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);
        Lightmap &lightmap(lightmaps[lmIdx]);

        // direct lighting is relatively fast to calculate, so parallelize per model
        futures << QtConcurrent::run([this, &lm, &lightmap] {
            QElapsedTimer directLightTimer;
            directLightTimer.start();

            const int lightCount = lights.count();
            for (LightmapEntry &lmPix : lightmap.entries) {
                if (!lmPix.isValid())
                    continue;

                QVector3D worldPos = lmPix.worldPos;
                if (options.useAdaptiveBias)
                    worldPos += vectorSign(lmPix.normal) * vectorAbs(worldPos * 0.0000002f);

                // 'lights' should have all lights that are either BakeModeIndirect or BakeModeAll
                for (int i = 0; i < lightCount; ++i) {
                    const Light &light(lights[i]);

                    QVector3D lightWorldPos;
                    float dist = std::numeric_limits<float>::infinity();
                    float attenuation = 1.0f;
                    if (light.type == Light::Directional) {
                        lightWorldPos = worldPos - light.direction;
                    } else {
                        lightWorldPos = light.worldPos;
                        dist = (worldPos - lightWorldPos).length();
                        attenuation = 1.0f / (light.constantAttenuation
                                              + light.linearAttenuation * dist
                                              + light.quadraticAttenuation * dist * dist);
                        if (light.type == Light::Spot) {
                            const float spotAngle = QVector3D::dotProduct((worldPos - lightWorldPos).normalized(),
                                                                          light.direction.normalized());
                            if (spotAngle > light.cosConeAngle) {
                                // spotFactor = smoothstep(light.cosConeAngle, light.cosInnerConeAngle, spotAngle);
                                const float edge0 = light.cosConeAngle;
                                const float edge1 = light.cosInnerConeAngle;
                                const float x = spotAngle;
                                const float t = qBound(0.0f, (x - edge0) / (edge1 - edge0), 1.0f);
                                const float spotFactor = t * t * (3.0f - 2.0f * t);
                                attenuation *= spotFactor;
                            } else {
                                attenuation = 0.0f;
                            }
                        }
                    }

                    const QVector3D N = lmPix.normal;
                    const QVector3D L = (lightWorldPos - worldPos).normalized();
                    const float energy = qMax(0.0f, QVector3D::dotProduct(N, L)) * attenuation;
                    if (qFuzzyIsNull(energy))
                        continue;

                    // trace a ray from this point towards the light, and see if something is hit on the way
                    RayHit ray(worldPos, L, options.bias, dist);
                    const bool lightReachable = !ray.intersect(rscene);
                    if (lightReachable) {
                        // direct light must always be stored because indirect computation will need it
                        lmPix.directLight += light.color * energy;
                        // but we take it into account in the final result only for lights that have BakeModeAll
                        if (!light.indirectOnly)
                            lmPix.allLight += light.color * energy;
                    }
                }
            }

            qDebug() << "lm: Direct light computed for model" << lm.model << "in" << directLightTimer.elapsed() << "ms";
        });
    }

    for (QFuture<void> &future : futures)
        future.waitForFinished();

    qDebug() << "lm: Total time for parallel direct light computation was" << fullDirectLightTimer.elapsed() << "ms";
}

// xorshift rng. this is called a lot -> rand/QRandomGenerator is out of question (way too slow)
static inline float uniformRand()
{
    static thread_local quint32 state = QRandomGenerator::global()->generate();
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return float(state) / float(UINT32_MAX);
}

static inline QVector3D cosWeightedHemisphereSample()
{
    const float r1 = uniformRand();
    const float r2 = uniformRand() * 2.0f * float(M_PI);
    const float sqr1 = std::sqrt(r1);
    const float sqr1m = std::sqrt(1.0f - r1);
    return QVector3D(sqr1 * std::cos(r2), sqr1 * std::sin(r2), sqr1m);
}

void QSSGLightmapperPrivate::computeIndirectLight()
{
    QElapsedTimer fullIndirectLightTimer;
    fullIndirectLightTimer.start();

    const int bakedLightingModelCount = bakedLightingModels.count();

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        // here we only care about the models that will store the lightmap image persistently
        if (!bakedLightingModels[lmIdx].model->hasLightmap())
            continue;

        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);
        Lightmap &lightmap(lightmaps[lmIdx]);
        int texelsDone = 0;

        QElapsedTimer indirectLightTimer;
        indirectLightTimer.start();

        // indirect lighting is slow, so parallelize per groups of samples,
        // e.g. if sample count is 256 and workgroup size is 32, then do up to
        // 8 sets in parallel, each calculating 32 samples (how many of the 8
        // are really done concurrently that's up to the thread pool to manage)

        int wgSizePerGroup = qMax(1, options.indirectLightWorkgroupSize);
        int wgCount = options.indirectLightSamples / wgSizePerGroup;
        if (options.indirectLightSamples % wgSizePerGroup)
            ++wgCount;

        QVector<QFuture<QVector3D>> wg(wgCount);

        qDebug() << "lm: Computing indirect lighting for model" << lm.model << "with key" << lm.model->lightmapKey;
        qDebug() << "lm: Sample count is" << options.indirectLightSamples << "Workgroup size is" << wgSizePerGroup
                 << "Max bounces is" << options.indirectLightBounces << "Multiplier is" << options.indirectLightFactor;

        for (LightmapEntry &lmPix : lightmap.entries) {
            if (!lmPix.isValid())
                continue;

            for (int wgIdx = 0; wgIdx < wgCount; ++wgIdx) {
                const int beginIdx = wgIdx * wgSizePerGroup;
                const int endIdx = qMin(beginIdx + wgSizePerGroup, options.indirectLightSamples);

                wg[wgIdx] = QtConcurrent::run([this, beginIdx, endIdx, &lmPix] {
                    QVector3D wgResult;
                    for (int sampleIdx = beginIdx; sampleIdx < endIdx; ++sampleIdx) {
                        QVector3D position = lmPix.worldPos;
                        QVector3D normal = lmPix.normal;
                        QVector3D throughput(1.0f, 1.0f, 1.0f);
                        QVector3D sampleResult;

                        for (int bounce = 0; bounce < options.indirectLightBounces; ++bounce) {
                            if (options.useAdaptiveBias)
                                position += vectorSign(normal) * vectorAbs(position * 0.0000002f);

                            // get a sample using a cosine-weighted hemisphere sampler
                            const QVector3D sample = cosWeightedHemisphereSample();

                            // transform to the point's local coordinate system
                            const QVector3D v0 = qFuzzyCompare(qAbs(normal.z()), 1.0f)
                                    ? QVector3D(0.0f, 1.0f, 0.0f)
                                    : QVector3D(0.0f, 0.0f, 1.0f);
                            const QVector3D tangent = QVector3D::crossProduct(v0, normal).normalized();
                            const QVector3D bitangent = QVector3D::crossProduct(tangent, normal).normalized();
                            QVector3D direction(
                                        tangent.x() * sample.x() + bitangent.x() * sample.y() + normal.x() * sample.z(),
                                        tangent.y() * sample.x() + bitangent.y() * sample.y() + normal.y() * sample.z(),
                                        tangent.z() * sample.x() + bitangent.z() * sample.y() + normal.z() * sample.z());
                            direction.normalize();

                            // probability distribution function
                            const float NdotL = qMax(0.0f, QVector3D::dotProduct(normal, direction));
                            const float pdf = NdotL / float(M_PI);
                            if (qFuzzyIsNull(pdf))
                                break;

                            // shoot ray, stop if no hit
                            RayHit ray(position, direction, options.bias);
                            if (!ray.intersect(rscene))
                                break;

                            // see what (sub)mesh and which texel it intersected with
                            const LightmapEntry &hitEntry = texelForLightmapUV(ray.rayhit.hit.geomID,
                                                                               ray.rayhit.hit.u,
                                                                               ray.rayhit.hit.v);

                            // won't bounce further from a back face
                            const bool hitBackFace = QVector3D::dotProduct(hitEntry.normal, direction) > 0.0f;
                            if (hitBackFace)
                                break;

                            // the BRDF of a diffuse surface is albedo / PI
                            const QVector3D brdf = hitEntry.baseColor.toVector3D() / float(M_PI);

                            // calculate result for this bounce
                            sampleResult += throughput * hitEntry.emission;
                            throughput *= brdf * NdotL / pdf;
                            sampleResult += throughput * hitEntry.directLight;

                            // stop if we guess there's no point in bouncing further
                            // (low throughput path wouldn't contribute much)
                            const float p = qMax(qMax(throughput.x(), throughput.y()), throughput.z());
                            if (p < uniformRand())
                                break;

                            // was not terminated: boost the energy by the probability to be terminated
                            throughput /= p;

                            // next bounce starts from the hit's position
                            position = hitEntry.worldPos;
                            normal = hitEntry.normal;
                        }

                        wgResult += sampleResult;
                    }
                    return wgResult;
                });
            }

            QVector3D totalIndirect;
            for (const auto &future : wg)
                totalIndirect += future.result();

            lmPix.allLight += totalIndirect * options.indirectLightFactor / options.indirectLightSamples;

            ++texelsDone;
            if (texelsDone % 10000 == 0)
                qDebug() << "lm:" << (lightmap.entries.count() - texelsDone) << "texels left";
        }

        qDebug() << "lm: Indirect light computed for model" << lm.model
                 << "with key" << lm.model->lightmapKey << "in" << indirectLightTimer.elapsed() << "ms";
    }

    qDebug() << "lm: Total time for parallel indirect light computation was" << fullIndirectLightTimer.elapsed() << "ms";
}

struct Edge {
    std::array<QVector3D, 2> pos;
    std::array<QVector3D, 2> normal;
};

inline bool operator==(const Edge &a, const Edge &b)
{
    return qFuzzyCompare(a.pos[0], b.pos[0])
            && qFuzzyCompare(a.pos[1], b.pos[1])
            && qFuzzyCompare(a.normal[0], b.normal[0])
            && qFuzzyCompare(a.normal[1], b.normal[1]);
}

inline size_t qHash(const Edge &e, size_t seed) Q_DECL_NOTHROW
{
    return qHash(e.pos[0].x(), seed) ^ qHash(e.pos[0].y()) ^ qHash(e.pos[0].z())
            ^ qHash(e.pos[1].x()) ^ qHash(e.pos[1].y()) ^ qHash(e.pos[1].z());
}

struct EdgeUV {
    std::array<QVector2D, 2> uv;
    bool seam = false;
};

struct SeamUV {
    std::array<std::array<QVector2D, 2>, 2> uv;
};

static inline bool vectorLessThan(const QVector3D &a, const QVector3D &b)
{
    if (a.x() == b.x()) {
        if (a.y() == b.y())
            return a.z() < b.z();
        else
            return a.y() < b.y();
    }
    return a.x() < b.x();
}

static inline float floatSign(float f)
{
    return f > 0.0f ? 1.0f : (f < 0.0f ? -1.0f : 0.0f);
}

static inline QVector2D flooredVec(const QVector2D &v)
{
    return QVector2D(std::floor(v.x()), std::floor(v.y()));
}

static inline QVector2D projectPointToLine(const QVector2D &point, const std::array<QVector2D, 2> &line)
{
    const QVector2D p = point - line[0];
    const QVector2D n = line[1] - line[0];
    const float lengthSquared = n.lengthSquared();
    if (!qFuzzyIsNull(lengthSquared)) {
        const float d = (n.x() * p.x() + n.y() * p.y()) / lengthSquared;
        return d <= 0.0f ? line[0] : (d >= 1.0f ? line[1] : line[0] + n * d);
    }
    return line[0];
}

static void blendLine(const QVector2D &from, const QVector2D &to,
                      const QVector2D &uvFrom, const QVector2D &uvTo,
                      const QByteArray &readBuf, QByteArray &writeBuf,
                      const QSize &lightmapPixelSize)
{
    const QVector2D size(lightmapPixelSize.width(), lightmapPixelSize.height());
    const std::array<QVector2D, 2> line = { QVector2D(from.x(), 1.0f - from.y()) * size,
                                            QVector2D(to.x(), 1.0f - to.y()) * size };
    const float lineLength = line[0].distanceToPoint(line[1]);
    if (qFuzzyIsNull(lineLength))
        return;

    const QVector2D startPixel = flooredVec(line[0]);
    const QVector2D endPixel = flooredVec(line[1]);

    const QVector2D dir = (line[1] - line[0]).normalized();
    const QVector2D tStep(1.0f / std::abs(dir.x()), 1.0f / std::abs(dir.y()));
    const QVector2D pixelStep(floatSign(dir.x()), floatSign(dir.y()));

    QVector2D nextT(std::fmod(line[0].x(), 1.0f), std::fmod(line[0].y(), 1.0f));
    if (pixelStep.x() == 1.0f)
        nextT.setX(1.0f - nextT.x());
    if (pixelStep.y() == 1.0f)
        nextT.setY(1.0f - nextT.y());
    nextT /= QVector2D(std::abs(dir.x()), std::abs(dir.y()));
    if (std::isnan(nextT.x()))
        nextT.setX(std::numeric_limits<float>::max());
    if (std::isnan(nextT.y()))
        nextT.setY(std::numeric_limits<float>::max());

    float *fpW = reinterpret_cast<float *>(writeBuf.data());
    const float *fpR = reinterpret_cast<const float *>(readBuf.constData());

    QVector2D pixel = startPixel;

    while (startPixel.distanceToPoint(pixel) < lineLength + 1.0f) {
        const QVector2D point = projectPointToLine(pixel + QVector2D(0.5f, 0.5f), line);
        const float t = line[0].distanceToPoint(point) / lineLength;
        const QVector2D uvInterp = uvFrom * (1.0 - t) + uvTo * t;
        const QVector2D sampledPixel = flooredVec(QVector2D(uvInterp.x(), 1.0f - uvInterp.y()) * size);

        const int sampOfs = (int(sampledPixel.x()) + int(sampledPixel.y()) * lightmapPixelSize.width()) * 4;
        const QVector3D sampledColor(fpR[sampOfs], fpR[sampOfs + 1], fpR[sampOfs + 2]);
        const int pixOfs = (int(pixel.x()) + int(pixel.y()) * lightmapPixelSize.width()) * 4;
        QVector3D currentColor(fpW[pixOfs], fpW[pixOfs + 1], fpW[pixOfs + 2]);
        currentColor = currentColor * 0.6f + sampledColor * 0.4f;
        fpW[pixOfs] = currentColor.x();
        fpW[pixOfs + 1] = currentColor.y();
        fpW[pixOfs + 2] = currentColor.z();

        if (pixel != endPixel) {
            if (nextT.x() < nextT.y()) {
                pixel.setX(pixel.x() + pixelStep.x());
                nextT.setX(nextT.x() + tStep.x());
            } else {
                pixel.setY(pixel.y() + pixelStep.y());
                nextT.setY(nextT.y() + tStep.y());
            }
        } else {
            break;
        }
    }
}

bool QSSGLightmapperPrivate::postProcess()
{
    QRhi *rhi = rhiCtx->rhi();
    QRhiCommandBuffer *cb = rhiCtx->commandBuffer();
    const int bakedLightingModelCount = bakedLightingModels.count();

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        QElapsedTimer postProcessTimer;
        postProcessTimer.start();

        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);
        // only care about the ones that will store the lightmap image persistently
        if (!lm.model->hasLightmap())
            continue;

        Lightmap &lightmap(lightmaps[lmIdx]);

        // Assemble the RGBA32F image from the baker data structures
        QByteArray lightmapFP32(lightmap.entries.count() * 4 * sizeof(float), Qt::Uninitialized);
        float *lightmapFloatPtr = reinterpret_cast<float *>(lightmapFP32.data());
        for (const LightmapEntry &lmPix : qAsConst(lightmap.entries)) {
            *lightmapFloatPtr++ = lmPix.allLight.x();
            *lightmapFloatPtr++ = lmPix.allLight.y();
            *lightmapFloatPtr++ = lmPix.allLight.z();
            *lightmapFloatPtr++ = lmPix.isValid() ? 1.0f : 0.0f;
        }

        // Dilate
        const QRhiViewport viewport(0, 0, float(lightmap.pixelSize.width()), float(lightmap.pixelSize.height()));

        std::unique_ptr<QRhiTexture> lightmapTex(rhi->newTexture(QRhiTexture::RGBA32F, lightmap.pixelSize));
        if (!lightmapTex->create()) {
            qWarning("lm: Failed to create FP32 texture for postprocessing");
            return false;
        }
        std::unique_ptr<QRhiTexture> dilatedLightmapTex(rhi->newTexture(QRhiTexture::RGBA32F, lightmap.pixelSize, 1,
                                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        if (!dilatedLightmapTex->create()) {
            qWarning("lm: Failed to create FP32 dest. texture for postprocessing");
            return false;
        }
        QRhiTextureRenderTargetDescription rtDescDilate(dilatedLightmapTex.get());
        std::unique_ptr<QRhiTextureRenderTarget> rtDilate(rhi->newTextureRenderTarget(rtDescDilate));
        std::unique_ptr<QRhiRenderPassDescriptor> rpDescDilate(rtDilate->newCompatibleRenderPassDescriptor());
        rtDilate->setRenderPassDescriptor(rpDescDilate.get());
        if (!rtDilate->create()) {
            qWarning("lm: Failed to create postprocessing texture render target");
            return false;
        }
        QRhiResourceUpdateBatch *resUpd = rhi->nextResourceUpdateBatch();
        QRhiTextureSubresourceUploadDescription lightmapTexUpload(lightmapFP32.constData(), lightmapFP32.size());
        resUpd->uploadTexture(lightmapTex.get(), QRhiTextureUploadDescription({ 0, 0, lightmapTexUpload }));
        QSSGRhiShaderResourceBindingList bindings;
        QRhiSampler *nearestSampler = rhiCtx->sampler({ QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::Repeat });
        bindings.addTexture(0, QRhiShaderResourceBinding::FragmentStage, lightmapTex.get(), nearestSampler);
        renderer->rhiQuadRenderer()->prepareQuad(rhiCtx, resUpd);
        QSSGRef<QSSGRhiShaderPipeline> lmDilatePipeline = renderer->getRhiLightmapDilateShader();
        if (!lmDilatePipeline) {
            qWarning("lm: Failed to load shaders");
            return false;
        }
        QSSGRhiGraphicsPipelineState dilatePs;
        dilatePs.viewport = viewport;
        dilatePs.shaderPipeline = lmDilatePipeline.data();
        renderer->rhiQuadRenderer()->recordRenderQuadPass(rhiCtx, &dilatePs, rhiCtx->srb(bindings), rtDilate.get(), QSSGRhiQuadRenderer::UvCoords);
        resUpd = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult dilateReadResult;
        resUpd->readBackTexture({ dilatedLightmapTex.get() }, &dilateReadResult);
        cb->resourceUpdate(resUpd);

        // Submit and wait for completion.
        rhi->finish();

        lightmap.imageFP32 = dilateReadResult.data;

        // Reduce UV seams by collecting all edges (going through all
        // triangles), looking for (fuzzy)matching ones, then drawing lines
        // with blending on top.
        const DrawInfo &drawInfo(drawInfos[lmIdx]);
        const char *vbase = drawInfo.vertexData.constData();
        const quint32 *ibase = reinterpret_cast<const quint32 *>(drawInfo.indexData.constData());

        // topology is Triangles, would be indexed draw - get rid of the index
        // buffer, need nothing but triangles afterwards
        qsizetype assembledVertexCount = 0;
        for (SubMeshInfo &subMeshInfo : subMeshInfos[lmIdx])
            assembledVertexCount += subMeshInfo.count;
        QVector<QVector3D> smPos(assembledVertexCount);
        QVector<QVector3D> smNormal(assembledVertexCount);
        QVector<QVector2D> smCoord(assembledVertexCount);
        qsizetype vertexIdx = 0;
        for (SubMeshInfo &subMeshInfo : subMeshInfos[lmIdx]) {
            for (quint32 i = 0; i < subMeshInfo.count; ++i) {
                const quint32 idx = *(ibase + subMeshInfo.offset + i);
                const float *src = reinterpret_cast<const float *>(vbase + idx * drawInfo.vertexStride + drawInfo.positionOffset);
                float x = *src++;
                float y = *src++;
                float z = *src++;
                smPos[vertexIdx] = QVector3D(x, y, z);
                src = reinterpret_cast<const float *>(vbase + idx * drawInfo.vertexStride + drawInfo.normalOffset);
                x = *src++;
                y = *src++;
                z = *src++;
                smNormal[vertexIdx] = QVector3D(x, y, z);
                src = reinterpret_cast<const float *>(vbase + idx * drawInfo.vertexStride + drawInfo.lightmapUVOffset);
                x = *src++;
                y = *src++;
                smCoord[vertexIdx] = QVector2D(x, y);
                ++vertexIdx;
            }
        }

        QHash<Edge, EdgeUV> edgeUVMap;
        QVector<SeamUV> seams;
        for (vertexIdx = 0; vertexIdx < assembledVertexCount; vertexIdx += 3) {
            QVector3D triVert[3] = { smPos[vertexIdx], smPos[vertexIdx + 1], smPos[vertexIdx + 2] };
            QVector3D triNorm[3] = { smNormal[vertexIdx], smNormal[vertexIdx + 1], smNormal[vertexIdx + 2] };
            QVector2D triUV[3] = { smCoord[vertexIdx], smCoord[vertexIdx + 1], smCoord[vertexIdx + 2] };

            for (int i = 0; i < 3; ++i) {
                int i0 = i;
                int i1 = (i + 1) % 3;
                if (vectorLessThan(triVert[i1], triVert[i0]))
                    std::swap(i0, i1);

                const Edge e = {
                    { triVert[i0], triVert[i1] },
                    { triNorm[i0], triNorm[i1] }
                };
                const EdgeUV edgeUV = { { triUV[i0], triUV[i1] } };
                auto it = edgeUVMap.find(e);
                if (it == edgeUVMap.end()) {
                    edgeUVMap.insert(e, edgeUV);
                } else if (!qFuzzyCompare(it->uv[0], edgeUV.uv[0]) || !qFuzzyCompare(it->uv[1], edgeUV.uv[1])) {
                    if (!it->seam) {
                        seams.append(SeamUV({ { edgeUV.uv, it->uv } }));
                        it->seam = true;
                    }
                }
            }
        }
        qDebug() << "lm:" << seams.count() << "UV seams in" << lm.model;

        QByteArray workBuf(lightmap.imageFP32.size(), Qt::Uninitialized);
        for (int blendIter = 0; blendIter < LM_SEAM_BLEND_ITER_COUNT; ++blendIter) {
            memcpy(workBuf.data(), lightmap.imageFP32.constData(), lightmap.imageFP32.size());
            for (int seamIdx = 0, end = seams.count(); seamIdx != end; ++seamIdx) {
                const SeamUV &seam(seams[seamIdx]);
                blendLine(seam.uv[0][0], seam.uv[0][1],
                          seam.uv[1][0], seam.uv[1][1],
                          workBuf, lightmap.imageFP32, lightmap.pixelSize);
                blendLine(seam.uv[1][0], seam.uv[1][1],
                          seam.uv[0][0], seam.uv[0][1],
                          workBuf, lightmap.imageFP32, lightmap.pixelSize);
            }
        }

        qDebug() << "lm: Lightmap post-processing for model" << lm.model << "with key" << lm.model->lightmapKey
                 << "done in" << postProcessTimer.elapsed() << "ms";
    }

    return true;
}

bool QSSGLightmapperPrivate::storeLightmaps()
{
    const int bakedLightingModelCount = bakedLightingModels.count();
    QByteArray listContents;

    for (int lmIdx = 0; lmIdx < bakedLightingModelCount; ++lmIdx) {
        const QSSGBakedLightingModel &lm(bakedLightingModels[lmIdx]);
        // only care about the ones that want to store the lightmap image persistently
        if (!lm.model->hasLightmap())
            continue;

        QElapsedTimer writeTimer;
        writeTimer.start();

        const QString fn = QSSGLightmapper::lightmapAssetPathForSave(*lm.model, QSSGLightmapper::LightmapAsset::LightmapImage);
        const QByteArray fns = fn.toUtf8();
        listContents += fns;
        listContents += '\n';
        const Lightmap &lightmap(lightmaps[lmIdx]);

        if (SaveEXR(reinterpret_cast<const float *>(lightmap.imageFP32.constData()),
                    lightmap.pixelSize.width(), lightmap.pixelSize.height(),
                    4, false, fns.constData(), nullptr) < 0)
        {
            qWarning("lm: Failed to write out lightmap");
            return false;
        }

        qDebug() << "lm: Lightmap saved for model" << lm.model << "to" << fn
                 << "in" << writeTimer.elapsed() << "ms";

        const DrawInfo &bakeModelDrawInfo(drawInfos[lmIdx]);
        if (bakeModelDrawInfo.meshWithLightmapUV.isValid()) {
            writeTimer.start();
            QFile f(QSSGLightmapper::lightmapAssetPathForSave(*lm.model, QSSGLightmapper::LightmapAsset::MeshWithLightmapUV));
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                bakeModelDrawInfo.meshWithLightmapUV.save(&f);
            } else {
                qWarning("lm: Failed to write mesh with lightmap UV data to '%s'",
                         qPrintable(f.fileName()));
                return false;
            }
            qDebug() << "lm: Lightmap-compatible mesh saved for model" << lm.model << "to" << f.fileName()
                     << "in" << writeTimer.elapsed() << "ms";
        } // else the mesh had a lightmap uv channel to begin with, no need to save another version of it
    }

    QFile listFile(QSSGLightmapper::lightmapAssetPathForSave(QSSGLightmapper::LightmapAsset::LightmapImageList));
    if (!listFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning("lm: Failed to create lightmap list file %s",
                 qPrintable(listFile.fileName()));
        return false;
    }
    listFile.write(listContents);

    return true;
}

bool QSSGLightmapper::bake()
{
    QElapsedTimer totalTimer;
    totalTimer.start();

    qDebug() << "lm: Starting bake with" << d->bakedLightingModels.count() << "registered models";

    if (!d->commitGeometry())
        return false;

    if (!d->prepareLightmaps())
        return false;

    d->computeDirectLight();

    if (d->options.indirectLightEnabled)
        d->computeIndirectLight();

    if (!d->postProcess())
        return false;

    if (!d->storeLightmaps())
        return false;

    qDebug() << "lm: Lightmap baking took" << totalTimer.elapsed() << "ms";
    return true;
}

#else

QSSGLightmapper::QSSGLightmapper(QSSGRhiContext *, QSSGRenderer *)
{
}

QSSGLightmapper::~QSSGLightmapper()
{
}

void QSSGLightmapper::reset()
{
}

void QSSGLightmapper::setOptions(const QSSGLightmapperOptions &)
{
}

qsizetype QSSGLightmapper::add(const QSSGBakedLightingModel &)
{
    return 0;
}

bool QSSGLightmapper::bake()
{
    qWarning("Qt Quick 3D was built without the lightmapper; cannot bake lightmaps");
    return false;
}

#endif // QT_QUICK3D_HAS_LIGHTMAPPER

QString QSSGLightmapper::lightmapAssetPathForLoad(const QSSGRenderModel &model, LightmapAsset asset)
{
    QString result;
    if (!model.lightmapLoadPrefix.isEmpty()) {
        result += model.lightmapLoadPrefix;
        result += QLatin1Char('/');
    }
    switch (asset) {
    case LightmapAsset::LightmapImage:
        result += QStringLiteral("qlm_%1.exr").arg(model.lightmapKey);
        break;
    case LightmapAsset::MeshWithLightmapUV:
        result += QStringLiteral("qlm_%1.mesh").arg(model.lightmapKey);
        break;
    default:
        return QString();
    }
    return result;
}

QString QSSGLightmapper::lightmapAssetPathForSave(const QSSGRenderModel &model, LightmapAsset asset)
{
    switch (asset) {
    case LightmapAsset::LightmapImage:
        return QStringLiteral("qlm_%1.exr").arg(model.lightmapKey);
    case LightmapAsset::MeshWithLightmapUV:
        return QStringLiteral("qlm_%1.mesh").arg(model.lightmapKey);
    default:
        break;
    }
    return lightmapAssetPathForSave(asset);
}

QString QSSGLightmapper::lightmapAssetPathForSave(LightmapAsset asset)
{
    switch (asset) {
    case LightmapAsset::LightmapImageList:
        return QStringLiteral("qlm_list.txt");
    default:
        break;
    }
    return QString();
}

QT_END_NAMESPACE
