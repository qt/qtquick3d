// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRCONTROLLER_H
#define QOPENXRCONTROLLER_H

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

#include <QtQuick3DXr/qtquick3dxrglobal.h>
#include "qopenxrgamepadinput_p.h"
#include "qopenxrhandinput_p.h"
#include "qopenxractionmapper_p.h"
#include "qopenxrhandtrackerinput_p.h"
#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtQml/QQmlEngine>

QT_BEGIN_NAMESPACE

class QOpenXRInputManager;
class Q_QUICK3DXR_EXPORT QOpenXRController : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(Controller controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(QOpenXRActionMapper* actionMapper READ actionMapper WRITE setActionMapper NOTIFY actionMapperChanged FINAL)
    Q_PROPERTY(QOpenXRHandInput* handInput READ handInput NOTIFY handInputChanged)
    QML_NAMED_ELEMENT(XrController)
public:
    enum Controller {
        ControllerNone = 0,
        ControllerLeft = 1,
        ControllerRight = 2,
        ControllerGamepad = 3
    };
    Q_ENUM(Controller)

    QOpenXRController();

    QOpenXRController::Controller controller() const;
    void setController(QOpenXRController::Controller newController);

    QOpenXRHandInput *handInput() const;
    Q_INVOKABLE QOpenXRGamepadInput *gamepadInput() const;

    QOpenXRActionMapper *actionMapper() const;
    void setActionMapper(QOpenXRActionMapper *newActionMapper);

Q_SIGNALS:
    void controllerChanged();
    void handInputChanged();
    void actionMapperChanged();

private:
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    QOpenXRInputManager *m_inputManager = nullptr;
#endif // Q_NO_TEMPORARY_DISABLE_XR_API
    QOpenXRActionMapper *m_actionMapper = nullptr;
    Controller m_controller = ControllerNone;
    QMetaObject::Connection m_posePositionConnection;
    QMetaObject::Connection m_poseRotationConnection;
    QMetaObject::Connection m_isActiveConnection;
    QMetaObject::Connection m_inputActionConnection;
    QMetaObject::Connection m_actionMapperConnection;
};

QT_END_NAMESPACE

#endif // QOPENXRCONTROLLER_H
