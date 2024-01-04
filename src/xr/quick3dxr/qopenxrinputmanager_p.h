// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRINPUTMANAGER_H
#define QOPENXRINPUTMANAGER_H

#include <QObject>

#include <openxr/openxr.h>
#include <functional>

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

QT_BEGIN_NAMESPACE

class QOpenXRHandInput;
class QOpenXRGamepadInput;

class QOpenXRInputManager : public QObject
{
    Q_OBJECT
public:
    static QOpenXRInputManager* instance();

    void init(XrInstance instance, XrSession session);
    void teardown();

    enum Hand {
        LeftHand = 0,
        RightHand = 1
    };

    void pollActions();
    void updatePoses(XrTime predictedDisplayTime, XrSpace appSpace);

    XrSpace handSpace(Hand hand);
    bool isHandActive(Hand hand);

    void setPosePosition(Hand hand, const QVector3D &position);
    void setPoseRotation(Hand hand, const QQuaternion &rotation);

    QOpenXRHandInput* leftHandInput() const;
    QOpenXRHandInput* rightHandInput() const;
    QOpenXRGamepadInput* gamepadInput() const;

private:
    QOpenXRInputManager();
    ~QOpenXRInputManager();

    void setupActions();
    bool checkXrResult(const XrResult &result);
    void setPath(XrPath &path, const char *pathString);

    void createAction(XrActionType type,
                      const char *name,
                      const char *localizedName,
                      int numSubactions,
                      XrPath *subactionPath,
                      XrAction &action);
    void getBoolInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(bool)> setter);
    void getFloatInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(float)> setter);

    XrInstance m_instance{XR_NULL_HANDLE};
    XrSession m_session{XR_NULL_HANDLE};

    struct HandActions {
        XrAction button1PressedAction{XR_NULL_HANDLE};
        XrAction button1TouchedAction{XR_NULL_HANDLE};
        XrAction button2PressedAction{XR_NULL_HANDLE};
        XrAction button2TouchedAction{XR_NULL_HANDLE};
        XrAction buttonMenuPressedAction{XR_NULL_HANDLE};
        XrAction buttonMenuTouchedAction{XR_NULL_HANDLE};
        XrAction buttonSystemPressedAction{XR_NULL_HANDLE};
        XrAction buttonSystemTouchedAction{XR_NULL_HANDLE};
        XrAction squeezeValueAction{XR_NULL_HANDLE};
        XrAction squeezeForceAction{XR_NULL_HANDLE};
        XrAction squeezePressedAction{XR_NULL_HANDLE};
        XrAction triggerValueAction{XR_NULL_HANDLE};
        XrAction triggerPressedAction{XR_NULL_HANDLE};
        XrAction triggerTouchedAction{XR_NULL_HANDLE};
        XrAction thumbstickXAction{XR_NULL_HANDLE};
        XrAction thumbstickYAction{XR_NULL_HANDLE};
        XrAction thumbstickPressedAction{XR_NULL_HANDLE};
        XrAction thumbstickTouchedAction{XR_NULL_HANDLE};
        XrAction thumbrestTouchedAction{XR_NULL_HANDLE};
        XrAction trackpadXAction{XR_NULL_HANDLE};
        XrAction trackpadYAction{XR_NULL_HANDLE};
        XrAction trackpadForceAction{XR_NULL_HANDLE};
        XrAction trackpadTouchedAction{XR_NULL_HANDLE};
        XrAction trackpadPressedAction{XR_NULL_HANDLE};
        XrAction gripPoseAction{XR_NULL_HANDLE};
        XrAction aimPoseAction{XR_NULL_HANDLE};
        XrAction hapticAction{XR_NULL_HANDLE};
    };

    struct GamepadActions {
        XrAction buttonMenuPressedAction{XR_NULL_HANDLE};
        XrAction buttonViewPressedAction{XR_NULL_HANDLE};
        XrAction buttonAPressedAction{XR_NULL_HANDLE};
        XrAction buttonBPressedAction{XR_NULL_HANDLE};
        XrAction buttonXPressedAction{XR_NULL_HANDLE};
        XrAction buttonYPressedAction{XR_NULL_HANDLE};
        XrAction buttonDownPressedAction{XR_NULL_HANDLE};
        XrAction buttonRightPressedAction{XR_NULL_HANDLE};
        XrAction buttonUpPressedAction{XR_NULL_HANDLE};
        XrAction buttonLeftPressedAction{XR_NULL_HANDLE};
        XrAction shoulderLeftPressedAction{XR_NULL_HANDLE};
        XrAction shoulderRightPressedAction{XR_NULL_HANDLE};
        XrAction thumbstickLeftPressedAction{XR_NULL_HANDLE};
        XrAction thumbstickRightPressedAction{XR_NULL_HANDLE};
        XrAction triggerLeftAction{XR_NULL_HANDLE};
        XrAction triggerRightAction{XR_NULL_HANDLE};
        XrAction thumbstickLeftXAction{XR_NULL_HANDLE};
        XrAction thumbstickLeftYAction{XR_NULL_HANDLE};
        XrAction thumbstickRightXAction{XR_NULL_HANDLE};
        XrAction thumbstickRightYAction{XR_NULL_HANDLE};
        XrAction hapticLeftAction{XR_NULL_HANDLE};
        XrAction hapticRightAction{XR_NULL_HANDLE};
        XrAction hapticLeftTriggerAction{XR_NULL_HANDLE};
        XrAction hapticRightTriggerAction{XR_NULL_HANDLE};
    };

    // Input State
    XrActionSet m_actionSet{XR_NULL_HANDLE};
    XrPath m_handSubactionPath[2];
    XrSpace m_handGripSpace[2];
    XrSpace m_handAimSpace[2];

    QOpenXRHandInput *m_handInputState[2];
    QOpenXRGamepadInput *m_gamepadInputState;
    XrPath m_gamepadSubactionPath;
    HandActions m_handActions;
    GamepadActions m_gamepadActions;

    bool m_initialized = false;
    bool m_disableGamepad = false;
};

QT_END_NAMESPACE

#endif // QOPENXRINPUTMANAGER_H
