// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QString>
#include <QtQuick3DRuntimeRender/private/qssglayerrenderdata_p.h>
#include <QtQuick3DRuntimeRender/private/qssglightmapbaker_p.h>
#include "qssgrendercontextcore.h"

QT_BEGIN_NAMESPACE

struct QSSGLightmapBakerPrivate
{
    QSSGLightmapBaker::Context ctx;
    std::unique_ptr<QSSGLightmapper> lightmapper = nullptr;
    QSSGLightmapBaker::Status currentStatus = QSSGLightmapBaker::Status::Preparing;
};

QSSGLightmapBaker::QSSGLightmapBaker(const QSSGLightmapBaker::Context &ctx)
    : d(new QSSGLightmapBakerPrivate)
{
    d->ctx = ctx;

    auto &env = d->ctx.env;
    auto &cb = d->ctx.callbacks;

    d->currentStatus = QSSGLightmapBaker::Status::Preparing;
    d->lightmapper = std::make_unique<QSSGLightmapper>(env.rhiCtx, env.renderer);
    d->lightmapper->setOptions(env.lmOptions);
    d->lightmapper->setOutputCallback(cb.lightmapBakingOutput);
}

QSSGLightmapBaker::Status QSSGLightmapBaker::process()
{
    auto &env = d->ctx.env;
    auto &settings = d->ctx.settings;
    auto &callbacks = d->ctx.callbacks;

    if (d->currentStatus == Status::Preparing) {
        // We need to prepare for lightmap baking by doing another frame so that
        // we can reload all meshes to use the original one and NOT the baked one.
        // When disableLightmaps is set on the layer, the mesh loader will always load the
        // original mesh and not the lightmap mesh.
        callbacks.setCurrentlyBaking(true);
        callbacks.triggerNewFrame(true);

        d->currentStatus = Status::Running;
    } else if (d->currentStatus == Status::Running) {
        {
            QRhiCommandBuffer *cb = env.rhiCtx->commandBuffer();
            cb->debugMarkBegin("Quick3D lightmap baking/denoising");
            if (settings.bakeRequested) {
                // bakedLightingModels contains all models with
                // usedInBakedLighting: true. These, together with lights that
                // have a bakeMode set to either Indirect or All, form the
                // lightmapped scene. A lightmap is stored persistently only
                // for models that have their lightmapKey set.
                const auto &bakedLightingModels = callbacks.modelsToBake();
                for (int i = 0, ie = bakedLightingModels.size(); i != ie; ++i)
                    d->lightmapper->add(bakedLightingModels[i]);

                d->lightmapper->bake();
            } else if (settings.denoiseRequested) {
                d->lightmapper->denoise();
            }
            cb->debugMarkEnd();
        }

        callbacks.setCurrentlyBaking(false);
        callbacks.triggerNewFrame(true);
        d->currentStatus = Status::Finished;

        if (settings.quitWhenFinished) {
            qDebug("Lightmap baking/denoising done, exiting application");
            QMetaObject::invokeMethod(qApp, "quit");
        }
    }

    return d->currentStatus;
}

QT_END_NAMESPACE

