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
#include <ssg/qssglightmapper.h>

#include <QString>

QT_BEGIN_NAMESPACE

struct QSSGLightmapperPrivate;
struct QSSGBakedLightingModel;
class QSSGRhiContext;
class QSSGRenderer;
struct QSSGRenderModel;

class QSSGLightmapper
{
public:
    enum class BakingStatus {
        None,
        Info,
        Warning,
        Error,
        Cancelled,
        Failed,
        Complete
    };

    struct BakingControl {
        bool cancelled = false;
    };

    /*
     * Payload:
     *  int status                  (BakingStatus)
     *  string message              (Might be empty)
     *  qint64 totalTimeRemaining   (in ms)
     *  double totalProgress        (Range 0 - 1)
     *  qint64 totalTimeElapsed     (in ms)
     */
    typedef std::function<void(const QVariantMap &payload, BakingControl*)> Callback;

    QSSGLightmapper(QSSGRhiContext *rhiCtx, QSSGRenderer *renderer);
    ~QSSGLightmapper();
    void reset();
    void setOptions(const QSSGLightmapperOptions &options);
    void setOutputCallback(Callback callback);
    qsizetype add(const QSSGBakedLightingModel &model);
    bool bake();
    bool denoise();

private:
#ifdef QT_QUICK3D_HAS_LIGHTMAPPER
    QSSGLightmapperPrivate *d = nullptr;
#endif
};

QT_END_NAMESPACE

#endif
