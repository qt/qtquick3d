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
#include <QtQuick3DXr/private/qopenxrorigin_p.h>
#include <QtQuick3DXr/private/qopenxrruntimeinfo_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DSceneEnvironment;
class QOpenXRHandInput;
class QOpenXRHandTrackerInput;
class QOpenXRGamepadInput;

class Q_QUICK3DXR_EXPORT QOpenXRView : public QQuick3DNode
{
    Q_OBJECT

    Q_PROPERTY(QOpenXROrigin *xrOrigin READ xrOrigin NOTIFY xrOriginChanged)
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

    QML_NAMED_ELEMENT(XrView)

public:
    explicit QOpenXRView();
    ~QOpenXRView();

    QOpenXROrigin *xrOrigin() const;
    QQuick3DSceneEnvironment *environment() const;
    QOpenXRHandInput *leftHandInput() const;
    QOpenXRHandInput *rightHandInput() const;
    QOpenXRHandTrackerInput *leftHandTrackerInput() const;
    QOpenXRHandTrackerInput *rightHandTrackerInput() const;
    QOpenXRGamepadInput *gamepadInput() const;

    bool isPassthroughSupported() const;
    bool enablePassthrough() const;

    QOpenXRRuntimeInfo *runtimeInfo() const;

    bool isQuitOnSessionEndEnabled() const;

    Q_INVOKABLE QQuick3DPickResult rayPick(const QVector3D &origin, const QVector3D &direction) const;
    Q_INVOKABLE QList<QQuick3DPickResult> rayPickAll(const QVector3D &origin, const QVector3D &direction) const;

public Q_SLOTS:
    void setEnvironment(QQuick3DSceneEnvironment * environment);
    void setEnablePassthrough(bool enable);
    void setQuitOnSessionEnd(bool enable);

private Q_SLOTS:
    void updateViewportGeometry();
    void handleSessionEnded();
    void handleClearColorChanged();

Q_SIGNALS:
    void initializeFailed(const QString &errorString);
    void sessionEnded();
    void xrOriginChanged(QOpenXROrigin* xrOrigin);
    void environmentChanged(QQuick3DSceneEnvironment * environment);
    void enablePassthroughChanged(bool enable);
    void quitOnSessionEndChanged();

private:
    // The XrView does not expose the View3D in its public interface. This is intentional.
    QQuick3DViewport *view3d() const;

    QOpenXRManager m_openXRManager;
    mutable QOpenXRRuntimeInfo m_openXRRuntimeInfo;
    bool m_quitOnSessionEnd = true;
    bool m_inDestructor = false;

    friend class QOpenXRVirtualMouse;
};

QT_END_NAMESPACE

#endif // QOPENXRVIEW_H
