// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DRENDERSTATS_H
#define QQUICK3DRENDERSTATS_H

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

#include <QtQuick3D/qtquick3dglobal.h>
#include <QtCore/qobject.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK3D_EXPORT QQuick3DRenderStats : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(float frameTime READ frameTime NOTIFY frameTimeChanged)
    Q_PROPERTY(float renderTime READ renderTime NOTIFY renderTimeChanged)
    Q_PROPERTY(float renderPrepareTime READ renderPrepareTime NOTIFY renderTimeChanged)
    Q_PROPERTY(float syncTime READ syncTime NOTIFY syncTimeChanged)
    Q_PROPERTY(float maxFrameTime READ maxFrameTime NOTIFY maxFrameTimeChanged)

public:
    QQuick3DRenderStats(QObject *parent = nullptr);

    int fps() const;
    float frameTime() const;
    float renderTime() const;
    float renderPrepareTime() const;
    float syncTime() const;
    float maxFrameTime() const;

    void startSync();
    void endSync(bool dump = false);

    void startRender();
    void startRenderPrepare();
    void endRenderPrepare();
    void endRender(bool dump = false);

Q_SIGNALS:
    void fpsChanged();
    void frameTimeChanged();
    void renderTimeChanged();
    void syncTimeChanged();
    void maxFrameTimeChanged();

private:
    float timestamp() const;

    QElapsedTimer m_frameTimer;
    int m_frameCount = 0;
    float m_secTimer = 0;
    float m_notifyTimer = 0;
    float m_renderStartTime = 0;
    float m_renderPrepareStartTime = 0;
    float m_syncStartTime = 0;

    float m_internalMaxFrameTime = 0;
    float m_maxFrameTime = 0;

    int m_fps = 0;

    struct Results {
        float frameTime = 0;
        float renderTime = 0;
        float renderPrepareTime = 0;
        float syncTime = 0;
    };

    Results m_results;
    Results m_notifiedResults;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuick3DRenderStats *)

#endif // QQUICK3DRENDERSTATS_H
