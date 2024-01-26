// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrinputmanager_p.h"
#include "qopenxrhelpers_p.h"
#include "qopenxrhandinput_p.h"
#include "qopenxrgamepadinput_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

QOpenXRInputManager::QOpenXRInputManager()
{
    m_handInputState[QOpenXRInputManager::LeftHand] = new QOpenXRHandInput(this);
    m_handInputState[QOpenXRInputManager::RightHand] = new QOpenXRHandInput(this);
    m_gamepadInputState = new QOpenXRGamepadInput(this);
}

QOpenXRInputManager::~QOpenXRInputManager()
{
    teardown();
    delete m_handInputState[QOpenXRInputManager::LeftHand];
    delete m_handInputState[QOpenXRInputManager::RightHand];
    delete m_gamepadInputState;

    m_handInputState[QOpenXRInputManager::LeftHand] = nullptr;
    m_handInputState[QOpenXRInputManager::RightHand] = nullptr;
    m_gamepadInputState = nullptr;
}

QOpenXRInputManager *QOpenXRInputManager::instance()
{
    static QOpenXRInputManager instance;
    return &instance;
}

void QOpenXRInputManager::init(XrInstance instance, XrSession session)
{
    if (m_initialized) {
        qWarning() << "QOpenXRInputManager: Trying to initialize an already initialized sesssion";
        teardown();
    }

    m_instance = instance;
    m_session = session;

    m_disableGamepad = false;

    // Gamepad actions lead to endless XR_ERROR_RUNTIME_FAILURE in
    // xrSyncActions with the Meta XR Simulator (v57 at least). The Simulator
    // can use an XBox controller to simulate head/controller/hand input (and
    // not as a gamepad), so it is not clear if it is supposed to have gamepad
    // support to begin with. So just disable it all.
    XrInstanceProperties instanceProperties = {};
    instanceProperties.type = XR_TYPE_INSTANCE_PROPERTIES;
    if (xrGetInstanceProperties(m_instance, &instanceProperties) == XR_SUCCESS) {
        if (strstr(instanceProperties.runtimeName, "Meta XR Simulator")) {
            qDebug("QOpenXRInputManager: Disabling gamepad actions due to running on the Simulator");
            m_disableGamepad = true;
        }
    }

    // Also on Android.
    // ### Why?
#ifdef XR_USE_PLATFORM_ANDROID
    qDebug("QOpenXRInputManager: Disabling gamepad actions due to running on Android");
    m_disableGamepad = true;
#endif

    setupActions();

    // Hand Left
    XrPath handLeftXClick;                    // OCULUS_TOUCH
    XrPath handLeftXTouch;                    // OCULUS_TOUCH
    XrPath handLeftYClick;                    // OCULUS_TOUCH
    XrPath handLeftYTouch;                    // OCULUS_TOUCH
    XrPath handLeftAClick;                    // VALVE_INDEX
    XrPath handLeftATouch;                    // VALVE_INDEX
    XrPath handLeftBClick;                    // VALVE_INDEX
    XrPath handLeftBTouch;                    // VALVE_INDEX
    XrPath handLeftMenuClick;                 // OCULUS_TOUCH | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftSystemClick;               // VALVE_INDEX | HTC_VIVE
    XrPath handLeftSystemTouch;               // VALVE_INDEX
    XrPath handLeftSqueezeValue;              // OCULUS_TOUCH | VALVE_INDEX
    XrPath handLeftSqueezeForce;              // VALVE_INDEX
    XrPath handLeftSqueezeClick;              // MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftTriggerValue;              // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftTriggerTouch;              // OCULUS_TOUCH | VALVE_INDEX
    XrPath handLeftTriggerClick;              // VALVE_INDEX | HTC_VIVE
    XrPath handLeftThumbstickX;               // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM
    XrPath handLeftThumbstickY;               // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM
    XrPath handLeftThumbstickClick;           // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM
    XrPath handLeftThumbstickTouch;           // OCULUS_TOUCH | VALVE_INDEX
    XrPath handLeftThumbrestTouch;            // OCULUS_TOUCH
    XrPath handLeftTrackpadX;                 // VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftTrackpadY;                 // VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftTrackpadForce;             // VALVE_INDEX
    XrPath handLeftTrackpadTouch;             // VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftTrackpadClick;             // MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftGripPose;                  // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftAimPose;                   // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftHaptic;                    // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE

    // Hand Right
    XrPath handRightAClick;                   // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightATouch;                   // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightBClick;                   // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightBTouch;                   // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightSystemClick;              // OCULUS_TOUCH | VALVE_INDEX | HTC_VIVE
    XrPath handRightSystemTouch;              // VALVE_INDEX
    XrPath handRightMenuClick;                // MICROSOFT_MRM | HTC_VIVE
    XrPath handRightSqueezeValue;             // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightSqueezeForce;             // VALVE_INDEX
    XrPath handRightSqueezeClick;             // MICROSOFT_MRM | HTC_VIVE
    XrPath handRightTriggerValue;             // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightTriggerTouch;             // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightTriggerClick;             // VALVE_INDEX | HTC_VIVE
    XrPath handRightThumbstickX;              // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightThumbstickY;              // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightThumbstickClick;          // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightThumbstickTouch;          // OCULUS_TOUCH | VALVE_INDEX
    XrPath handRightThumbrestTouch;           // OCULUS_TOUCH
    XrPath handRightTrackpadX;                // VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightTrackpadY;                // VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightTrackpadForce;            // VALVE_INDEX
    XrPath handRightTrackpadTouch;            // VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightTrackpadClick;            // MICROSOFT_MRM | HTC_VIVE
    XrPath handRightGripPose;                 // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightAimPose;                  // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightHaptic;                   // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE

    // Gamepad
    XrPath gamepadMenuClick;
    XrPath gamepadViewClick;
    XrPath gamepadAClick;
    XrPath gamepadBClick;
    XrPath gamepadXClick;
    XrPath gamepadYClick;
    XrPath gamepadDpadDownClick;
    XrPath gamepadDpadRightClick;
    XrPath gamepadDpadUpClick;
    XrPath gamepadDpadLeftClick;
    XrPath gamepadShoulderLeftClick;
    XrPath gamepadShoulderRightClick;
    XrPath gamepadThumbstickLeftClick;
    XrPath gamepadThumbstickRightClick;
    XrPath gamepadTriggerLeftValue;
    XrPath gamepadTriggerRightValue;
    XrPath gamepadThumbstickLeftX;
    XrPath gamepadThumbstickLeftY;
    XrPath gamepadThumbstickRightX;
    XrPath gamepadThumbstickRightY;
    XrPath gamepadHapticLeft;
    XrPath gamepadHapticRight;
    XrPath gamepadHapticLeftTrigger;
    XrPath gamepadHapticRightTrigger;

    // Hand Left
    setPath(handLeftXClick, "/user/hand/left/input/x/click");
    setPath(handLeftXTouch, "/user/hand/left/input/x/touch");
    setPath(handLeftYClick, "/user/hand/left/input/y/click");
    setPath(handLeftYTouch, "/user/hand/left/input/y/touch");
    setPath(handLeftAClick, "/user/hand/left/input/a/click");
    setPath(handLeftATouch, "/user/hand/left/input/a/touch");
    setPath(handLeftBClick, "/user/hand/left/input/b/click");
    setPath(handLeftBTouch, "/user/hand/left/input/b/touch");
    setPath(handLeftMenuClick, "/user/hand/left/input/menu/click");
    setPath(handLeftSystemClick, "/user/hand/left/input/system/click");
    setPath(handLeftSystemTouch, "/user/hand/left/input/system/touch");
    setPath(handLeftSqueezeValue, "/user/hand/left/input/squeeze/value");
    setPath(handLeftSqueezeForce, "/user/hand/left/input/squeeze/force");
    setPath(handLeftSqueezeClick, "/user/hand/left/input/squeeze/click");
    setPath(handLeftTriggerValue, "/user/hand/left/input/trigger/value");
    setPath(handLeftTriggerTouch, "/user/hand/left/input/trigger/touch");
    setPath(handLeftTriggerClick, "/user/hand/left/input/trigger/click");
    setPath(handLeftThumbstickX, "/user/hand/left/input/thumbstick/x");
    setPath(handLeftThumbstickY, "/user/hand/left/input/thumbstick/y");
    setPath(handLeftThumbstickClick, "/user/hand/left/input/thumbstick/click");
    setPath(handLeftThumbstickTouch, "/user/hand/left/input/thumbstick/touch");
    setPath(handLeftThumbrestTouch, "/user/hand/left/input/thumbrest/touch");
    setPath(handLeftTrackpadX, "/user/hand/left/input/trackpad/x");
    setPath(handLeftTrackpadY, "/user/hand/left/input/trackpad/y");
    setPath(handLeftTrackpadForce, "/user/hand/left/input/trackpad/force");
    setPath(handLeftTrackpadTouch, "/user/hand/left/input/trackpad/touch");
    setPath(handLeftTrackpadClick, "/user/hand/left/input/trackpad/click");
    setPath(handLeftGripPose, "/user/hand/left/input/grip/pose");
    setPath(handLeftAimPose, "/user/hand/left/input/aim/pose");
    setPath(handLeftHaptic, "/user/hand/left/output/haptic");

    // Hand Right
    setPath(handRightAClick, "/user/hand/right/input/a/click");
    setPath(handRightATouch, "/user/hand/right/input/a/touch");
    setPath(handRightBClick, "/user/hand/right/input/b/click");
    setPath(handRightBTouch, "/user/hand/right/input/b/touch");
    setPath(handRightSystemClick, "/user/hand/right/input/system/click");
    setPath(handRightSystemTouch, "/user/hand/right/input/system/touch");
    setPath(handRightMenuClick, "/user/hand/right/input/menu/click");
    setPath(handRightSqueezeValue, "/user/hand/right/input/squeeze/value");
    setPath(handRightSqueezeForce, "/user/hand/right/input/squeeze/force");
    setPath(handRightSqueezeClick, "/user/hand/right/input/squeeze/click");
    setPath(handRightTriggerValue, "/user/hand/right/input/trigger/value");
    setPath(handRightTriggerTouch, "/user/hand/right/input/trigger/touch");
    setPath(handRightTriggerClick, "/user/hand/right/input/trigger/click");
    setPath(handRightThumbstickX, "/user/hand/right/input/thumbstick/x");
    setPath(handRightThumbstickY, "/user/hand/right/input/thumbstick/y");
    setPath(handRightThumbstickClick, "/user/hand/right/input/thumbstick/click");
    setPath(handRightThumbstickTouch, "/user/hand/right/input/thumbstick/touch");
    setPath(handRightThumbrestTouch, "/user/hand/right/input/thumbrest/touch");
    setPath(handRightTrackpadX, "/user/hand/right/input/trackpad/x");
    setPath(handRightTrackpadY, "/user/hand/right/input/trackpad/y");
    setPath(handRightTrackpadForce, "/user/hand/right/input/trackpad/force");
    setPath(handRightTrackpadTouch, "/user/hand/right/input/trackpad/touch");
    setPath(handRightTrackpadClick, "/user/hand/right/input/trackpad/click");
    setPath(handRightGripPose, "/user/hand/right/input/grip/pose");
    setPath(handRightAimPose, "/user/hand/right/input/aim/pose");
    setPath(handRightHaptic, "/user/hand/right/output/haptic");

    // Gamepad
    setPath(gamepadMenuClick, "/user/gamepad/input/menu/click");
    setPath(gamepadViewClick, "/user/gamepad/input/view/click");
    setPath(gamepadAClick, "/user/gamepad/input/a/click");
    setPath(gamepadBClick, "/user/gamepad/input/b/click");
    setPath(gamepadXClick, "/user/gamepad/input/x/click");
    setPath(gamepadYClick, "/user/gamepad/input/y/click");
    setPath(gamepadDpadDownClick, "/user/gamepad/input/dpad_down/click");
    setPath(gamepadDpadRightClick, "/user/gamepad/input/dpad_right/click");
    setPath(gamepadDpadUpClick, "/user/gamepad/input/dpad_up/click");
    setPath(gamepadDpadLeftClick, "/user/gamepad/input/dpad_left/click");
    setPath(gamepadShoulderLeftClick, "/user/gamepad/input/shoulder_left/click");
    setPath(gamepadShoulderRightClick, "/user/gamepad/input/shoulder_right/click");
    setPath(gamepadThumbstickLeftClick, "/user/gamepad/input/thumbstick_left/click");
    setPath(gamepadThumbstickRightClick, "/user/gamepad/input/thumbstick_right/click");
    setPath(gamepadTriggerLeftValue, "/user/gamepad/input/trigger_left/value");
    setPath(gamepadTriggerRightValue, "/user/gamepad/input/trigger_right/value");
    setPath(gamepadThumbstickLeftX, "/user/gamepad/input/thumbstick_left/x");
    setPath(gamepadThumbstickLeftY, "/user/gamepad/input/thumbstick_left/y");
    setPath(gamepadThumbstickRightX, "/user/gamepad/input/thumbstick_right/x");
    setPath(gamepadThumbstickRightY, "/user/gamepad/input/thumbstick_right/y");
    setPath(gamepadHapticLeft, "/user/gamepad/output/haptic_left");
    setPath(gamepadHapticRight, "/user/gamepad/output/haptic_right");
    setPath(gamepadHapticLeftTrigger, "/user/gamepad/output/haptic_left_trigger");
    setPath(gamepadHapticRightTrigger, "/user/gamepad/output/haptic_right_trigger");

    // Bindings

    // Oculus Touch
    {
        XrPath oculusTouchProfile;
        setPath(oculusTouchProfile, "/interaction_profiles/oculus/touch_controller");
        std::vector<XrActionSuggestedBinding> bindings {{
                {m_handActions.button1PressedAction, handLeftXClick},
                {m_handActions.button2PressedAction, handLeftYClick},
                {m_handActions.button1TouchedAction, handLeftXTouch},
                {m_handActions.button2TouchedAction, handLeftYTouch},
                {m_handActions.buttonMenuPressedAction, handLeftMenuClick},
                {m_handActions.squeezeValueAction, handLeftSqueezeValue},
                {m_handActions.triggerValueAction, handLeftTriggerValue},
                {m_handActions.triggerTouchedAction, handLeftTriggerTouch},
                {m_handActions.thumbstickXAction, handLeftThumbstickX},
                {m_handActions.thumbstickYAction, handLeftThumbstickY},
                {m_handActions.thumbstickPressedAction, handLeftThumbstickClick},
                {m_handActions.thumbstickTouchedAction, handLeftThumbstickTouch},
                {m_handActions.thumbrestTouchedAction, handLeftThumbrestTouch},
                {m_handActions.gripPoseAction, handLeftGripPose},
                {m_handActions.aimPoseAction, handLeftAimPose},
                {m_handActions.hapticAction, handLeftHaptic},
                {m_handActions.button1PressedAction, handRightAClick},
                {m_handActions.button2PressedAction, handRightBClick},
                {m_handActions.button1TouchedAction, handRightATouch},
                {m_handActions.button2TouchedAction, handRightBTouch},
                {m_handActions.buttonSystemPressedAction, handRightSystemClick},
                {m_handActions.squeezeValueAction, handRightSqueezeValue},
                {m_handActions.triggerValueAction, handRightTriggerValue},
                {m_handActions.triggerTouchedAction, handRightTriggerTouch},
                {m_handActions.thumbstickXAction, handRightThumbstickX},
                {m_handActions.thumbstickYAction, handRightThumbstickY},
                {m_handActions.thumbstickPressedAction, handRightThumbstickClick},
                {m_handActions.thumbstickTouchedAction, handRightThumbstickTouch},
                {m_handActions.thumbrestTouchedAction, handRightThumbrestTouch},
                {m_handActions.gripPoseAction, handRightGripPose},
                {m_handActions.aimPoseAction, handRightAimPose},
                {m_handActions.hapticAction, handRightHaptic},
                                                        }};
        XrInteractionProfileSuggestedBinding suggestedBindings{};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = oculusTouchProfile;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
    }

    // HTC Vive ### TODO
    {
        XrPath htcViveProfile;
        setPath(htcViveProfile, "/interaction_profiles/htc/vive_controller");


        std::vector<XrActionSuggestedBinding> bindings {{
                {m_handActions.buttonMenuPressedAction, handLeftMenuClick},
                {m_handActions.buttonSystemPressedAction, handLeftSystemClick },
                {m_handActions.squeezePressedAction, handLeftSqueezeClick},
                {m_handActions.triggerValueAction, handLeftTriggerValue},
                {m_handActions.triggerPressedAction, handLeftTriggerClick},
                {m_handActions.trackpadXAction, handLeftTrackpadX},
                {m_handActions.trackpadYAction, handLeftTrackpadY},
                {m_handActions.trackpadTouchedAction, handLeftTrackpadTouch},
                {m_handActions.trackpadPressedAction, handLeftTrackpadClick},
                {m_handActions.gripPoseAction, handLeftGripPose},
                {m_handActions.aimPoseAction, handLeftAimPose},
                {m_handActions.hapticAction, handLeftHaptic},

                {m_handActions.buttonMenuPressedAction, handRightMenuClick},
                {m_handActions.buttonSystemPressedAction, handRightSystemClick },
                {m_handActions.squeezePressedAction, handRightSqueezeClick},
                {m_handActions.triggerValueAction, handRightTriggerValue},
                {m_handActions.triggerPressedAction, handRightTriggerClick},
                {m_handActions.trackpadXAction, handRightTrackpadX},
                {m_handActions.trackpadYAction, handRightTrackpadY},
                {m_handActions.trackpadTouchedAction, handRightTrackpadTouch},
                {m_handActions.trackpadPressedAction, handRightTrackpadClick},
                {m_handActions.gripPoseAction, handRightGripPose},
                {m_handActions.aimPoseAction, handRightAimPose},
                {m_handActions.hapticAction, handRightHaptic},


            }};
        XrInteractionProfileSuggestedBinding suggestedBindings{};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = htcViveProfile;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
    }

    // Microsoft MRM ### TODO
    {
        XrPath microsoftMotionProfile;
        setPath(microsoftMotionProfile, "/interaction_profiles/microsoft/motion_controller");
    }

    // Valve Index ### TODO
    {
        XrPath valveIndexProfile;
        setPath(valveIndexProfile, "/interaction_profiles/valve/index_controller");
    }

    // XBox Controller
    {
        XrPath xboxControllerProfile;
        setPath(xboxControllerProfile, "/interaction_profiles/microsoft/xbox_controller");
        std::vector<XrActionSuggestedBinding> bindings {{
                {m_gamepadActions.buttonMenuPressedAction, gamepadMenuClick},
                {m_gamepadActions.buttonViewPressedAction, gamepadViewClick},
                {m_gamepadActions.buttonAPressedAction, gamepadAClick},
                {m_gamepadActions.buttonBPressedAction, gamepadBClick},
                {m_gamepadActions.buttonXPressedAction, gamepadXClick},
                {m_gamepadActions.buttonYPressedAction, gamepadYClick},
                {m_gamepadActions.buttonDownPressedAction, gamepadDpadDownClick},
                {m_gamepadActions.buttonRightPressedAction, gamepadDpadRightClick},
                {m_gamepadActions.buttonUpPressedAction, gamepadDpadUpClick},
                {m_gamepadActions.buttonLeftPressedAction, gamepadDpadLeftClick},
                {m_gamepadActions.shoulderLeftPressedAction, gamepadShoulderLeftClick},
                {m_gamepadActions.shoulderRightPressedAction, gamepadShoulderRightClick},
                {m_gamepadActions.thumbstickLeftPressedAction, gamepadThumbstickLeftClick},
                {m_gamepadActions.thumbstickRightPressedAction, gamepadThumbstickRightClick},
                {m_gamepadActions.triggerLeftAction, gamepadTriggerLeftValue},
                {m_gamepadActions.triggerRightAction, gamepadTriggerRightValue},
                {m_gamepadActions.thumbstickLeftXAction, gamepadThumbstickLeftX},
                {m_gamepadActions.thumbstickLeftYAction, gamepadThumbstickLeftY},
                {m_gamepadActions.thumbstickRightXAction, gamepadThumbstickRightX},
                {m_gamepadActions.thumbstickRightYAction, gamepadThumbstickRightY},
                {m_gamepadActions.hapticLeftAction, gamepadHapticLeft},
                {m_gamepadActions.hapticRightAction, gamepadHapticRight},
                {m_gamepadActions.hapticLeftTriggerAction, gamepadHapticLeftTrigger},
                {m_gamepadActions.hapticRightTriggerAction, gamepadHapticRightTrigger},
                                                        }};
        XrInteractionProfileSuggestedBinding suggestedBindings{};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = xboxControllerProfile;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
    }

    // Setup Action Spaces

    XrActionSpaceCreateInfo actionSpaceInfo{};
    actionSpaceInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
    actionSpaceInfo.action = m_handActions.gripPoseAction;
    actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
    //actionSpaceInfo.poseInActionSpace.orientation.y = 1.0f;
    actionSpaceInfo.subactionPath = m_handSubactionPath[0];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handGripSpace[0]));
    actionSpaceInfo.subactionPath = m_handSubactionPath[1];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handGripSpace[1]));

    actionSpaceInfo.action = m_handActions.aimPoseAction;
    actionSpaceInfo.subactionPath = m_handSubactionPath[0];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handAimSpace[0]));
    actionSpaceInfo.subactionPath = m_handSubactionPath[1];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handAimSpace[1]));


    // Attach Action set to session

    XrSessionActionSetsAttachInfo attachInfo{};
    attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    checkXrResult(xrAttachSessionActionSets(m_session, &attachInfo));

    m_initialized = true;
}

void QOpenXRInputManager::teardown()
{
    if (!m_initialized)
        return;

    m_initialized = false;

    if (m_actionSet != XR_NULL_HANDLE) {
        xrDestroySpace(m_handGripSpace[0]);
        xrDestroySpace(m_handGripSpace[1]);
        xrDestroySpace(m_handAimSpace[0]);
        xrDestroySpace(m_handAimSpace[1]);
        xrDestroyActionSet(m_actionSet);
    }

    m_instance = {XR_NULL_HANDLE};
    m_session = {XR_NULL_HANDLE};
}

void QOpenXRInputManager::pollActions()
{
    if (!m_initialized)
        return;

    // Sync Actions
    const XrActiveActionSet activeActionSet{m_actionSet, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{};
    syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &activeActionSet;
    XrResult result = xrSyncActions(m_session, &syncInfo);
    if (!(result == XR_SUCCESS ||
          result == XR_SESSION_LOSS_PENDING ||
          result == XR_SESSION_NOT_FOCUSED)) {
        checkXrResult(result);
        return;
    }

    using namespace std::placeholders;

    // Hands
    XrActionStateGetInfo getInfo{};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    for (int i = 0; i < 2; ++i) {

        getInfo.subactionPath = m_handSubactionPath[i];
        auto &inputState = m_handInputState[i];

        getBoolInputState(getInfo, m_handActions.button1PressedAction, std::bind(&QOpenXRHandInput::setButton1Pressed, inputState, _1));
        getBoolInputState(getInfo, m_handActions.button1TouchedAction, std::bind(&QOpenXRHandInput::setButton1Touched, inputState, _1));
        getBoolInputState(getInfo, m_handActions.button2PressedAction, std::bind(&QOpenXRHandInput::setButton2Pressed, inputState, _1));
        getBoolInputState(getInfo, m_handActions.button2TouchedAction, std::bind(&QOpenXRHandInput::setButton2Touched, inputState, _1));
        getBoolInputState(getInfo, m_handActions.buttonMenuPressedAction, std::bind(&QOpenXRHandInput::setButtonMenuPressed, inputState, _1));
        getBoolInputState(getInfo, m_handActions.buttonMenuTouchedAction, std::bind(&QOpenXRHandInput::setButtonMenuTouched, inputState, _1));
        getBoolInputState(getInfo, m_handActions.buttonSystemPressedAction, std::bind(&QOpenXRHandInput::setButtonSystemPressed, inputState, _1));
        getBoolInputState(getInfo, m_handActions.buttonSystemTouchedAction, std::bind(&QOpenXRHandInput::setButtonSystemTouched, inputState, _1));
        getFloatInputState(getInfo, m_handActions.squeezeValueAction, std::bind(&QOpenXRHandInput::setSqueezeValue, inputState, _1));
        getFloatInputState(getInfo, m_handActions.squeezeForceAction, std::bind(&QOpenXRHandInput::setSqueezeForce, inputState, _1));
        getBoolInputState(getInfo, m_handActions.squeezePressedAction, std::bind(&QOpenXRHandInput::setSqueezePressed, inputState, _1));
        getFloatInputState(getInfo, m_handActions.triggerValueAction, std::bind(&QOpenXRHandInput::setTriggerValue, inputState, _1));
        getBoolInputState(getInfo, m_handActions.triggerPressedAction, std::bind(&QOpenXRHandInput::setTriggerPressed, inputState, _1));
        getBoolInputState(getInfo, m_handActions.triggerTouchedAction, std::bind(&QOpenXRHandInput::setTriggerTouched, inputState, _1));
        getFloatInputState(getInfo, m_handActions.thumbstickXAction, std::bind(&QOpenXRHandInput::setThumbstickX, inputState, _1));
        getFloatInputState(getInfo, m_handActions.thumbstickYAction, std::bind(&QOpenXRHandInput::setThumbstickY, inputState, _1));
        getBoolInputState(getInfo, m_handActions.thumbstickPressedAction, std::bind(&QOpenXRHandInput::setThumbstickPressed, inputState, _1));
        getBoolInputState(getInfo, m_handActions.thumbstickTouchedAction, std::bind(&QOpenXRHandInput::setThumbstickTouched, inputState, _1));
        getBoolInputState(getInfo, m_handActions.thumbrestTouchedAction, std::bind(&QOpenXRHandInput::setThumbrestTouched, inputState, _1));
        getFloatInputState(getInfo, m_handActions.trackpadXAction, std::bind(&QOpenXRHandInput::setTrackpadX, inputState, _1));
        getFloatInputState(getInfo, m_handActions.trackpadYAction, std::bind(&QOpenXRHandInput::setTrackpadY, inputState, _1));
        getFloatInputState(getInfo, m_handActions.trackpadForceAction, std::bind(&QOpenXRHandInput::setTrackpadForce, inputState, _1));
        getBoolInputState(getInfo, m_handActions.trackpadTouchedAction, std::bind(&QOpenXRHandInput::setTrackpadTouched, inputState, _1));
        getBoolInputState(getInfo, m_handActions.trackpadPressedAction, std::bind(&QOpenXRHandInput::setTrackpadPressed, inputState, _1));
        // Get pose activity status
        getInfo.action = m_handActions.gripPoseAction;
        XrActionStatePose poseState{};
        poseState.type = XR_TYPE_ACTION_STATE_POSE;
        checkXrResult(xrGetActionStatePose(m_session, &getInfo, &poseState));
        inputState->setIsActive(poseState.isActive);

        // TODO handle any output as well here (haptics)
    //    XrAction gripPoseAction{XR_NULL_HANDLE};
    //    XrAction aimPoseAction{XR_NULL_HANDLE};
    //    XrAction hapticAction{XR_NULL_HANDLE};

    }

    // Gamepad
    if (!m_disableGamepad) {
        getInfo.subactionPath = m_gamepadSubactionPath;
        getBoolInputState(getInfo, m_gamepadActions.buttonMenuPressedAction, std::bind(&QOpenXRGamepadInput::setButtonMenu, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonViewPressedAction, std::bind(&QOpenXRGamepadInput::setButtonView, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonAPressedAction, std::bind(&QOpenXRGamepadInput::setButtonA, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonBPressedAction, std::bind(&QOpenXRGamepadInput::setButtonB, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonXPressedAction, std::bind(&QOpenXRGamepadInput::setButtonX, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonYPressedAction, std::bind(&QOpenXRGamepadInput::setButtonY, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonDownPressedAction, std::bind(&QOpenXRGamepadInput::setDpadDown, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonRightPressedAction, std::bind(&QOpenXRGamepadInput::setDpadRight, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonUpPressedAction, std::bind(&QOpenXRGamepadInput::setDpadUp, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.buttonLeftPressedAction, std::bind(&QOpenXRGamepadInput::setDpadLeft, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.shoulderLeftPressedAction, std::bind(&QOpenXRGamepadInput::setShoulderLeft, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.shoulderRightPressedAction, std::bind(&QOpenXRGamepadInput::setShoulderRight, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.thumbstickLeftPressedAction, std::bind(&QOpenXRGamepadInput::setButtonThumbstickLeft, m_gamepadInputState, _1));
        getBoolInputState(getInfo, m_gamepadActions.thumbstickRightPressedAction, std::bind(&QOpenXRGamepadInput::setButtonThumbstickRight, m_gamepadInputState, _1));
        getFloatInputState(getInfo, m_gamepadActions.triggerLeftAction, std::bind(&QOpenXRGamepadInput::setTriggerLeft, m_gamepadInputState, _1));
        getFloatInputState(getInfo, m_gamepadActions.triggerRightAction, std::bind(&QOpenXRGamepadInput::setTriggerRight, m_gamepadInputState, _1));
        getFloatInputState(getInfo, m_gamepadActions.thumbstickLeftXAction, std::bind(&QOpenXRGamepadInput::setThumbstickLeftX, m_gamepadInputState, _1));
        getFloatInputState(getInfo, m_gamepadActions.thumbstickLeftYAction, std::bind(&QOpenXRGamepadInput::setThumbstickLeftY, m_gamepadInputState, _1));
        getFloatInputState(getInfo, m_gamepadActions.thumbstickRightXAction, std::bind(&QOpenXRGamepadInput::setThumbstickRightX, m_gamepadInputState, _1));
        getFloatInputState(getInfo, m_gamepadActions.thumbstickRightYAction, std::bind(&QOpenXRGamepadInput::setThumbstickRightY, m_gamepadInputState, _1));

        // TODO handle any outputs as well here (haptics)
//        XrAction hapticLeftAction{XR_NULL_HANDLE};
//        XrAction hapticRightAction{XR_NULL_HANDLE};
//        XrAction hapticLeftTriggerAction{XR_NULL_HANDLE};
//        XrAction hapticRightTriggerAction{XR_NULL_HANDLE};
    }
}

void QOpenXRInputManager::updatePoses(XrTime predictedDisplayTime, XrSpace appSpace)
{
    // Update the Hands pose
    for (auto hand : {QOpenXRInputManager::LeftHand, QOpenXRInputManager::RightHand}) {
        XrSpaceLocation spaceLocation{};
        spaceLocation.type = XR_TYPE_SPACE_LOCATION;
        XrResult res;
        res = xrLocateSpace(handSpace(hand), appSpace, predictedDisplayTime, &spaceLocation);
        checkXrResult(res);
        if (XR_UNQUALIFIED_SUCCESS(res)) {
            if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {

                // Update hand transform
                setPosePosition(hand, QVector3D(spaceLocation.pose.position.x,
                                                                spaceLocation.pose.position.y,
                                                                spaceLocation.pose.position.z) * 100.0f);
                setPoseRotation(hand, QQuaternion(spaceLocation.pose.orientation.w,
                                                                  spaceLocation.pose.orientation.x,
                                                                  spaceLocation.pose.orientation.y,
                                                                  spaceLocation.pose.orientation.z));
            }
        } else {
            // Tracking loss is expected when the hand is not active so only log a message
            // if the hand is active.
            if (isHandActive(hand)) {
                const char* handName[] = {"left", "right"};
                qDebug("Unable to locate %s hand action space in app space: %d", handName[hand], res);
            }
        }
    }
}

void QOpenXRInputManager::setupActions()
{
    // Create an action set.
    {
        XrActionSetCreateInfo actionSetInfo{};
        actionSetInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
        strcpy(actionSetInfo.actionSetName, "gameplay");
        strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
        actionSetInfo.priority = 0;
        checkXrResult(xrCreateActionSet(m_instance, &actionSetInfo, &m_actionSet));
    }

    // Create Hand Actions
    setPath(m_handSubactionPath[0], "/user/hand/left");
    setPath(m_handSubactionPath[1], "/user/hand/right");
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "b1_pressed",
                 "Button 1 Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.button1PressedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "b1_touched",
                 "Button 1 Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.button1TouchedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "b2_pressed",
                 "Button 2 Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.button2PressedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "b2_touched",
                 "Button 2 Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.button2TouchedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "bmenu_pressed",
                 "Button Menu Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.buttonMenuPressedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "bmenu_touched",
                 "Button Menu Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.buttonMenuTouchedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "bsystem_pressed",
                 "Button System Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.buttonSystemPressedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "bsystem_touched",
                 "Button System Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.buttonSystemTouchedAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "squeeze_value",
                 "Squeeze Value",
                 2,
                 m_handSubactionPath,
                 m_handActions.squeezeValueAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "squeeze_force",
                 "Squeeze Force",
                 2,
                 m_handSubactionPath,
                 m_handActions.squeezeForceAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "squeeze_pressed",
                 "Squeeze Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.squeezePressedAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "trigger_value",
                 "Trigger Value",
                 2,
                 m_handSubactionPath,
                 m_handActions.triggerValueAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "trigger_pressed",
                 "Trigger Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.triggerPressedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "trigger_touched",
                 "Trigger Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.triggerTouchedAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "thumbstick_x",
                 "Thumbstick X",
                 2,
                 m_handSubactionPath,
                 m_handActions.thumbstickXAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "thumbstick_y",
                 "Thumbstick Y",
                 2,
                 m_handSubactionPath,
                 m_handActions.thumbstickYAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "thumbstick_pressed",
                 "Thumbstick Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.thumbstickPressedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "thumbstick_touched",
                 "Thumbstick Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.thumbstickTouchedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "thumbrest_touched",
                 "Thumbrest Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.thumbrestTouchedAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "trackpad_x",
                 "Trackpad X",
                 2,
                 m_handSubactionPath,
                 m_handActions.trackpadXAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "trackpad_y",
                 "Trackpad Y",
                 2,
                 m_handSubactionPath,
                 m_handActions.trackpadYAction);
    createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                 "trackpad_force",
                 "Trackpad Force",
                 2,
                 m_handSubactionPath,
                 m_handActions.trackpadForceAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "trackpad_touched",
                 "Trackpad Touched",
                 2,
                 m_handSubactionPath,
                 m_handActions.trackpadTouchedAction);
    createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                 "trackpad_pressed",
                 "Trackpad Pressed",
                 2,
                 m_handSubactionPath,
                 m_handActions.trackpadPressedAction);
    createAction(XR_ACTION_TYPE_VIBRATION_OUTPUT,
                 "vibrate_hand",
                 "Vibrate Hand",
                 2,
                 m_handSubactionPath,
                 m_handActions.hapticAction);
    createAction(XR_ACTION_TYPE_POSE_INPUT,
                 "hand_grip_pose",
                 "Hand Grip Pose",
                 2,
                 m_handSubactionPath,
                 m_handActions.gripPoseAction);
    createAction(XR_ACTION_TYPE_POSE_INPUT,
                 "hand_aim_pose",
                 "Hand Aim Pose",
                 2,
                 m_handSubactionPath,
                 m_handActions.aimPoseAction);

    // Create Gamepad Actions
    if (!m_disableGamepad) {
        setPath(m_gamepadSubactionPath, "/user/gamepad");
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bmenu_pressed",
                     "Gamepad Button Menu Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonMenuPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bview_pressed",
                     "Gamepad Button View Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonViewPressedAction);

        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_ba_pressed",
                     "Gamepad Button A Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonAPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bb_pressed",
                     "Gamepad Button B Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonBPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bx_pressed",
                     "Gamepad Button X Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonXPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_by_pressed",
                     "Gamepad Button Y Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonYPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bdown_pressed",
                     "Gamepad Button Down Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonDownPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bright_pressed",
                     "Gamepad Button Right Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonRightPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bup_pressed",
                     "Gamepad Button Up Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonUpPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_bleft_pressed",
                     "Gamepad Button Left Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.buttonLeftPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_sleft_pressed",
                     "Gamepad Shoulder Left Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.shoulderLeftPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_sright_pressed",
                     "Gamepad Shoulder Right Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.shoulderRightPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_tsleft_pressed",
                     "Gamepad Thumbstick Left Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.thumbstickLeftPressedAction);
        createAction(XR_ACTION_TYPE_BOOLEAN_INPUT,
                     "gp_tsright_pressed",
                     "Gamepad Thumbstick Right Pressed",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.thumbstickRightPressedAction);
        createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                     "gp_tleft_value",
                     "Gamepad Trigger Left",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.triggerLeftAction);
        createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                     "gp_tright_value",
                     "Gamepad Trigger Right",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.triggerRightAction);
        createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                     "gp_tsleft_x_value",
                     "Gamepad Thumbstick Left X",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.thumbstickLeftXAction);
        createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                     "gp_tsleft_y_value",
                     "Gamepad Thumbstick Left Y",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.thumbstickLeftYAction);
        createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                     "gp_tsright_x_value",
                     "Gamepad Thumbstick Right X",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.thumbstickRightXAction);
        createAction(XR_ACTION_TYPE_FLOAT_INPUT,
                     "gp_tsright_y_value",
                     "Gamepad Thumbstick Right Y",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.thumbstickRightYAction);
        createAction(XR_ACTION_TYPE_VIBRATION_OUTPUT,
                     "gp_vibrate_left",
                     "Gamepad Vibrate Left",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.hapticLeftAction);
        createAction(XR_ACTION_TYPE_VIBRATION_OUTPUT,
                     "gp_vibrate_right",
                     "Gamepad Vibrate Right",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.hapticRightAction);
        createAction(XR_ACTION_TYPE_VIBRATION_OUTPUT,
                     "gp_vibrate_trigger_left",
                     "Gamepad Vibrate Trigger Left",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.hapticLeftTriggerAction);
        createAction(XR_ACTION_TYPE_VIBRATION_OUTPUT,
                     "gp_vibrate_trigger_right",
                     "Gamepad Vibrate Trigger Right",
                     1,
                     &m_gamepadSubactionPath,
                     m_gamepadActions.hapticRightTriggerAction);
    }
}

bool QOpenXRInputManager::checkXrResult(const XrResult &result)
{
    bool checkResult = OpenXRHelpers::checkXrResult(result, m_instance);
    if (!checkResult) {
        qDebug("here");
    }
    return checkResult;

}

void QOpenXRInputManager::setPath(XrPath &path, const char *pathString)
{
    checkXrResult(xrStringToPath(m_instance, pathString, &path));
}

void QOpenXRInputManager::createAction(XrActionType type,
                                       const char *name,
                                       const char *localizedName,
                                       int numSubactions,
                                       XrPath *subactionPath,
                                       XrAction &action)
{
    XrActionCreateInfo actionInfo{};
    actionInfo.type = XR_TYPE_ACTION_CREATE_INFO;
    actionInfo.actionType = type;
    strcpy(actionInfo.actionName, name);
    strcpy(actionInfo.localizedActionName, localizedName);
    actionInfo.countSubactionPaths = quint32(numSubactions);
    actionInfo.subactionPaths = subactionPath;
    checkXrResult(xrCreateAction(m_actionSet, &actionInfo, &action));
}

void QOpenXRInputManager::getBoolInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(bool)> setter)
{
    getInfo.action = action;
    XrActionStateBoolean boolValue{};
    boolValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;
    checkXrResult(xrGetActionStateBoolean(m_session, &getInfo, &boolValue));
    if (boolValue.isActive == XR_TRUE)
        setter(bool(boolValue.currentState));
}

void QOpenXRInputManager::getFloatInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(float)> setter)
{
    getInfo.action = action;
    XrActionStateFloat floatValue{};
    floatValue.type = XR_TYPE_ACTION_STATE_FLOAT;
    checkXrResult(xrGetActionStateFloat(m_session, &getInfo, &floatValue));
    if (floatValue.isActive == XR_TRUE)
        setter(float(floatValue.currentState));
}

XrSpace QOpenXRInputManager::handSpace(QOpenXRInputManager::Hand hand)
{
    if (m_handInputState[hand]->poseSpace() == QOpenXRHandInput::HandPoseSpace::GripPose)
        return m_handGripSpace[hand];
    else
        return m_handAimSpace[hand];
}

bool QOpenXRInputManager::isHandActive(QOpenXRInputManager::Hand hand)
{
    return m_handInputState[hand]->isActive();
}

void QOpenXRInputManager::setPosePosition(Hand hand, const QVector3D &position)
{
    m_handInputState[hand]->setPosePosition(position);
}

void QOpenXRInputManager::setPoseRotation(Hand hand, const QQuaternion &rotation)
{
    m_handInputState[hand]->setPoseRotation(rotation);
}

QOpenXRHandInput *QOpenXRInputManager::leftHandInput() const
{
    return m_handInputState[QOpenXRInputManager::LeftHand];
}

QOpenXRHandInput *QOpenXRInputManager::rightHandInput() const
{
    return m_handInputState[QOpenXRInputManager::RightHand];
}

QOpenXRGamepadInput *QOpenXRInputManager::gamepadInput() const
{
    return m_gamepadInputState;
}

QT_END_NAMESPACE
