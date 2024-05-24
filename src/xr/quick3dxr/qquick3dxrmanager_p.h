// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRMANAGER_H
#define QOPENXRMANAGER_H

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

#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>

#include <QtCore/QObject>
#include <QtCore/QVersionNumber>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QQuickRenderControl;
class QQuick3DNode;
class QQuick3DViewport;
class QQuick3DXrEyeCamera;
class QQuick3DXrView;
class QQuick3DXrOrigin;
class QOpenXRInputManager;
class QRhiTexture;
class QQuick3DXrAnimationDriver;

class QQuick3DXrManagerPrivate;

// FIXME: Follow the same pattern as of the XrManager once we have
// the input for AVP in place.
#if defined(Q_OS_VISIONOS)
class QQuick3DXrInputManager {};
#else
using QQuick3DXrInputManager = QOpenXRInputManager;
#endif

class QQuick3DXrManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuick3DXrManager)
public:
    explicit QQuick3DXrManager(QObject *parent = nullptr);
    ~QQuick3DXrManager();

    bool isReady() const;

    bool initialize();
    void teardown();

    bool isValid() const;

    void setPassthroughEnabled(bool enabled);
    bool isPassthroughEnabled() const;

    QtQuick3DXr::FoveationLevel getFixedFoveationLevel() const;
    void setFixedFoveationLevel(QtQuick3DXr::FoveationLevel level);

    QtQuick3DXr::ReferenceSpace getReferenceSpace() const;
    void setReferenceSpace(QtQuick3DXr::ReferenceSpace newReferenceSpace);

    bool isDepthSubmissionEnabled() const;
    void setDepthSubmissionEnabled(bool enable);

    QString errorString() const;

    void setSamples(int samples);

    void setMultiviewRenderingEnabled(bool enable);
    bool isMultiViewRenderingEnabled() const;
    bool isMultiViewRenderingSupported() const;

private Q_SLOTS:
    void update();

Q_SIGNALS:
    void initialized();
    void sessionEnded();
    void xrOriginChanged();
    void frameReady(QRhiTexture *colorBuffer);
    void referenceSpaceChanged();

protected:
    bool event(QEvent *e) override;

private:
    friend class QOpenXRRuntimeInfo;
    friend class QQuick3DXrView;

    QQuick3DXrInputManager *getInputManager() const;

    bool setupGraphics();

    void processXrEvents();
    void renderFrame();

    void preSetupQuickScene();
    bool setupQuickScene();
    void checkOrigin();

    bool supportsPassthrough() const;

    QQuickWindow *m_quickWindow = nullptr;
    QQuickRenderControl *m_renderControl = nullptr;
    QQuick3DViewport *m_vrViewport = nullptr;
    QQuick3DXrOrigin *m_xrOrigin = nullptr;
    QQuick3DXrAnimationDriver *m_animationDriver = nullptr;

    std::unique_ptr<QQuick3DXrManagerPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QOPENXRMANAGER_H
