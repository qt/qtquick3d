// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DLIGHTMAPBAKER_P_H
#define QQUICK3DLIGHTMAPBAKER_P_H

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
#include <QtQuick/private/qquickview_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DViewport;

class Q_QUICK3D_EXPORT QQuick3DLightmapBaker : public QObject
{
    Q_OBJECT
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
        void reset();
        void requestCancel();
        bool isCancelled() const;

    private:
        std::atomic_int cancelFlag = 0;
    };

    typedef std::function<void(BakingStatus, std::optional<QString>, BakingControl*)> Callback;

    explicit QQuick3DLightmapBaker(QQuick3DViewport *view);
    ~QQuick3DLightmapBaker();

    void bake(Callback callback);
    void bake();

private slots:
    void onLmCancelButtonClicked();
    void onLmWindowClosing(QQuickCloseEvent *event);

private:
    void updateView();

    bool m_bakingRequested = false;
    BakingControl *m_bakingControl;
    QQuick3DViewport *m_view = nullptr;
    Callback m_callback;

    // For the internal status/control provided by the default impl through DebugView
    QQuickView *m_lmWindow = nullptr;
    bool m_windowCancelRequested = false;

    friend class QQuick3DSceneRenderer;
};

QT_END_NAMESPACE


#endif // QQUICK3DLIGHTMAPBAKER_P_H
        ;
