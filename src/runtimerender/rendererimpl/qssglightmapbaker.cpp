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
};

QSSGLightmapBaker::QSSGLightmapBaker(const QSSGLightmapBaker::Context &ctx)
    : d(new QSSGLightmapBakerPrivate)
{
    d->ctx = ctx;

    auto &env = d->ctx.env;
    auto &cb = d->ctx.callbacks;

    d->lightmapper = std::make_unique<QSSGLightmapper>(env.rhiCtx, env.renderer);
    d->lightmapper->setOptions(env.lmOptions);
    d->lightmapper->setOutputCallback(cb.lightmapBakingOutput);

    // bakedLightingModels contains all models with
    // usedInBakedLighting: true. These, together with lights that
    // have a bakeMode set to either Indirect or All, form the
    // lightmapped scene. A lightmap is stored persistently only
    // for models that have their lightmapKey set.
    for (int i = 0, ie = env.bakedLightingModels.size(); i != ie; ++i)
        d->lightmapper->add(env.bakedLightingModels[i]);
}

void QSSGLightmapBaker::process()
{
    auto &env = d->ctx.env;
    auto &settings = d->ctx.settings;

    QRhiCommandBuffer *cb = env.rhiCtx->commandBuffer();
    cb->debugMarkBegin("Quick3D lightmap baking");
    if (settings.bakeRequested) {
        d->lightmapper->bake();
    }
    cb->debugMarkEnd();

    if (settings.quitWhenFinished) {
        qDebug("Lightmap baking done, exiting application");
        QMetaObject::invokeMethod(qApp, "quit");
    }
}

QT_END_NAMESPACE

