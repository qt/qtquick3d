// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrinputmanager_p.h"
#include "qopenxrhelpers_p.h"
#include "qopenxrhandinput_p.h"
#include "qopenxrhandtrackerinput_p.h"
#include "qopenxrgamepadinput_p.h"

#include "qopenxrcontroller_p.h" //### InputAction enum

#include <QDebug>

QT_BEGIN_NAMESPACE

QOpenXRInputManager::QOpenXRInputManager()
{
    m_handInputState[QOpenXRInputManager::LeftHand] = new QOpenXRHandInput(this);
    m_handInputState[QOpenXRInputManager::RightHand] = new QOpenXRHandInput(this);
    m_handTrackerInputState[QOpenXRInputManager::LeftHand] = new QOpenXRHandTrackerInput(this);
    m_handTrackerInputState[QOpenXRInputManager::RightHand] = new QOpenXRHandTrackerInput(this);
    m_gamepadInputState = new QOpenXRGamepadInput(this);
}

QOpenXRInputManager::~QOpenXRInputManager()
{
    teardown();
    delete m_handInputState[QOpenXRInputManager::LeftHand];
    delete m_handInputState[QOpenXRInputManager::RightHand];
    delete m_handTrackerInputState[QOpenXRInputManager::LeftHand];
    delete m_handTrackerInputState[QOpenXRInputManager::RightHand];
    delete m_gamepadInputState;

    m_handInputState[QOpenXRInputManager::LeftHand] = nullptr;
    m_handInputState[QOpenXRInputManager::RightHand] = nullptr;
    m_handTrackerInputState[QOpenXRInputManager::LeftHand] = nullptr;
    m_handTrackerInputState[QOpenXRInputManager::RightHand] = nullptr;
    m_gamepadInputState = nullptr;
}

QOpenXRInputManager *QOpenXRInputManager::instance()
{
    static QOpenXRInputManager instance;
    return &instance;
}

QOpenXRInputManager::QXRHandComponentPath QOpenXRInputManager::makeQXRPath(const QByteArrayView path)
{
    QXRHandComponentPath res;
    setPath(res.paths[QOpenXRInputManager::LeftHand], "/user/hand/left/" + path);
    setPath(res.paths[QOpenXRInputManager::RightHand], "/user/hand/right/" + path);
    return res;
}

void QOpenXRInputManager::init(XrInstance instance, XrSession session)
{
    if (m_initialized) {
        qWarning() << "QOpenXRInputManager: Trying to initialize an already initialized session";
        teardown();
    }

    m_instance = instance;
    m_session = session;

    m_disableGamepad = false;

    setupHandTracking();

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

    // Also on Android. ### Why?
#ifdef XR_USE_PLATFORM_ANDROID
    qDebug("QOpenXRInputManager: Disabling gamepad actions due to running on Android");
    m_disableGamepad = true;
#endif

    setupActions();

    QXRHandComponentPath aClick = makeQXRPath("input/a/click"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)
    QXRHandComponentPath bClick = makeQXRPath("input/b/click"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)
    QXRHandComponentPath aTouch = makeQXRPath("input/a/touch"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)
    QXRHandComponentPath bTouch = makeQXRPath("input/b/touch"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)

    QXRHandComponentPath xClick = makeQXRPath("input/x/click"); // OCULUS_TOUCH (left)
    QXRHandComponentPath yClick = makeQXRPath("input/y/click"); // OCULUS_TOUCH (left)
    QXRHandComponentPath xTouch = makeQXRPath("input/x/touch"); // OCULUS_TOUCH (left)
    QXRHandComponentPath yTouch = makeQXRPath("input/y/touch"); // OCULUS_TOUCH (left)

    QXRHandComponentPath menuClick = makeQXRPath("input/menu/click"); // OCULUS_TOUCH (left) | MICROSOFT_MRM (right + left) | HTC_VIVE (right + left)
    QXRHandComponentPath systemClick = makeQXRPath("input/system/click"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left) | HTC_VIVE (right + left)
    QXRHandComponentPath systemTouch = makeQXRPath("input/system/touch"); // VALVE_INDEX (right + left)

    QXRHandComponentPath squeezeValue = makeQXRPath("input/squeeze/value"); // right + left: OCULUS_TOUCH | VALVE_INDEX
    QXRHandComponentPath squeezeForce = makeQXRPath("input/squeeze/force"); // right + left: VALVE_INDEX
    QXRHandComponentPath squeezeClick = makeQXRPath("input/squeeze/click"); // right + left: MICROSOFT_MRM | HTC_VIVE

    QXRHandComponentPath triggerValue = makeQXRPath("input/trigger/value"); // right + left: OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath triggerTouch = makeQXRPath("input/trigger/touch"); // right + left: OCULUS_TOUCH | VALVE_INDEX
    QXRHandComponentPath triggerClick = makeQXRPath("input/trigger/click"); // right + left: VALVE_INDEX | HTC_VIVE

    QXRHandComponentPath thumbstickX = makeQXRPath("input/thumbstick/x"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left) | MICROSOFT_MRM (left)
    QXRHandComponentPath thumbstickY = makeQXRPath("input/thumbstick/y"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left) | MICROSOFT_MRM (left)
    QXRHandComponentPath thumbstickClick = makeQXRPath("input/thumbstick/click"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left) | MICROSOFT_MRM (left)
    QXRHandComponentPath thumbstickTouch = makeQXRPath("input/thumbstick/touch"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left)
    QXRHandComponentPath thumbrestTouch = makeQXRPath("input/thumbrest/touch"); // OCULUS_TOUCH (right + left)

    QXRHandComponentPath trackpadX = makeQXRPath("input/trackpad/x"); // right + left:  VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath trackpadY = makeQXRPath("input/trackpad/y"); // right + left:  VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath trackpadForce = makeQXRPath("input/trackpad/force"); // right + left:  VALVE_INDEX
    QXRHandComponentPath trackpadClick = makeQXRPath("input/trackpad/click"); // right + left:  VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath trackpadTouch = makeQXRPath("input/trackpad/touch"); // right + left:  MICROSOFT_MRM | HTC_VIVE

    XrPath handLeftGripPose;                  // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftAimPose;                   // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftHaptic;                    // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE

    XrPath handRightGripPose;                 // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightAimPose;                  // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightHaptic;                   // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE

    // Hand interaction extension (TODO)
    // XrPath handLeftSelectValue;
    // XrPath handLeftPinch;
    // XrPath handLeftPoke;
    // XrPath handRightSelectValue;
    // XrPath handRightPinch;
    // XrPath handRightPoke;

    // Gamepad ### TODO
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

    setPath(handLeftGripPose, "/user/hand/left/input/grip/pose");
    setPath(handLeftAimPose, "/user/hand/left/input/aim/pose");
    setPath(handLeftHaptic, "/user/hand/left/output/haptic");

    setPath(handRightGripPose, "/user/hand/right/input/grip/pose");
    setPath(handRightAimPose, "/user/hand/right/input/aim/pose");
    setPath(handRightHaptic, "/user/hand/right/output/haptic");

    // Bindings


    using XrActionBindings = std::vector<XrActionSuggestedBinding>;
    using InputMapping = std::vector<std::tuple<QOpenXRActionMapper::InputAction, QXRHandComponentPath, SubPathSelector>>;
    auto addToBindings = [this](XrActionBindings &bindings, const InputMapping &defs){
        for (const auto &[actionId, path, selector] : defs) {
            if (selector & LeftHandSubPath)
                bindings.push_back({ m_handActions.actions[actionId], path.paths[LeftHand] });
            if (selector & RightHandSubPath)
                bindings.push_back({ m_handActions.actions[actionId], path.paths[RightHand] });
        }
    };

    // Oculus Touch
    {
        InputMapping mappingDefs {
            { QOpenXRActionMapper::Button1Pressed, xClick, LeftHandSubPath },
            { QOpenXRActionMapper::Button1Pressed, aClick, RightHandSubPath },
            { QOpenXRActionMapper::Button2Pressed, yClick, LeftHandSubPath },
            { QOpenXRActionMapper::Button2Pressed, bClick, RightHandSubPath },
            { QOpenXRActionMapper::Button1Touched, xTouch, LeftHandSubPath },
            { QOpenXRActionMapper::Button1Touched, aTouch, RightHandSubPath },
            { QOpenXRActionMapper::Button2Touched, yTouch, LeftHandSubPath },
            { QOpenXRActionMapper::Button2Touched, bTouch, RightHandSubPath },
            { QOpenXRActionMapper::ButtonMenuPressed, menuClick, LeftHandSubPath },
            { QOpenXRActionMapper::ButtonSystemPressed, systemClick, RightHandSubPath },
            { QOpenXRActionMapper::SqueezeValue, squeezeValue, BothHandsSubPath },
            { QOpenXRActionMapper::TriggerValue, triggerValue, BothHandsSubPath },
            { QOpenXRActionMapper::TriggerTouched, triggerTouch, BothHandsSubPath },
            { QOpenXRActionMapper::ThumbstickX, thumbstickX, BothHandsSubPath },
            { QOpenXRActionMapper::ThumbstickY, thumbstickY, BothHandsSubPath },
            { QOpenXRActionMapper::ThumbstickPressed, thumbstickClick, BothHandsSubPath },
            { QOpenXRActionMapper::ThumbstickTouched, thumbstickTouch, BothHandsSubPath },
            { QOpenXRActionMapper::ThumbrestTouched, thumbrestTouch, BothHandsSubPath },
        };

        XrPath oculusTouchProfile;
        setPath(oculusTouchProfile, "/interaction_profiles/oculus/touch_controller");
        std::vector<XrActionSuggestedBinding> bindings {{
                {m_handActions.gripPoseAction, handLeftGripPose},
                {m_handActions.aimPoseAction, handLeftAimPose},
                {m_handActions.hapticAction, handLeftHaptic},

                {m_handActions.gripPoseAction, handRightGripPose},
                {m_handActions.aimPoseAction, handRightAimPose},
                {m_handActions.hapticAction, handRightHaptic},
                                                        }};

        addToBindings(bindings, mappingDefs);

        XrInteractionProfileSuggestedBinding suggestedBindings{};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = oculusTouchProfile;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings), "suggested bindings: Oculus touch");
    }

    // Microsoft hand interaction extension as supported by Quest 3
    // TODO: there are other, very similar, extensions: XR_HTC_HAND_INTERACTION_EXTENSION_NAME and XR_EXT_HAND_INTERACTION_EXTENSION_NAME
    {
        XrPath handInteractionProfile;
        setPath(handInteractionProfile, "/interaction_profiles/microsoft/hand_interaction");
        std::vector<XrActionSuggestedBinding> bindings {{
                {m_handActions.gripPoseAction, handLeftGripPose},
                {m_handActions.aimPoseAction, handLeftAimPose}, // ### Binding succeeds, but does not seem to work on the Quest 3
                {m_handActions.gripPoseAction, handRightGripPose},
                {m_handActions.aimPoseAction, handRightAimPose},
        }};

        InputMapping mappingDefs {
            { QOpenXRActionMapper::SqueezeValue, squeezeValue, BothHandsSubPath },
        };

        addToBindings(bindings, mappingDefs);

        XrInteractionProfileSuggestedBinding suggestedBindings{};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = handInteractionProfile;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();

        checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings), "suggested bindings: MSFT hand interaction");
    }

    {
        XrPath htcViveProfile;
        setPath(htcViveProfile, "/interaction_profiles/htc/vive_controller");

        InputMapping mappingDefs {
            { QOpenXRActionMapper::ButtonMenuPressed, menuClick, BothHandsSubPath },
            { QOpenXRActionMapper::ButtonSystemPressed, systemClick, BothHandsSubPath },
            { QOpenXRActionMapper::SqueezePressed, squeezeClick, BothHandsSubPath },
            { QOpenXRActionMapper::TriggerValue, triggerValue, BothHandsSubPath },
            { QOpenXRActionMapper::TriggerPressed, triggerClick, BothHandsSubPath },
            { QOpenXRActionMapper::TrackpadX, trackpadX, BothHandsSubPath },
            { QOpenXRActionMapper::TrackpadY, trackpadY, BothHandsSubPath },
            { QOpenXRActionMapper::TrackpadPressed, trackpadClick, BothHandsSubPath },
            { QOpenXRActionMapper::TrackpadTouched, trackpadTouch, BothHandsSubPath },
        };

        std::vector<XrActionSuggestedBinding> bindings {{
                {m_handActions.gripPoseAction, handLeftGripPose},
                {m_handActions.aimPoseAction, handLeftAimPose},
                {m_handActions.hapticAction, handLeftHaptic},

                {m_handActions.gripPoseAction, handRightGripPose},
                {m_handActions.aimPoseAction, handRightAimPose},
                {m_handActions.hapticAction, handRightHaptic},
            }};

        addToBindings(bindings, mappingDefs);

        XrInteractionProfileSuggestedBinding suggestedBindings{};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = htcViveProfile;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
        checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings), "suggested bindings: Vive controller");
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

    // Gamepad ### TODO update to new pattern
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

    // XBox Controller
    if (!m_disableGamepad) {
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
        checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings), "suggested bindings: XBox controller");
    }

    // Setup Action Spaces

    XrActionSpaceCreateInfo actionSpaceInfo{};
    actionSpaceInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
    actionSpaceInfo.action = m_handActions.gripPoseAction;
    actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
    //actionSpaceInfo.poseInActionSpace.orientation.y = 1.0f;
    actionSpaceInfo.subactionPath = m_handSubactionPath[0];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handGripSpace[0]), "action space: handGripSpace[0]");
    actionSpaceInfo.subactionPath = m_handSubactionPath[1];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handGripSpace[1]), "action space: handGripSpace[1]");

    actionSpaceInfo.action = m_handActions.aimPoseAction;
    actionSpaceInfo.subactionPath = m_handSubactionPath[0];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handAimSpace[0]), "action space: handAimSpace[0]");
    actionSpaceInfo.subactionPath = m_handSubactionPath[1];
    checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handAimSpace[1]), "action space: handAimSpace[1]");

    // Attach Action set to session

    XrSessionActionSetsAttachInfo attachInfo{};
    attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    checkXrResult(xrAttachSessionActionSets(m_session, &attachInfo), "attach action set");

    m_initialized = true;
}

void QOpenXRInputManager::teardown()
{
    if (!m_initialized)
        return;

    m_initialized = false;

    xrDestroySpace(m_handGripSpace[0]);
    xrDestroySpace(m_handGripSpace[1]);
    xrDestroySpace(m_handAimSpace[0]);
    xrDestroySpace(m_handAimSpace[1]);

    destroyActions();

    if (xrDestroyHandTrackerEXT_) {
        xrDestroyHandTrackerEXT_(handTracker[LeftHand]);
        xrDestroyHandTrackerEXT_(handTracker[RightHand]);
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
        checkXrResult(result, "xrSyncActions");
        return;
    }

    using namespace std::placeholders;

    // Hands
    XrActionStateGetInfo getInfo{};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    for (int i = 0; i < 2; ++i) {

        getInfo.subactionPath = m_handSubactionPath[i];
        auto &inputState = m_handInputState[i];

        for (const auto &def : handInputActions) {
            getInfo.action = m_handActions.actions[def.id];
            switch (def.type) {
            case XR_ACTION_TYPE_BOOLEAN_INPUT: {
                XrActionStateBoolean boolValue{};
                boolValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;
                checkXrResult(xrGetActionStateBoolean(m_session, &getInfo, &boolValue), "bool hand input");
                if (boolValue.isActive && boolValue.changedSinceLastSync) {
                    //qDebug() << "ACTION" << i << def.shortName << bool(boolValue.currentState);
                    m_handInputState[i]->setInputValue(def.id, def.shortName, float(boolValue.currentState));
                }
                break;
            }
            case XR_ACTION_TYPE_FLOAT_INPUT: {
                XrActionStateFloat floatValue{};
                floatValue.type = XR_TYPE_ACTION_STATE_FLOAT;
                checkXrResult(xrGetActionStateFloat(m_session, &getInfo, &floatValue), "float hand input");
                if (floatValue.isActive && floatValue.changedSinceLastSync) {
                    //qDebug() << "ACTION" << i << def.shortName << floatValue.currentState;
                    m_handInputState[i]->setInputValue(def.id, def.shortName, float(floatValue.currentState));
                }
                break;
            }
            case XR_ACTION_TYPE_VECTOR2F_INPUT:
            case XR_ACTION_TYPE_POSE_INPUT:
            case XR_ACTION_TYPE_VIBRATION_OUTPUT:
            case XR_ACTION_TYPE_MAX_ENUM:
                break;
            }
        }

        // Get pose activity status
        getInfo.action = m_handActions.gripPoseAction;
        XrActionStatePose poseState{};
        poseState.type = XR_TYPE_ACTION_STATE_POSE;
        checkXrResult(xrGetActionStatePose(m_session, &getInfo, &poseState), "xrGetActionStatePose XR_TYPE_ACTION_STATE_POSE");
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
        // qDebug() << "LOCATE SPACE hand:" << hand << "res" << res << "flags" << spaceLocation.locationFlags
        //          << "active" << m_handInputState[hand]->isActive()
        //          << "Pos" << spaceLocation.pose.position.x << spaceLocation.pose.position.y << spaceLocation.pose.position.z;
        checkXrResult(res, "xrLocateSpace");
        m_validAimStateFromUpdatePoses[hand] = m_handInputState[hand]->poseSpace() == QOpenXRHandInput::HandPoseSpace::AimPose
                && XR_UNQUALIFIED_SUCCESS(res) && (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
                && (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT); // ### Workaround for Quest issue with hand interaction aim pose

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

void QOpenXRInputManager::updateHandtracking(XrTime predictedDisplayTime, XrSpace appSpace, bool aimExtensionEnabled)
{
    if (xrLocateHandJointsEXT_) {

        XrHandTrackingAimStateFB aimState[2] = {{}, {}}; // Only used when aim extension is enabled
        XrHandJointVelocitiesEXT velocities[2]{{}, {}};
        XrHandJointLocationsEXT locations[2]{{}, {}};
        XrHandJointsLocateInfoEXT locateInfo[2] = {{}, {}};

        for (auto hand : {QOpenXRInputManager::LeftHand, QOpenXRInputManager::RightHand}) {
            aimState[hand].type = XR_TYPE_HAND_TRACKING_AIM_STATE_FB;

            velocities[hand].type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
            velocities[hand].jointCount = XR_HAND_JOINT_COUNT_EXT;
            velocities[hand].jointVelocities = jointVelocities[hand];
            velocities[hand].next = aimExtensionEnabled ? &aimState[hand] : nullptr;

            locations[hand].type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
            locations[hand].next = &velocities[hand];
            locations[hand].jointCount = XR_HAND_JOINT_COUNT_EXT;
            locations[hand].jointLocations = jointLocations[hand];

            locateInfo[hand].type = XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT;
            locateInfo[hand].baseSpace = appSpace;
            locateInfo[hand].time = predictedDisplayTime;
            checkXrResult(xrLocateHandJointsEXT_(handTracker[hand], &locateInfo[hand], &locations[hand]), "handTracker");
        }

        if (aimExtensionEnabled) {
            // Finger pinch handling
            for (auto hand : {QOpenXRInputManager::LeftHand, QOpenXRInputManager::RightHand}) {
                const uint state = aimState[hand].status;
                const uint oldState = m_aimStateFlags[hand];
                auto updateState = [&](const char *name, QOpenXRActionMapper::InputAction id, uint flag) {
                    if ((state & flag) != (oldState & flag))
                        m_handInputState[hand]->setInputValue(id, name, float(!!(state & flag)));
                };

                updateState("index_pinch", QOpenXRActionMapper::IndexFingerPinch, XR_HAND_TRACKING_AIM_INDEX_PINCHING_BIT_FB);
                updateState("middle_pinch", QOpenXRActionMapper::MiddleFingerPinch, XR_HAND_TRACKING_AIM_MIDDLE_PINCHING_BIT_FB);
                updateState("ring_pinch", QOpenXRActionMapper::RingFingerPinch, XR_HAND_TRACKING_AIM_RING_PINCHING_BIT_FB);
                updateState("little_pinch", QOpenXRActionMapper::LittleFingerPinch, XR_HAND_TRACKING_AIM_LITTLE_PINCHING_BIT_FB);
                updateState("hand_tracking_menu_press", QOpenXRActionMapper::HandTrackingMenuPress, XR_HAND_TRACKING_AIM_MENU_PRESSED_BIT_FB);
                m_aimStateFlags[hand] = state;
            }

            // ### Workaround for Quest issue with hand interaction aim pose
            for (auto hand : {QOpenXRInputManager::LeftHand, QOpenXRInputManager::RightHand}) {
                if (!m_validAimStateFromUpdatePoses[hand]) {
                    if ((aimState[hand].status & XR_HAND_TRACKING_AIM_VALID_BIT_FB) && m_handInputState[hand]->poseSpace() == QOpenXRHandInput::HandPoseSpace::AimPose) {
                        setPosePosition(hand, QVector3D(aimState[hand].aimPose.position.x,
                                                        aimState[hand].aimPose.position.y,
                                                        aimState[hand].aimPose.position.z) * 100.0f);
                        setPoseRotation(hand, QQuaternion(aimState[hand].aimPose.orientation.w,
                                                          aimState[hand].aimPose.orientation.x,
                                                          aimState[hand].aimPose.orientation.y,
                                                          aimState[hand].aimPose.orientation.z));
                        m_handInputState[hand]->setIsActive(true); // TODO: clean up
                    }
                }
            }
        }
    }
}

void QOpenXRInputManager::setupHandTracking()
{
    checkXrResult(xrGetInstanceProcAddr(
        m_instance,
        "xrCreateHandTrackerEXT",
        (PFN_xrVoidFunction*)(&xrCreateHandTrackerEXT_)), "xrCreateHandTrackerEXT");
    checkXrResult(xrGetInstanceProcAddr(
        m_instance,
        "xrDestroyHandTrackerEXT",
        (PFN_xrVoidFunction*)(&xrDestroyHandTrackerEXT_)), "xrDestroyHandTrackerEXT");
    checkXrResult(xrGetInstanceProcAddr(
        m_instance,
        "xrLocateHandJointsEXT",
        (PFN_xrVoidFunction*)(&xrLocateHandJointsEXT_)), "xrLocateHandJointsEXT");

    if (xrCreateHandTrackerEXT_) {
        XrHandTrackerCreateInfoEXT createInfo{};
        createInfo.type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT;
        createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
        createInfo.hand = XR_HAND_LEFT_EXT;
        checkXrResult(xrCreateHandTrackerEXT_(m_session, &createInfo, &handTracker[LeftHand]), "xrCreateHandTrackerEXT handTrackerLeft");
        createInfo.hand = XR_HAND_RIGHT_EXT;
        checkXrResult(xrCreateHandTrackerEXT_(m_session, &createInfo, &handTracker[RightHand]), "xrCreateHandTrackerEXT handTrackerRight");
    }
};

void QOpenXRInputManager::setupActions()
{
    handInputActions = {
        { QOpenXRActionMapper::Button1Pressed, "b1_pressed", "Button 1 Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::Button1Touched, "b1_touched", "Button 1 Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::Button2Pressed, "b2_pressed", "Button 2 Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::Button2Touched, "b2_touched", "Button 2 Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::ButtonMenuPressed, "bmenu_pressed", "Button Menu Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::ButtonMenuTouched, "bmenu_touched", "Button Menu Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::ButtonSystemPressed, "bsystem_pressed", "Button System Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::ButtonSystemTouched, "bsystem_touched", "Button System Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::SqueezeValue, "squeeze_value", "Squeeze Value", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::SqueezeForce, "squeeze_force", "Squeeze Force", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::SqueezePressed, "squeeze_pressed", "Squeeze Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::TriggerValue, "trigger_value", "Trigger Value", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::TriggerPressed, "trigger_pressed", "Trigger Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::TriggerTouched, "trigger_touched", "Trigger Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::ThumbstickX, "thumbstick_x", "Thumbstick X", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::ThumbstickY, "thumbstick_y", "Thumbstick Y", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::ThumbstickPressed, "thumbstick_pressed", "Thumbstick Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::ThumbstickTouched, "thumbstick_touched", "Thumbstick Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::ThumbrestTouched, "thumbrest_touched", "Thumbrest Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::TrackpadX, "trackpad_x", "Trackpad X", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::TrackpadY, "trackpad_y", "Trackpad Y", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::TrackpadForce, "trackpad_force", "Trackpad Force", XR_ACTION_TYPE_FLOAT_INPUT, 0 },
        { QOpenXRActionMapper::TrackpadTouched, "trackpad_touched", "Trackpad Touched", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 },
        { QOpenXRActionMapper::TrackpadPressed, "trackpad_pressed", "Trackpad Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT, 0 }
    };

    // Create an action set.
    {
        XrActionSetCreateInfo actionSetInfo{};
        actionSetInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
        strcpy(actionSetInfo.actionSetName, "gameplay");
        strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
        actionSetInfo.priority = 0;
        checkXrResult(xrCreateActionSet(m_instance, &actionSetInfo, &m_actionSet), "xrCreateActionSet gameplay");
    }

    // Create Hand Actions
    setPath(m_handSubactionPath[0], "/user/hand/left");
    setPath(m_handSubactionPath[1], "/user/hand/right");

    for (const auto &def : handInputActions) {
        createAction(def.type,
                     def.shortName,
                     def.localizedName,
                     2,
                     m_handSubactionPath,
                     m_handActions.actions[def.id]);
    }

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

    // Create Gamepad Actions ### TODO
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

void QOpenXRInputManager::destroyActions()
{
    for (auto &action : m_handActions.actions) {
        if (action)
            xrDestroyAction(action);
    }

    xrDestroyAction(m_handActions.gripPoseAction);
    xrDestroyAction(m_handActions.aimPoseAction);
    xrDestroyAction(m_handActions.hapticAction);

    if (!m_disableGamepad) {
        xrDestroyAction(m_gamepadActions.buttonMenuPressedAction);
        xrDestroyAction(m_gamepadActions.buttonViewPressedAction);
        xrDestroyAction(m_gamepadActions.buttonAPressedAction);
        xrDestroyAction(m_gamepadActions.buttonBPressedAction);
        xrDestroyAction(m_gamepadActions.buttonXPressedAction);
        xrDestroyAction(m_gamepadActions.buttonYPressedAction);
        xrDestroyAction(m_gamepadActions.buttonDownPressedAction);
        xrDestroyAction(m_gamepadActions.buttonRightPressedAction);
        xrDestroyAction(m_gamepadActions.buttonUpPressedAction);
        xrDestroyAction(m_gamepadActions.buttonLeftPressedAction);
        xrDestroyAction(m_gamepadActions.shoulderLeftPressedAction);
        xrDestroyAction(m_gamepadActions.shoulderRightPressedAction);
        xrDestroyAction(m_gamepadActions.thumbstickLeftPressedAction);
        xrDestroyAction(m_gamepadActions.thumbstickRightPressedAction);
        xrDestroyAction(m_gamepadActions.triggerLeftAction);
        xrDestroyAction(m_gamepadActions.triggerRightAction);
        xrDestroyAction(m_gamepadActions.thumbstickLeftXAction);
        xrDestroyAction(m_gamepadActions.thumbstickLeftYAction);
        xrDestroyAction(m_gamepadActions.thumbstickRightXAction);
        xrDestroyAction(m_gamepadActions.thumbstickRightYAction);
        xrDestroyAction(m_gamepadActions.hapticLeftAction);
        xrDestroyAction(m_gamepadActions.hapticRightAction);
        xrDestroyAction(m_gamepadActions.hapticLeftTriggerAction);
        xrDestroyAction(m_gamepadActions.hapticRightTriggerAction);
    }

    xrDestroyActionSet(m_actionSet);
}

bool QOpenXRInputManager::checkXrResult(const XrResult &result, const char *debugText)
{
    bool checkResult = OpenXRHelpers::checkXrResult(result, m_instance);
    if (!checkResult) {
        qDebug() << "checkXrResult failed" << result << (debugText ? debugText : "");
    }
    return checkResult;

}

void QOpenXRInputManager::setPath(XrPath &path, const QByteArray &pathString)
{
    checkXrResult(xrStringToPath(m_instance, pathString.constData(), &path), "xrStringToPath");
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
    bool res = checkXrResult(xrCreateAction(m_actionSet, &actionInfo, &action), "xrCreateAction");
    if (!res)
        qDebug() << "xrCreateAction failed. Name:" << name << "localizedName:" << localizedName;
}

void QOpenXRInputManager::getBoolInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(bool)> setter)
{
    getInfo.action = action;
    XrActionStateBoolean boolValue{};
    boolValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;
    checkXrResult(xrGetActionStateBoolean(m_session, &getInfo, &boolValue), "getBoolInputState");
    if (boolValue.isActive == XR_TRUE)
        setter(bool(boolValue.currentState));
}

void QOpenXRInputManager::getFloatInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(float)> setter)
{
    getInfo.action = action;
    XrActionStateFloat floatValue{};
    floatValue.type = XR_TYPE_ACTION_STATE_FLOAT;
    checkXrResult(xrGetActionStateFloat(m_session, &getInfo, &floatValue), "getFloatInputState");
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

XrSpace QOpenXRInputManager::handTrackerSpace(Hand handtracker)
{
    if (m_handTrackerInputState[handtracker]->poseSpace() == QOpenXRHandTrackerInput::HandPoseSpace::GripPose)
        return m_handGripSpace[handtracker];
    else
        return m_handAimSpace[handtracker];
}

bool QOpenXRInputManager::isHandActive(QOpenXRInputManager::Hand hand)
{
    return m_handInputState[hand]->isActive();
}

bool QOpenXRInputManager::isHandTrackerActive(Hand handtracker)
{
    return m_handTrackerInputState[handtracker]->isActive();
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

QOpenXRHandTrackerInput *QOpenXRInputManager::rightHandTrackerInput() const
{
    return m_handTrackerInputState[QOpenXRInputManager::RightHand];
}

QOpenXRHandTrackerInput *QOpenXRInputManager::leftHandTrackerInput() const
{
    return m_handTrackerInputState[QOpenXRInputManager::LeftHand];
}

QOpenXRGamepadInput *QOpenXRInputManager::gamepadInput() const
{
    return m_gamepadInputState;
}

QT_END_NAMESPACE
