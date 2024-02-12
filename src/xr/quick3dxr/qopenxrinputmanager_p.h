// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRINPUTMANAGER_H
#define QOPENXRINPUTMANAGER_H

#include <QObject>

#include <openxr/openxr.h>
#include <functional>
#include "qopenxractionmapper_p.h"
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
class QOpenXRHandTrackerInput;
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
        RightHand = 1,
    };

    void pollActions();
    void updatePoses(XrTime predictedDisplayTime, XrSpace appSpace);
    void updateHandtracking(XrTime predictedDisplayTime, XrSpace appSpace, bool aimExtensionEnabled);

    XrSpace handSpace(Hand hand);
    bool isHandActive(Hand hand);

    XrSpace handTrackerSpace(Hand handtracker);
    bool isHandTrackerActive(Hand handtracker);

    void setPosePosition(Hand hand, const QVector3D &position);
    void setPoseRotation(Hand hand, const QQuaternion &rotation);

    QOpenXRHandInput *leftHandInput() const;
    QOpenXRHandInput *rightHandInput() const;
    QOpenXRHandTrackerInput *leftHandTrackerInput() const;
    QOpenXRHandTrackerInput *rightHandTrackerInput() const;
    QOpenXRGamepadInput *gamepadInput() const;

    PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_;
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_;
    PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_;

    PFN_xrGetHandMeshFB xrGetHandMeshFB_;

    XrHandTrackerEXT handTracker[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};

    XrHandJointLocationEXT jointLocations[2][XR_HAND_JOINT_COUNT_EXT];
    XrHandJointVelocityEXT jointVelocities[2][XR_HAND_JOINT_COUNT_EXT];

private:
    QOpenXRInputManager();
    ~QOpenXRInputManager();

    void setupHandTracking();
    void setupActions();
    void destroyActions();
    bool checkXrResult(const XrResult &result, const char *debugText = nullptr);
    void setPath(XrPath &path, const QByteArray &pathString);

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

    enum SubPathSelector {LeftHandSubPath = 1, RightHandSubPath = 2, BothHandsSubPath = 3};

    struct QXRHandComponentPath
    {
        XrPath paths[2] = {{}, {}};
        QByteArray componentPathString;
    };
    QXRHandComponentPath makeQXRPath(const QByteArrayView path);

    struct HandInputAction {
        QOpenXRActionMapper::InputAction id;
        const char *shortName;
        const char *localizedName;
        XrActionType type;
        float lastValue;
    };

    QList<HandInputAction> handInputActions;

    struct HandActions {
        XrAction actions[QOpenXRActionMapper::NumActions] = {};
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
    QOpenXRHandTrackerInput *m_handTrackerInputState[2];
    QOpenXRGamepadInput *m_gamepadInputState;
    XrPath m_gamepadSubactionPath;
    HandActions m_handActions;
    GamepadActions m_gamepadActions;

    uint m_aimStateFlags[2] = {};
    bool m_initialized = false;
    bool m_disableGamepad = false;
    bool m_validAimStateFromUpdatePoses[2] = {false, false};
};

QT_END_NAMESPACE

#endif // QOPENXRINPUTMANAGER_H
