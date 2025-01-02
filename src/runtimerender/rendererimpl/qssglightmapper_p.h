// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSSGLIGHTMAPPER_P_H
#define QSSGLIGHTMAPPER_P_H

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

#include <QString>

QT_BEGIN_NAMESPACE

struct QSSGLightmapperPrivate;
struct QSSGBakedLightingModel;
class QSSGRhiContext;
class QSSGRenderer;
struct QSSGRenderModel;

struct QSSGLightmapperOptions
{
    float opacityThreshold = 0.5f;
    float bias = 0.005f;
    bool useAdaptiveBias = true;
    bool indirectLightEnabled = true;
    int indirectLightSamples = 256;
    int indirectLightWorkgroupSize = 32;
    int indirectLightBounces = 3;
    float indirectLightFactor = 1.0f;
};

class QSSGLightmapper
{
public:
    enum class BakingStatus {
        None,
        Progress,
        Warning,
        Error,
        Cancelled,
        Complete
    };

    struct BakingControl {
        bool cancelled = false;
    };

    typedef std::function<void(BakingStatus, std::optional<QString>, BakingControl*)> Callback;

    QSSGLightmapper(QSSGRhiContext *rhiCtx, QSSGRenderer *renderer);
    ~QSSGLightmapper();
    void reset();
    void setOptions(const QSSGLightmapperOptions &options);
    void setOutputCallback(Callback callback);
    qsizetype add(const QSSGBakedLightingModel &model);
    bool bake();

    enum class LightmapAsset {
        LightmapImage,
        MeshWithLightmapUV,
        LightmapImageList
    };
    static QString lightmapAssetPathForLoad(const QSSGRenderModel &model, LightmapAsset asset);
    static QString lightmapAssetPathForSave(const QSSGRenderModel &model, LightmapAsset asset, const QString& outputFolder = {});
    static QString lightmapAssetPathForSave(LightmapAsset asset, const QString& outputFolder = {});

private:
#ifdef QT_QUICK3D_HAS_LIGHTMAPPER
    QSSGLightmapperPrivate *d = nullptr;
#endif
};

QT_END_NAMESPACE

#endif
