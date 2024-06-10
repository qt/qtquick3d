// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrhandinput_p.h"
#include "qquick3dxrhandtrackerinput_p.h"
#include "qquick3dxrinputmanager_visionos_p.h"

#include "../qquick3dxrinputmanager_p.h"

#include <QtQuick3DUtils/private/qssgassert_p.h>
#include <QtQuick3DUtils/private/qssgutils_p.h>

#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>

QT_BEGIN_NAMESPACE


QQuick3DXrInputManagerPrivate::QQuick3DXrInputManagerPrivate(QQuick3DXrInputManager &manager)
    : q_ptr(&manager)
{
    m_handInputState[Hand::LeftHand] = new QQuick3DXrHandInput(&manager);
    m_handInputState[Hand::RightHand] = new QQuick3DXrHandInput(&manager);
    m_handTrackerInputState[Hand::LeftHand] = new QQuick3DXrHandTrackerInput(&manager);
    m_handTrackerInputState[Hand::RightHand] = new QQuick3DXrHandTrackerInput(&manager);
}

QQuick3DXrInputManagerPrivate::~QQuick3DXrInputManagerPrivate()
{

}

void QQuick3DXrInputManagerPrivate::prepareHandtracking(ar_data_providers_t dataProviders)
{
    QSSG_ASSERT_X(!m_initialized, "Handtracking is already initialized!", return);

    m_isHandTrackingSupported = ar_hand_tracking_provider_is_supported();
    if (m_isHandTrackingSupported) {
        ar_hand_tracking_configuration_t handTrackingConfiguration = ar_hand_tracking_configuration_create();
        m_handTrackingProvider = ar_hand_tracking_provider_create(handTrackingConfiguration);
        ar_data_providers_add_data_provider(dataProviders, m_handTrackingProvider);
    } else {
        qWarning("Hand tracking is not supported on this device.");
    }

    qDebug() << Q_FUNC_INFO << ", Handtracking supported: " << m_isHandTrackingSupported;
}

void QQuick3DXrInputManagerPrivate::initHandtracking()
{
    if (m_isHandTrackingSupported) {
        m_handAnchors[Hand::LeftHand] = ar_hand_anchor_create();
        m_handAnchors[Hand::RightHand] = ar_hand_anchor_create();
        m_initialized = true;
    }

    qDebug() << Q_FUNC_INFO << ", Initialized: " << m_initialized;
}

void QQuick3DXrInputManagerPrivate::teardown()
{
    Q_UNIMPLEMENTED(); qWarning() << Q_FUNC_INFO;
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

QQuick3DXrInputManagerPrivate *QQuick3DXrInputManagerPrivate::get(QQuick3DXrInputManager *inputManager)
{
    return inputManager->d_func();
}

void QQuick3DXrInputManagerPrivate::setupHandModel(QQuick3DXrHandModel *model)
{
    Q_UNUSED(model);
    Q_UNIMPLEMENTED(); qWarning() << Q_FUNC_INFO;
}

static bool setupJoint(ar_hand_skeleton_joint_name_t jointName, const ar_hand_skeleton_t handSkeleton, const simd_float4x4 handTransform, QVector3D &jointPosition, QQuaternion &jointRotation)
{
    bool isTracked = false;
    ar_skeleton_joint_t joint = ar_hand_skeleton_get_joint_named(handSkeleton, jointName);
    if (joint != nullptr) {
        if (ar_skeleton_joint_is_tracked(joint)) {
            simd_float4x4 jointTransform = ar_skeleton_joint_get_anchor_from_joint_transform(joint);
            jointTransform = simd_mul(handTransform, jointTransform);

            QMatrix4x4 transform{jointTransform.columns[0].x, jointTransform.columns[1].x, jointTransform.columns[2].x, jointTransform.columns[3].x,
                                   jointTransform.columns[0].y, jointTransform.columns[1].y, jointTransform.columns[2].y, jointTransform.columns[3].y,
                                   jointTransform.columns[0].z, jointTransform.columns[1].z, jointTransform.columns[2].z, jointTransform.columns[3].z,
                                   0.0f, 0.0f, 0.0f, 1.0f};


            QVector3D jp;
            QVector3D scale;
            QQuaternion jr;
            QSSGUtils::mat44::decompose(transform, jp, scale, jr);

            // NOTE: We need to scale the joint position by 100 to get it into the right scale (m -> cm).
            jointPosition = jp * 100.0f;
            jointRotation = jr;

            isTracked = true;
        }
    }

    return isTracked;
}

using HandJointList = QVarLengthArray<ar_hand_skeleton_joint_name_t, 28>;

const HandJointList &getJointNameTable()
{
    // NOTE: "Joints" are placed in the order from the forarm to the fingers (This might differ from Apple's documentation),
    // moving from the wrist to the finger tips going from the left to the right.
    // That means the forarm is at position 0, and the wrist is at position 1 (this is the origin).

    static const HandJointList jointNames {
        ar_hand_skeleton_joint_name_forearm_arm, // This is the forearm.

        // These are technically the same joint, but depends on the orientation of the hand.
        ar_hand_skeleton_joint_name_wrist, // This is the actual wrist.
        ar_hand_skeleton_joint_name_forearm_wrist,

        // The thumb.
        ar_hand_skeleton_joint_name_thumb_knuckle,
        ar_hand_skeleton_joint_name_thumb_intermediate_base,
        ar_hand_skeleton_joint_name_thumb_intermediate_tip,
        ar_hand_skeleton_joint_name_thumb_tip,

        // The index finger.
        ar_hand_skeleton_joint_name_index_finger_metacarpal,
        ar_hand_skeleton_joint_name_index_finger_knuckle,
        ar_hand_skeleton_joint_name_index_finger_intermediate_base,
        ar_hand_skeleton_joint_name_index_finger_intermediate_tip,
        ar_hand_skeleton_joint_name_index_finger_tip,

        // The middle finger.
        ar_hand_skeleton_joint_name_middle_finger_metacarpal,
        ar_hand_skeleton_joint_name_middle_finger_knuckle,
        ar_hand_skeleton_joint_name_middle_finger_intermediate_base,
        ar_hand_skeleton_joint_name_middle_finger_intermediate_tip,
        ar_hand_skeleton_joint_name_middle_finger_tip,

        // The ring finger.
        ar_hand_skeleton_joint_name_ring_finger_metacarpal,
        ar_hand_skeleton_joint_name_ring_finger_knuckle,
        ar_hand_skeleton_joint_name_ring_finger_intermediate_base,
        ar_hand_skeleton_joint_name_ring_finger_intermediate_tip,
        ar_hand_skeleton_joint_name_ring_finger_tip,

        // The little finger.
        ar_hand_skeleton_joint_name_little_finger_metacarpal,
        ar_hand_skeleton_joint_name_little_finger_knuckle,
        ar_hand_skeleton_joint_name_little_finger_intermediate_base,
        ar_hand_skeleton_joint_name_little_finger_intermediate_tip,
        ar_hand_skeleton_joint_name_little_finger_tip,
    };

    return jointNames;
}

static qsizetype getJointIndex(ar_hand_skeleton_joint_name_t jointName)
{
    qsizetype index = -1;
    const auto &jointNames = getJointNameTable();
    for (size_t i = 0, e = jointNames.size(); i != e && index == -1; ++i) {
        if (jointNames[i] == jointName)
             index = i;
     }

    return index;
}

qsizetype QQuick3DXrInputManagerPrivate::getPokeJointIndex() const
{
    // NOTE: Static for now...
    static const qsizetype idx = getJointIndex(ar_hand_skeleton_joint_name_index_finger_tip);
    return idx;
}

void QQuick3DXrInputManagerPrivate::updateHandtracking()
{
    if (!m_isHandTrackingSupported)
        return;

    QSSG_ASSERT(m_handTrackingProvider != nullptr, return);
    QSSG_ASSERT(m_handAnchors[Hand::LeftHand] != nullptr && m_handAnchors[Hand::RightHand] != nullptr, return);

    ar_hand_tracking_provider_get_latest_anchors(m_handTrackingProvider, m_handAnchors[Hand::LeftHand], m_handAnchors[Hand::RightHand]);

    // FIXME: We can and probably should cache the hand skeleton.
    ar_hand_skeleton_t handSkeletons[2] {};
    uint64_t handJointCount = 0;
    for (const auto hand : { Hand::LeftHand, Hand::RightHand }) {
        handSkeletons[hand] = ar_hand_anchor_get_hand_skeleton(m_handAnchors[hand]);
        handJointCount = qMax(handJointCount, ar_hand_skeleton_get_joint_count(handSkeletons[hand]));
    }

    const auto &jointNames = getJointNameTable();
    // Sanity check the joint count.
    QSSG_CHECK(handJointCount <= size_t(jointNames.size()));

    for (const auto hand : { Hand::LeftHand, Hand::RightHand }) {
        const auto handSkeleton = handSkeletons[hand];
        if (handSkeleton == nullptr) {
            m_handTrackerInputState[hand]->setIsActive(false);
            continue;
        }

        // Clear cached joint data.
        auto &jpositions = jcache[hand].positions;
        auto &jrotations = jcache[hand].rotations;
        jpositions.clear();
        jrotations.clear();

        // NOTE: Separate as we're just sanity checking that the wrist is tracked.
        ar_skeleton_joint_t wristJoinOrigin = ar_hand_skeleton_get_joint_named(handSkeleton, ar_hand_skeleton_joint_name_wrist);
        const bool isWristTracked = ar_skeleton_joint_is_tracked(wristJoinOrigin);

        // The hand transform is relative to the head anchor. The wrist is at the origin of the hand anchor.
        const simd_float4x4 handTransform = ar_anchor_get_origin_from_anchor_transform(m_handAnchors[hand]);

        // Get the joint data.
        for (auto jointName : jointNames) {
            QVector3D jointPosition;
            QQuaternion jointRotation;
            if (setupJoint(jointName, handSkeleton, handTransform, jointPosition, jointRotation)) {
                jpositions.append(jointPosition);
                jrotations.append(jointRotation);
            }
        }

        m_handTrackerInputState[hand]->setJointPositionsAndRotations(jpositions, jrotations);

        m_handInputState[hand]->setIsActive(isWristTracked);
    }
}

QT_END_NAMESPACE
