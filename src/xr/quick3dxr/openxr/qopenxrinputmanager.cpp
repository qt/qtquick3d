// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../qquick3dxrinputmanager_p.h"
#include "qopenxrinputmanager_p.h"
#include "openxr/qopenxrhelpers_p.h"
#include "qquick3dxrhandinput_p.h"
#include "qquick3dxrhandtrackerinput_p.h"

#include "qopenxrcontroller_p.h" //### InputAction enum

#include <QDebug>

#include <private/qquick3djoint_p.h>

QT_BEGIN_NAMESPACE

QQuick3DXrInputManagerPrivate::QQuick3DXrInputManagerPrivate(QQuick3DXrInputManager &manager)
    : q_ptr(&manager)
{
    m_handInputState[Hand::LeftHand] = new QQuick3DXrHandInput(this);
    m_handInputState[Hand::RightHand] = new QQuick3DXrHandInput(this);
    m_handTrackerInputState[Hand::LeftHand] = new QQuick3DXrHandTrackerInput(this);
    m_handTrackerInputState[Hand::RightHand] = new QQuick3DXrHandTrackerInput(this);
}

QQuick3DXrInputManagerPrivate::~QQuick3DXrInputManagerPrivate()
{
    teardown();
    delete m_handInputState[Hand::LeftHand];
    delete m_handInputState[Hand::RightHand];
    delete m_handTrackerInputState[Hand::LeftHand];
    delete m_handTrackerInputState[Hand::RightHand];

    m_handInputState[Hand::LeftHand] = nullptr;
    m_handInputState[Hand::RightHand] = nullptr;
    m_handTrackerInputState[Hand::LeftHand] = nullptr;
    m_handTrackerInputState[Hand::RightHand] = nullptr;
}

QQuick3DXrInputManagerPrivate::QXRHandComponentPath QQuick3DXrInputManagerPrivate::makeHandInputPaths(const QByteArrayView path)
{
    QXRHandComponentPath res;
    setPath(res.paths[Hand::LeftHand], "/user/hand/left/" + path);
    setPath(res.paths[Hand::RightHand], "/user/hand/right/" + path);
    return res;
}


XrPath QQuick3DXrInputManagerPrivate::makeInputPath(const QByteArrayView path)
{
    XrPath res;
    setPath(res, path.toByteArray());
    return res;
}

QQuick3DGeometry *QQuick3DXrInputManagerPrivate::createHandMeshGeometry(const HandMeshData &handMeshData)
{
    QQuick3DGeometry *geometry = new QQuick3DGeometry();
    geometry->setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

    // Figure out which attributes should be used
    const qsizetype expectedLength = handMeshData.vertexPositions.size();
    bool hasPositions = !handMeshData.vertexPositions.isEmpty();
    bool hasNormals = handMeshData.vertexNormals.size() >= expectedLength;
    bool hasUV0s = handMeshData.vertexUVs.size() >= expectedLength;
    bool hasJoints = handMeshData.vertexBlendIndices.size() >= expectedLength;
    bool hasWeights = handMeshData.vertexBlendWeights.size() >= expectedLength;
    bool hasIndexes = !handMeshData.indices.isEmpty();

    int offset = 0;
    if (hasPositions) {
        geometry->addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, offset, QQuick3DGeometry::Attribute::ComponentType::F32Type);
        offset += 3 * sizeof(float);
    }

    if (hasNormals) {
        geometry->addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, offset, QQuick3DGeometry::Attribute::ComponentType::F32Type);
        offset += 3 * sizeof(float);
    }

    if (hasUV0s) {
        geometry->addAttribute(QQuick3DGeometry::Attribute::TexCoordSemantic, offset, QQuick3DGeometry::Attribute::ComponentType::F32Type);
        offset += 2 * sizeof(float);
    }

    if (hasJoints) {
        geometry->addAttribute(QQuick3DGeometry::Attribute::JointSemantic, offset, QQuick3DGeometry::Attribute::ComponentType::I32Type);
        offset += 4 * sizeof(qint32);
    }

    if (hasWeights) {
        geometry->addAttribute(QQuick3DGeometry::Attribute::WeightSemantic, offset, QQuick3DGeometry::Attribute::ComponentType::F32Type);
        offset += 4 * sizeof(float);
    }

    if (hasIndexes)
        geometry->addAttribute(QQuick3DGeometry::Attribute::IndexSemantic, 0, QQuick3DGeometry::Attribute::ComponentType::U16Type);

    // set up the vertex buffer
    const int stride = offset;
    const qsizetype bufferSize = expectedLength * stride;
    geometry->setStride(stride);

    QByteArray vertexBuffer;
    vertexBuffer.reserve(bufferSize);

    QVector3D minBounds;
    QVector3D maxBounds;

    auto appendFloat = [&vertexBuffer](float f) {
        vertexBuffer.append(reinterpret_cast<const char *>(&f), sizeof(float));
    };
    auto appendInt = [&vertexBuffer](qint32 i) {
        vertexBuffer.append(reinterpret_cast<const char *>(&i), sizeof(qint32));
    };

    for (qsizetype i = 0; i < expectedLength; ++i) {
        // start writing float values to vertexBuffer
        if (hasPositions) {
            const QVector3D position = OpenXRHelpers::toQVector(handMeshData.vertexPositions[i]);
            appendFloat(position.x());
            appendFloat(position.y());
            appendFloat(position.z());
            minBounds.setX(qMin(minBounds.x(), position.x()));
            maxBounds.setX(qMax(maxBounds.x(), position.x()));
            minBounds.setY(qMin(minBounds.y(), position.y()));
            maxBounds.setY(qMax(maxBounds.y(), position.y()));
            minBounds.setZ(qMin(minBounds.z(), position.z()));
            maxBounds.setZ(qMax(maxBounds.z(), position.z()));
        }
        if (hasNormals) {
            const auto &normal = handMeshData.vertexNormals[i];
            appendFloat(normal.x);
            appendFloat(normal.y);
            appendFloat(normal.z);
        }

        if (hasUV0s) {
            const auto &uv0 = handMeshData.vertexUVs[i];
            appendFloat(uv0.x);
            appendFloat(uv0.y);
        }

        if (hasJoints) {
            const auto &joint = handMeshData.vertexBlendIndices[i];
            appendInt(joint.x);
            appendInt(joint.y);
            appendInt(joint.z);
            appendInt(joint.w);
        }

        if (hasWeights) {
            const auto &weight = handMeshData.vertexBlendWeights[i];
            appendFloat(weight.x);
            appendFloat(weight.y);
            appendFloat(weight.z);
            appendFloat(weight.w);
        }
    }

    geometry->setBounds(minBounds, maxBounds);
    geometry->setVertexData(vertexBuffer);

    // Index Buffer
    if (hasIndexes) {
        const qsizetype indexLength = handMeshData.indices.size();
        QByteArray indexBuffer;
        indexBuffer.reserve(indexLength * sizeof(int16_t));
        for (qsizetype i = 0; i < indexLength; ++i) {
            const auto &index = handMeshData.indices[i];
            indexBuffer.append(reinterpret_cast<const char *>(&index), sizeof(int16_t));
        }
        geometry->setIndexData(indexBuffer);
    }

    return geometry;
}

void QQuick3DXrInputManagerPrivate::init(XrInstance instance, XrSession session)
{
    if (m_initialized) {
        qWarning() << "QQuick3DXrInputManager: Trying to initialize an already initialized session";
        teardown();
    }

    m_instance = instance;
    m_session = session;

    setupHandTracking();

    setupActions();

    QXRHandComponentPath aClick = makeHandInputPaths("input/a/click"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)
    QXRHandComponentPath bClick = makeHandInputPaths("input/b/click"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)
    QXRHandComponentPath aTouch = makeHandInputPaths("input/a/touch"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)
    QXRHandComponentPath bTouch = makeHandInputPaths("input/b/touch"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left)

    QXRHandComponentPath xClick = makeHandInputPaths("input/x/click"); // OCULUS_TOUCH (left)
    QXRHandComponentPath yClick = makeHandInputPaths("input/y/click"); // OCULUS_TOUCH (left)
    QXRHandComponentPath xTouch = makeHandInputPaths("input/x/touch"); // OCULUS_TOUCH (left)
    QXRHandComponentPath yTouch = makeHandInputPaths("input/y/touch"); // OCULUS_TOUCH (left)

    QXRHandComponentPath menuClick = makeHandInputPaths("input/menu/click"); // OCULUS_TOUCH (left) | MICROSOFT_MRM (right + left) | HTC_VIVE (right + left)
    QXRHandComponentPath systemClick = makeHandInputPaths("input/system/click"); // OCULUS_TOUCH (right) | VALVE_INDEX (right + left) | HTC_VIVE (right + left)
    QXRHandComponentPath systemTouch = makeHandInputPaths("input/system/touch"); // VALVE_INDEX (right + left)

    QXRHandComponentPath squeezeValue = makeHandInputPaths("input/squeeze/value"); // right + left: OCULUS_TOUCH | VALVE_INDEX
    QXRHandComponentPath squeezeForce = makeHandInputPaths("input/squeeze/force"); // right + left: VALVE_INDEX
    QXRHandComponentPath squeezeClick = makeHandInputPaths("input/squeeze/click"); // right + left: MICROSOFT_MRM | HTC_VIVE

    QXRHandComponentPath triggerValue = makeHandInputPaths("input/trigger/value"); // right + left: OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath triggerTouch = makeHandInputPaths("input/trigger/touch"); // right + left: OCULUS_TOUCH | VALVE_INDEX
    QXRHandComponentPath triggerClick = makeHandInputPaths("input/trigger/click"); // right + left: VALVE_INDEX | HTC_VIVE

    QXRHandComponentPath thumbstickX = makeHandInputPaths("input/thumbstick/x"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left) | MICROSOFT_MRM (left)
    QXRHandComponentPath thumbstickY = makeHandInputPaths("input/thumbstick/y"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left) | MICROSOFT_MRM (left)
    QXRHandComponentPath thumbstickClick = makeHandInputPaths("input/thumbstick/click"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left) | MICROSOFT_MRM (left)
    QXRHandComponentPath thumbstickTouch = makeHandInputPaths("input/thumbstick/touch"); // OCULUS_TOUCH (right + left) | VALVE_INDEX (right + left)
    QXRHandComponentPath thumbrestTouch = makeHandInputPaths("input/thumbrest/touch"); // OCULUS_TOUCH (right + left)

    QXRHandComponentPath trackpadX = makeHandInputPaths("input/trackpad/x"); // right + left:  VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath trackpadY = makeHandInputPaths("input/trackpad/y"); // right + left:  VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath trackpadForce = makeHandInputPaths("input/trackpad/force"); // right + left:  VALVE_INDEX
    QXRHandComponentPath trackpadClick = makeHandInputPaths("input/trackpad/click"); // right + left:  VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    QXRHandComponentPath trackpadTouch = makeHandInputPaths("input/trackpad/touch"); // right + left:  MICROSOFT_MRM | HTC_VIVE

    XrPath handLeftGripPose;                  // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftAimPose;                   // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handLeftHaptic;                    // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE

    XrPath handRightGripPose;                 // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightAimPose;                  // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE
    XrPath handRightHaptic;                   // OCULUS_TOUCH | VALVE_INDEX | MICROSOFT_MRM | HTC_VIVE

    // Hand Left

    setPath(handLeftGripPose, "/user/hand/left/input/grip/pose");
    setPath(handLeftAimPose, "/user/hand/left/input/aim/pose");
    setPath(handLeftHaptic, "/user/hand/left/output/haptic");

    setPath(handRightGripPose, "/user/hand/right/input/grip/pose");
    setPath(handRightAimPose, "/user/hand/right/input/aim/pose");
    setPath(handRightHaptic, "/user/hand/right/output/haptic");

    // Bindings

    using XrActionBindings = std::vector<XrActionSuggestedBinding>;
    using HandInputMapping = std::vector<std::tuple<QOpenXRActionMapper::InputAction, QXRHandComponentPath, SubPathSelector>>;
    auto addToBindings = [this](XrActionBindings &bindings, const HandInputMapping &defs){
        for (const auto &[actionId, path, selector] : defs) {
            if (selector & LeftHandSubPath)
                bindings.push_back({ m_inputActions[actionId], path.paths[Hand::LeftHand] });
            if (selector & RightHandSubPath)
                bindings.push_back({ m_inputActions[actionId], path.paths[Hand::RightHand] });
        }
    };

    // Oculus Touch
    {
        HandInputMapping mappingDefs {
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
        if (!checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)))
            qWarning("Failed to get suggested interaction profile bindings for Oculus touch");
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

        HandInputMapping mappingDefs {
            { QOpenXRActionMapper::SqueezeValue, squeezeValue, BothHandsSubPath },
        };

        addToBindings(bindings, mappingDefs);

        XrInteractionProfileSuggestedBinding suggestedBindings{};
        suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestedBindings.interactionProfile = handInteractionProfile;
        suggestedBindings.suggestedBindings = bindings.data();
        suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();

        if (!checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)))
            qWarning("Failed to get suggested interaction profile bindings for MSFT hand interaction");
    }

    {
        XrPath htcViveProfile;
        setPath(htcViveProfile, "/interaction_profiles/htc/vive_controller");

        HandInputMapping mappingDefs {
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
        if (!checkXrResult(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings)))
            qWarning("Failed to get suggested interaction profile bindings for Vive controller");
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

    // Setup Action Spaces

    XrActionSpaceCreateInfo actionSpaceInfo{};
    actionSpaceInfo.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
    actionSpaceInfo.action = m_handActions.gripPoseAction;
    actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
    //actionSpaceInfo.poseInActionSpace.orientation.y = 1.0f;
    actionSpaceInfo.subactionPath = m_handSubactionPath[0];
    if (!checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handGripSpace[0])))
        qWarning("Failed to create action space for handGripSpace[0]");
    actionSpaceInfo.subactionPath = m_handSubactionPath[1];
    if (!checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handGripSpace[1])))
        qWarning("Failed to create action space for handGripSpace[1]");

    actionSpaceInfo.action = m_handActions.aimPoseAction;
    actionSpaceInfo.subactionPath = m_handSubactionPath[0];
    if (!checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handAimSpace[0])))
        qWarning("Failed to create action space for handAimSpace[0]");
    actionSpaceInfo.subactionPath = m_handSubactionPath[1];
    if (!checkXrResult(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_handAimSpace[1])))
        qWarning("Failed to create action space for handAimSpace[1]");

    // Attach Action set to session

    XrSessionActionSetsAttachInfo attachInfo{};
    attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &m_actionSet;
    if (!checkXrResult(xrAttachSessionActionSets(m_session, &attachInfo)))
        qWarning("Failed to attach action sets to session");

    m_initialized = true;
}

void QQuick3DXrInputManagerPrivate::teardown()
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
        xrDestroyHandTrackerEXT_(handTracker[Hand::LeftHand]);
        xrDestroyHandTrackerEXT_(handTracker[Hand::RightHand]);
    }

    m_instance = {XR_NULL_HANDLE};
    m_session = {XR_NULL_HANDLE};
}

QQuick3DXrInputManagerPrivate *QQuick3DXrInputManagerPrivate::get(QQuick3DXrInputManager *inputManager)
{
    QSSG_ASSERT(inputManager != nullptr, return nullptr);
    return inputManager->d_func();
}

void QQuick3DXrInputManagerPrivate::pollActions()
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
          result == XR_SESSION_NOT_FOCUSED))
    {
        if (!checkXrResult(result)) {
            qWarning("xrSyncActions failed");
            return;
        }
    }

    // Hands
    XrActionStateGetInfo getInfo{};
    getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
    for (int i = 0; i < 2; ++i) {

        getInfo.subactionPath = m_handSubactionPath[i];
        auto &inputState = m_handInputState[i];

        for (const auto &def : m_handInputActionDefs) {
            getInfo.action = m_inputActions[def.id];
            switch (def.type) {
            case XR_ACTION_TYPE_BOOLEAN_INPUT: {
                XrActionStateBoolean boolValue{};
                boolValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;
                if (checkXrResult(xrGetActionStateBoolean(m_session, &getInfo, &boolValue))) {
                    if (boolValue.isActive && boolValue.changedSinceLastSync) {
                        //qDebug() << "ACTION" << i << def.shortName << bool(boolValue.currentState);
                        m_handInputState[i]->setInputValue(def.id, def.shortName, float(boolValue.currentState));
                    }
                } else {
                    qWarning("Failed to get action state for bool hand input");
                }
                break;
            }
            case XR_ACTION_TYPE_FLOAT_INPUT: {
                XrActionStateFloat floatValue{};
                floatValue.type = XR_TYPE_ACTION_STATE_FLOAT;
                if (checkXrResult(xrGetActionStateFloat(m_session, &getInfo, &floatValue))) {
                    if (floatValue.isActive && floatValue.changedSinceLastSync) {
                        //qDebug() << "ACTION" << i << def.shortName << floatValue.currentState;
                        m_handInputState[i]->setInputValue(def.id, def.shortName, float(floatValue.currentState));
                    }
                } else {
                    qWarning("Failed to get action state for float hand input");
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
        if (checkXrResult(xrGetActionStatePose(m_session, &getInfo, &poseState)))
            inputState->setIsActive(poseState.isActive);
        else
            qWarning("Failed to get action state pose");

        // TODO handle any output as well here (haptics)
    //    XrAction gripPoseAction{XR_NULL_HANDLE};
    //    XrAction aimPoseAction{XR_NULL_HANDLE};
    //    XrAction hapticAction{XR_NULL_HANDLE};

    }

}

void QQuick3DXrInputManagerPrivate::updatePoses(XrTime predictedDisplayTime, XrSpace appSpace)
{
    // Update the Hands pose

    for (auto hand : {Hand::LeftHand, Hand::RightHand}) {
        XrSpaceLocation spaceLocation{};
        spaceLocation.type = XR_TYPE_SPACE_LOCATION;
        XrResult res;
        res = xrLocateSpace(handSpace(hand), appSpace, predictedDisplayTime, &spaceLocation);
        // qDebug() << "LOCATE SPACE hand:" << hand << "res" << res << "flags" << spaceLocation.locationFlags
        //          << "active" << m_handInputState[hand]->isActive()
        //          << "Pos" << spaceLocation.pose.position.x << spaceLocation.pose.position.y << spaceLocation.pose.position.z;
        m_validAimStateFromUpdatePoses[hand] = m_handInputState[hand]->poseSpace() == QQuick3DXrHandInput::HandPoseSpace::AimPose
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

void QQuick3DXrInputManagerPrivate::updateHandtracking(XrTime predictedDisplayTime, XrSpace appSpace, bool aimExtensionEnabled)
{
    if (xrLocateHandJointsEXT_) {

        XrHandTrackingAimStateFB aimState[2] = {{}, {}}; // Only used when aim extension is enabled
        XrHandJointVelocitiesEXT velocities[2]{{}, {}};
        XrHandJointLocationsEXT locations[2]{{}, {}};
        XrHandJointsLocateInfoEXT locateInfo[2] = {{}, {}};

        for (auto hand : {Hand::LeftHand, Hand::RightHand}) {
            if (handTracker[hand] == XR_NULL_HANDLE)
                continue;

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
            if (!checkXrResult(xrLocateHandJointsEXT_(handTracker[hand], &locateInfo[hand], &locations[hand])))
                qWarning("Failed to locate hand joints for hand tracker");

            QList<QVector3D> jp;
            jp.reserve(XR_HAND_JOINT_COUNT_EXT);
            QList<QQuaternion> jr;
            jr.reserve(XR_HAND_JOINT_COUNT_EXT);
            for (uint i = 0; i < locations[hand].jointCount; ++i) {
                auto &pose = jointLocations[hand][i].pose;
                jp.append(OpenXRHelpers::toQVector(pose.position));
                jr.append(OpenXRHelpers::toQQuaternion(pose.orientation));
            }
            m_handTrackerInputState[hand]->setJointPositionsAndRotations(jp, jr);
            m_handTrackerInputState[hand]->setIsActive(locations[hand].isActive);
        }

        if (aimExtensionEnabled) {
            // Finger pinch handling
            for (auto hand : {Hand::LeftHand, Hand::RightHand}) {
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
            for (auto hand : {Hand::LeftHand, Hand::RightHand}) {
                if (!m_validAimStateFromUpdatePoses[hand]) {
                    if ((aimState[hand].status & XR_HAND_TRACKING_AIM_VALID_BIT_FB) && m_handInputState[hand]->poseSpace() == QQuick3DXrHandInput::HandPoseSpace::AimPose) {
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

void QQuick3DXrInputManagerPrivate::setupHandTracking()
{
    resolveXrFunction(
        "xrCreateHandTrackerEXT",
        (PFN_xrVoidFunction*)(&xrCreateHandTrackerEXT_));
    resolveXrFunction(
        "xrDestroyHandTrackerEXT",
        (PFN_xrVoidFunction*)(&xrDestroyHandTrackerEXT_));
    resolveXrFunction(
        "xrLocateHandJointsEXT",
        (PFN_xrVoidFunction*)(&xrLocateHandJointsEXT_));
    resolveXrFunction(
        "xrGetHandMeshFB",
        (PFN_xrVoidFunction*)(&xrGetHandMeshFB_));

    if (xrCreateHandTrackerEXT_) {
        XrHandTrackerCreateInfoEXT createInfo{};
        createInfo.type = XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT;
        createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
        createInfo.hand = XR_HAND_LEFT_EXT;
        if (!checkXrResult(xrCreateHandTrackerEXT_(m_session, &createInfo, &handTracker[QtQuick3DXr::LeftHand])))
            qWarning("Failed to create left hand tracker");
        createInfo.hand = XR_HAND_RIGHT_EXT;
        if (!checkXrResult(xrCreateHandTrackerEXT_(m_session, &createInfo, &handTracker[QtQuick3DXr::RightHand])))
            qWarning("Failed to create right hand tracker");
    }
    if (xrGetHandMeshFB_) {
        for (auto hand : {Hand::LeftHand, Hand::RightHand}) {
            if (queryHandMesh(hand))
                createHandModelData(hand);
        }
    }
}

bool QQuick3DXrInputManagerPrivate::queryHandMesh(Hand hand)
{
    XrHandTrackingMeshFB mesh {};
    mesh.type = XR_TYPE_HAND_TRACKING_MESH_FB;
    // Left hand
    if (!checkXrResult(xrGetHandMeshFB_(handTracker[hand], &mesh))) {
        qWarning("Failed to query hand mesh info.");
        return false;
    }

    mesh.jointCapacityInput = mesh.jointCountOutput;
    mesh.vertexCapacityInput = mesh.vertexCountOutput;
    mesh.indexCapacityInput = mesh.indexCountOutput;
    m_handMeshData[hand].vertexPositions.resize(mesh.vertexCapacityInput);
    m_handMeshData[hand].vertexNormals.resize(mesh.vertexCapacityInput);
    m_handMeshData[hand].vertexUVs.resize(mesh.vertexCapacityInput);
    m_handMeshData[hand].vertexBlendIndices.resize(mesh.vertexCapacityInput);
    m_handMeshData[hand].vertexBlendWeights.resize(mesh.vertexCapacityInput);
    m_handMeshData[hand].indices.resize(mesh.indexCapacityInput);
    mesh.jointBindPoses = m_handMeshData[hand].jointBindPoses;
    mesh.jointParents = m_handMeshData[hand].jointParents;
    mesh.jointRadii = m_handMeshData[hand].jointRadii;
    mesh.vertexPositions = m_handMeshData[hand].vertexPositions.data();
    mesh.vertexNormals = m_handMeshData[hand].vertexNormals.data();
    mesh.vertexUVs = m_handMeshData[hand].vertexUVs.data();
    mesh.vertexBlendIndices = m_handMeshData[hand].vertexBlendIndices.data();
    mesh.vertexBlendWeights = m_handMeshData[hand].vertexBlendWeights.data();
    mesh.indices = m_handMeshData[hand].indices.data();

    if (!checkXrResult(xrGetHandMeshFB_(handTracker[hand], &mesh))) {
        qWarning("Failed to get hand mesh data.");
        return false;
    }

    return true;
};

void QQuick3DXrInputManagerPrivate::setupActions()
{
    m_handInputActionDefs = {
        { QOpenXRActionMapper::Button1Pressed, "b1_pressed", "Button 1 Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::Button1Touched, "b1_touched", "Button 1 Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::Button2Pressed, "b2_pressed", "Button 2 Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::Button2Touched, "b2_touched", "Button 2 Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::ButtonMenuPressed, "bmenu_pressed", "Button Menu Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::ButtonMenuTouched, "bmenu_touched", "Button Menu Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::ButtonSystemPressed, "bsystem_pressed", "Button System Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::ButtonSystemTouched, "bsystem_touched", "Button System Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::SqueezeValue, "squeeze_value", "Squeeze Value", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::SqueezeForce, "squeeze_force", "Squeeze Force", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::SqueezePressed, "squeeze_pressed", "Squeeze Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::TriggerValue, "trigger_value", "Trigger Value", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::TriggerPressed, "trigger_pressed", "Trigger Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::TriggerTouched, "trigger_touched", "Trigger Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::ThumbstickX, "thumbstick_x", "Thumbstick X", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::ThumbstickY, "thumbstick_y", "Thumbstick Y", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::ThumbstickPressed, "thumbstick_pressed", "Thumbstick Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::ThumbstickTouched, "thumbstick_touched", "Thumbstick Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::ThumbrestTouched, "thumbrest_touched", "Thumbrest Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::TrackpadX, "trackpad_x", "Trackpad X", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::TrackpadY, "trackpad_y", "Trackpad Y", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::TrackpadForce, "trackpad_force", "Trackpad Force", XR_ACTION_TYPE_FLOAT_INPUT },
        { QOpenXRActionMapper::TrackpadTouched, "trackpad_touched", "Trackpad Touched", XR_ACTION_TYPE_BOOLEAN_INPUT },
        { QOpenXRActionMapper::TrackpadPressed, "trackpad_pressed", "Trackpad Pressed", XR_ACTION_TYPE_BOOLEAN_INPUT }
    };

    // Create an action set.
    {
        XrActionSetCreateInfo actionSetInfo{};
        actionSetInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
        strcpy(actionSetInfo.actionSetName, "gameplay");
        strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
        actionSetInfo.priority = 0;
        if (!checkXrResult(xrCreateActionSet(m_instance, &actionSetInfo, &m_actionSet)))
            qWarning("Failed to create gameplay action set");
    }

    // Create Hand Actions
    setPath(m_handSubactionPath[0], "/user/hand/left");
    setPath(m_handSubactionPath[1], "/user/hand/right");

    for (const auto &def : m_handInputActionDefs) {
        createAction(def.type,
                     def.shortName,
                     def.localizedName,
                     2,
                     m_handSubactionPath,
                     m_inputActions[def.id]);
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

}

void QQuick3DXrInputManagerPrivate::destroyActions()
{
    for (auto &action : m_inputActions) {
        if (action)
            xrDestroyAction(action);
    }

    xrDestroyAction(m_handActions.gripPoseAction);
    xrDestroyAction(m_handActions.aimPoseAction);
    xrDestroyAction(m_handActions.hapticAction);

    xrDestroyActionSet(m_actionSet);
}

bool QQuick3DXrInputManagerPrivate::checkXrResult(const XrResult &result)
{
    return OpenXRHelpers::checkXrResult(result, m_instance);
}

bool QQuick3DXrInputManagerPrivate::resolveXrFunction(const char *name, PFN_xrVoidFunction *function)
{
    XrResult result = xrGetInstanceProcAddr(m_instance, name, function);
    if (!OpenXRHelpers::checkXrResult(result, m_instance)) {
        qWarning("Failed to resolve OpenXR function %s", name);
        *function = nullptr;
        return false;
    }
    return true;
}

void QQuick3DXrInputManagerPrivate::setPath(XrPath &path, const QByteArray &pathString)
{
    if (!checkXrResult(xrStringToPath(m_instance, pathString.constData(), &path)))
        qWarning("xrStringToPath failed");
}

void QQuick3DXrInputManagerPrivate::createAction(XrActionType type,
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
    if (!checkXrResult(xrCreateAction(m_actionSet, &actionInfo, &action)))
        qDebug() << "xrCreateAction failed. Name:" << name << "localizedName:" << localizedName;
}

void QQuick3DXrInputManagerPrivate::getBoolInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(bool)> setter)
{
    getInfo.action = action;
    XrActionStateBoolean boolValue{};
    boolValue.type = XR_TYPE_ACTION_STATE_BOOLEAN;
    if (checkXrResult(xrGetActionStateBoolean(m_session, &getInfo, &boolValue))) {
        if (boolValue.isActive == XR_TRUE)
            setter(bool(boolValue.currentState));
    } else {
        qWarning("Failed to get action state: bool");
    }
}

void QQuick3DXrInputManagerPrivate::getFloatInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(float)> setter)
{
    getInfo.action = action;
    XrActionStateFloat floatValue{};
    floatValue.type = XR_TYPE_ACTION_STATE_FLOAT;
    if (checkXrResult(xrGetActionStateFloat(m_session, &getInfo, &floatValue))) {
        if (floatValue.isActive == XR_TRUE)
            setter(float(floatValue.currentState));
    } else {
        qWarning("Failed to get action state: float");
    }
}

XrSpace QQuick3DXrInputManagerPrivate::handSpace(QQuick3DXrInputManagerPrivate::Hand hand)
{
    if (m_handInputState[hand]->poseSpace() == QQuick3DXrHandInput::HandPoseSpace::GripPose)
        return m_handGripSpace[hand];
    else
        return m_handAimSpace[hand];
}

XrSpace QQuick3DXrInputManagerPrivate::handTrackerSpace(Hand handtracker)
{
    if (m_handTrackerInputState[handtracker]->poseSpace() == QQuick3DXrHandTrackerInput::HandPoseSpace::GripPose)
        return m_handGripSpace[handtracker];
    else
        return m_handAimSpace[handtracker];
}

bool QQuick3DXrInputManagerPrivate::isHandActive(QQuick3DXrInputManagerPrivate::Hand hand)
{
    return m_handInputState[hand]->isActive();
}

bool QQuick3DXrInputManagerPrivate::isHandTrackerActive(Hand handtracker)
{
    return m_handTrackerInputState[handtracker]->isActive();
}

void QQuick3DXrInputManagerPrivate::setPosePosition(Hand hand, const QVector3D &position)
{
    m_handInputState[hand]->setPosePosition(position);
}

void QQuick3DXrInputManagerPrivate::setPoseRotation(Hand hand, const QQuaternion &rotation)
{
    m_handInputState[hand]->setPoseRotation(rotation);
}

QQuick3DXrHandInput *QQuick3DXrInputManagerPrivate::leftHandInput() const
{
    return m_handInputState[Hand::LeftHand];
}

QQuick3DXrHandInput *QQuick3DXrInputManagerPrivate::rightHandInput() const
{
    return m_handInputState[Hand::RightHand];
}

QQuick3DXrHandTrackerInput *QQuick3DXrInputManagerPrivate::rightHandTrackerInput() const
{
    return m_handTrackerInputState[Hand::RightHand];
}

QQuick3DXrHandTrackerInput *QQuick3DXrInputManagerPrivate::leftHandTrackerInput() const
{
    return m_handTrackerInputState[Hand::LeftHand];
}

static inline QMatrix4x4 transformMatrix(const QVector3D &position, const QQuaternion &rotation)
{
    QMatrix4x4 transform = QMatrix4x4{rotation.toRotationMatrix()};

    transform(0, 3) += position[0];
    transform(1, 3) += position[1];
    transform(2, 3) += position[2];

    return transform;
}

void QQuick3DXrInputManagerPrivate::setupHandModelInternal(QQuick3DXrHandModel *model, Hand hand)
{
    QQuick3DGeometry *geometry = m_handGeometryData[hand].geometry;
    if (!geometry)
        return;

    model->setGeometry(geometry);

    QQuick3DSkin *skin = new QQuick3DSkin(model);
    auto jointListProp = skin->joints();
    QList<QMatrix4x4> inverseBindPoses;
    inverseBindPoses.reserve(XR_HAND_JOINT_COUNT_EXT);

    const auto &handMeshData = m_handMeshData[hand];

    for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
        const auto &pose = handMeshData.jointBindPoses[i];
        const QVector3D pos = OpenXRHelpers::toQVector(pose.position);
        const QQuaternion rot = OpenXRHelpers::toQQuaternion(pose.orientation);
        inverseBindPoses.append(transformMatrix(pos, rot).inverted());
        QQuick3DNode *joint = new QQuick3DNode(model);
        joint->setPosition(pos);
        joint->setRotation(rot);
        jointListProp.append(&jointListProp, joint);
    }
    skin->setInverseBindPoses(inverseBindPoses);
    model->setSkin(skin);
}

void QQuick3DXrInputManagerPrivate::setupHandModel(QQuick3DXrHandModel *model)
{
    QSSG_ASSERT(model != nullptr, return);

    if (model->geometry() != nullptr || model->skin() != nullptr) {
        qWarning() << "Hand model already has geometry or skin set.";
        return;
    }

    if (QQuick3DXrHandTrackerInput *targetHT = model->handTracker()) {
        if (targetHT == m_handTrackerInputState[Hand::LeftHand])
            setupHandModelInternal(model, Hand::LeftHand);
        else if (targetHT == m_handTrackerInputState[Hand::RightHand])
            setupHandModelInternal(model, Hand::RightHand);
        else
            qWarning() << "No matching hand tracker input found for hand model.";
    }
}

void QQuick3DXrInputManagerPrivate::createHandModelData(Hand hand)
{
    const auto &handMeshData = m_handMeshData[hand];

    auto &geometry = m_handGeometryData[hand].geometry;
    delete geometry;
    geometry = createHandMeshGeometry(handMeshData);
}

QT_END_NAMESPACE
