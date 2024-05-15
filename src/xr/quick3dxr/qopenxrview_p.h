// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRVIEW_H
#define QOPENXRVIEW_H

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


#include <QtCore/QObject>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>

#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQuick3D/private/qquick3dsceneenvironment_p.h>
#include <QtQuick3D/private/qquick3dpickresult_p.h>

#include <QtQuick3DXr/private/qopenxrmanager_p.h>
#include <QtQuick3DXr/private/qquick3dxrorigin_p.h>
#include <QtQuick3DXr/private/qopenxrruntimeinfo_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DSceneEnvironment;
class QOpenXRHandInput;
class QOpenXRHandTrackerInput;
class QOpenXRGamepadInput;
class QRhiTexture;

class QOpenXRItem;

class Q_QUICK3DXR_EXPORT QOpenXRView : public QQuick3DNode
{
    Q_OBJECT

    Q_PROPERTY(QQuick3DXrOrigin *xrOrigin READ xrOrigin NOTIFY xrOriginChanged)
    Q_PROPERTY(QQuick3DSceneEnvironment *environment READ environment WRITE setEnvironment NOTIFY environmentChanged)
    Q_PROPERTY(QOpenXRHandInput *leftHandInput READ leftHandInput CONSTANT)
    Q_PROPERTY(QOpenXRHandInput *rightHandInput READ rightHandInput CONSTANT)
    Q_PROPERTY(QOpenXRHandTrackerInput *leftHandTrackerInput READ leftHandTrackerInput CONSTANT)
    Q_PROPERTY(QOpenXRHandTrackerInput *rightHandTrackerInput READ rightHandTrackerInput CONSTANT)
    Q_PROPERTY(QOpenXRGamepadInput *gamepadInput READ gamepadInput CONSTANT)
    Q_PROPERTY(bool passthroughSupported READ isPassthroughSupported CONSTANT)
    Q_PROPERTY(bool enablePassthrough READ enablePassthrough WRITE setEnablePassthrough NOTIFY enablePassthroughChanged FINAL)
    Q_PROPERTY(QOpenXRRuntimeInfo *runtimeInfo READ runtimeInfo CONSTANT)
    Q_PROPERTY(bool quitOnSessionEnd READ isQuitOnSessionEndEnabled WRITE setQuitOnSessionEnd NOTIFY quitOnSessionEndChanged FINAL)
    Q_PROPERTY(QQuick3DRenderStats *renderStats READ renderStats CONSTANT)
    Q_PROPERTY(FoveationLevel fixedFoveation READ fixedFoveation WRITE setFixedFoveation NOTIFY fixedFoveationChanged FINAL)
    Q_PROPERTY(ReferenceSpace referenceSpace READ referenceSpace WRITE setReferenceSpace NOTIFY referenceSpaceChanged FINAL)
    Q_PROPERTY(bool enableDepthSubmission READ isDepthSubmissionEnabled WRITE setEnableDepthSubmission NOTIFY enableDepthSubmissionChanged FINAL)
    QML_NAMED_ELEMENT(XrView)

public:
    enum FoveationLevel {
        NoFoveation = 0,
        LowFoveation = 1,
        MediumFoveation = 2,
        HighFoveation = 3
    };
    Q_ENUM(FoveationLevel)

    enum class ReferenceSpace {
        ReferenceSpaceUnknown,
        ReferenceSpaceLocal,
        ReferenceSpaceStage,
        ReferenceSpaceLocalFloor
    };
    Q_ENUM(ReferenceSpace)

    explicit QOpenXRView();
    ~QOpenXRView();

    QQuick3DXrOrigin *xrOrigin() const;
    QQuick3DSceneEnvironment *environment() const;
    QOpenXRHandInput *leftHandInput() const;
    QOpenXRHandInput *rightHandInput() const;
    QOpenXRHandTrackerInput *leftHandTrackerInput() const;
    QOpenXRHandTrackerInput *rightHandTrackerInput() const;
    QOpenXRGamepadInput *gamepadInput() const;

    bool isPassthroughSupported() const;
    bool enablePassthrough() const;

    FoveationLevel fixedFoveation() const;
    void setFixedFoveation(FoveationLevel level);

    QOpenXRRuntimeInfo *runtimeInfo() const;

    bool isQuitOnSessionEndEnabled() const;

    QQuick3DRenderStats *renderStats() const;

    Q_INVOKABLE QQuick3DPickResult rayPick(const QVector3D &origin, const QVector3D &direction) const;
    Q_INVOKABLE QList<QQuick3DPickResult> rayPickAll(const QVector3D &origin, const QVector3D &direction) const;

    Q_INVOKABLE void setTouchpoint(QQuickItem *target, const QPointF &position, int pointId, bool active);
    Q_INVOKABLE QVector3D processTouch(const QVector3D &pos, int pointId);
    Q_INVOKABLE QVariantMap touchpointState(int pointId) const;

    ReferenceSpace referenceSpace() const;
    void setReferenceSpace(ReferenceSpace newReferenceSpace);

    bool isDepthSubmissionEnabled() const;

    void registerXrItem(QOpenXRItem *newXrItem);
    void unregisterXrItem(QOpenXRItem *xrItem);

public Q_SLOTS:
    void setEnvironment(QQuick3DSceneEnvironment * environment);
    void setEnablePassthrough(bool enable);
    void setQuitOnSessionEnd(bool enable);
    void setEnableDepthSubmission(bool enable);

private Q_SLOTS:
    void updateViewportGeometry();
    void handleSessionEnded();
    void handleClearColorChanged();
    void handleAAChanged();
    bool init();

Q_SIGNALS:
    void initializeFailed(const QString &errorString);
    void sessionEnded();
    void xrOriginChanged(QQuick3DXrOrigin* xrOrigin);
    void environmentChanged(QQuick3DSceneEnvironment * environment);
    void enablePassthroughChanged(bool enable);
    void quitOnSessionEndChanged();
    void fixedFoveationChanged();
    void frameReady(QRhiTexture *colorBuffer); // tooling
    void referenceSpaceChanged();
    void enableDepthSubmissionChanged();

private:
    // The XrView does not expose the View3D in its public interface. This is intentional.
    QQuick3DViewport *view3d() const;

    QPointer<QQuick3DSceneEnvironment> m_sceneEnvironment;
    QOpenXRManager m_openXRManager;
    mutable QOpenXRRuntimeInfo m_openXRRuntimeInfo;
    bool m_quitOnSessionEnd = true;
    bool m_inDestructor = false;
    bool m_isInitialized = false;

    friend class QOpenXRVirtualMouse;
    QList<QOpenXRItem *> m_xrItems;
    struct XrTouchState;
    XrTouchState *m_touchState = nullptr;
};

QT_END_NAMESPACE

#endif // QOPENXRVIEW_H
