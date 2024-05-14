// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrhandtrackerinput_p.h"

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
#include "openxr/qopenxrhelpers_p.h"
#include "qopenxrinputmanager_p.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrHandTrackerInput
    \inqmlmodule QtQuick3D.Xr
    \brief Represents hand tracking input for XR (extended reality) applications.

    The XrHandTrackerInput type provides information about hand poses, joint positions,
    and other relevant data for hand tracking.
*/

QOpenXRHandTrackerInput::QOpenXRHandTrackerInput(QObject *parent)
    : QObject(parent)
{

}

/*!
    \qmlproperty bool XrHandTrackerInput::isActive
    \brief  Indicates whether hand tracking is active.
*/

bool QOpenXRHandTrackerInput::isActive() const
{
    return m_isActive;
}

/*!
    \qmlproperty enumeration XrHandTrackerInput::poseSpace
    \brief Specifies the space in which hand poses are defined.

    It can be one of:
    \value XrHandTrackerInput.GripPose
    \value XrHandTrackerInput.AimPose
    \value XrHandTrackerInput.PinchPose
    \value XrHandTrackerInput.PokePose
*/

QOpenXRHandTrackerInput::HandPoseSpace QOpenXRHandTrackerInput::poseSpace() const
{
    return m_poseSpace;
}

void QOpenXRHandTrackerInput::setIsActive(bool isActive)
{
    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    emit isActiveChanged();
}

void QOpenXRHandTrackerInput::setPoseSpace(QOpenXRHandTrackerInput::HandPoseSpace poseSpace)
{
    if (poseSpace == m_poseSpace)
        return;

    m_poseSpace = poseSpace;
    emit poseSpaceChanged();
}

/*!
    \qmlproperty vector3d XrHandTrackerInput::posePosition
    \brief The position of the hand pose.
*/

const QVector3D &QOpenXRHandTrackerInput::posePosition() const
{
    return m_posePosition;
}

const QQuaternion &QOpenXRHandTrackerInput::poseRotation() const
{
    return m_poseRotation;
}

/*!
    \qmlproperty list<vector3d> XrHandTrackerInput::jointPositions
    \brief List of joint positions for the hand.
*/

QList<QVector3D> QOpenXRHandTrackerInput::jointPositions() const
{
    return m_jointPositions;
}

void QOpenXRHandTrackerInput::setJointPositionsAndRotations(const QList<QVector3D> &newJointPositions, const QList<QQuaternion> &newJointRotations)
{
    m_jointPositions = newJointPositions;
    emit jointPositionsChanged();
    m_jointRotations = newJointRotations;
    emit jointRotationsChanged();
    emit jointDataUpdated();
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    setPokePosition(m_jointPositions[XR_HAND_JOINT_INDEX_TIP_EXT]);
#endif
}

/*!
    \qmlproperty list<quaternion> XrHandTrackerInput::jointRotations
    \brief List of joint rotations for the hand.
*/

QList<QQuaternion> QOpenXRHandTrackerInput::jointRotations() const
{
    return m_jointRotations;
}

QVector3D QOpenXRHandTrackerInput::pokePosition() const
{
    return m_pokePosition;
}

void QOpenXRHandTrackerInput::setPokePosition(const QVector3D &newPokePosition)
{
    if (m_pokePosition == newPokePosition)
        return;
    m_pokePosition = newPokePosition;
    emit pokePositionChanged();
}

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
static inline QMatrix4x4 transformMatrix(const QVector3D &position, const QQuaternion &rotation)
{
    QMatrix4x4 transform = QMatrix4x4{rotation.toRotationMatrix()};

    transform(0, 3) += position[0];
    transform(1, 3) += position[1];
    transform(2, 3) += position[2];

    return transform;
}
#endif

 /*!
    \qmlsignal XrHandTrackerInput::isActiveChanged()
    Emitted when the isActive property changes.
*/

 /*!
    \qmlsignal XrHandTrackerInput::poseSpaceChanged()
    Emitted when the poseSpace property changes.
*/

 /*!
    \qmlsignal XrHandTrackerInput::posePositionChanged()
    Emitted when the posePosition property changes.
*/

 /*!
    \qmlsignal XrHandTrackerInput::jointPositionsChanged()
    Emitted when the jointPositions property changes.
*/

 /*!
    \qmlsignal XrHandTrackerInput::jointDataUpdated()
    Emitted when joint data (positions or rotations) is updated.
*/

 /*!
    \qmlsignal XrHandTrackerInput::jointRotationsChanged()
    Emitted when the jointRotations property changes.
*/

 /*!
    \qmlsignal XrHandTrackerInput::pokePositionChanged()
    Emitted when the pokePosition property changes.
*/

/*!
    \qmltype XrHandModel
    \inherits Model
    \inqmlmodule QtQuick3D.Xr
    \brief Represents a 3D model for a hand.

    Contains a 3D model that is animated by a XrHandTrackerInput.
*/

QOpenXrHandModel::QOpenXrHandModel(QQuick3DNode *parent)
    : QQuick3DModel(parent)
{

}

void QOpenXrHandModel::updatePose()
{
    auto *skin = QQuick3DModel::skin();
    auto jointListProp = skin->joints();
    int count = jointListProp.count(&jointListProp);
    const auto positions = m_handTracker->jointPositions();
    const auto rotations = m_handTracker->jointRotations();
    for (int i = 0; i < count; ++i) {
        auto *joint = jointListProp.at(&jointListProp, i);
        joint->setPosition(positions.at(i));
        joint->setRotation(rotations.at(i));
    }
}

void QOpenXrHandModel::setupModel()
{
    if (!m_handTracker) {
        return;
    }
    if (m_initialized) {
        qWarning() << "XrHandModel does not support changing hand tracker";
        return;
    }
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    Q_ASSERT(QOpenXRInputManager::instance());
    auto *inputMan = QOpenXRInputManager::instance();
    QOpenXRInputManager::Hand hand = m_handTracker == inputMan->leftHandTrackerInput()
            ? QOpenXRInputManager::Hand::LeftHand : QOpenXRInputManager::Hand::RightHand;
    const auto &handGeometryData = inputMan->m_handGeometryData[hand];
    const auto &handMeshData = inputMan->m_handMeshData[hand];

    if (!handGeometryData.geometry)
        return;

    setGeometry(handGeometryData.geometry);

    //TODO: support changing hands

    auto *skin = new QQuick3DSkin(this);
    auto jointListProp = skin->joints();
    QList<QMatrix4x4> inverseBindPoses;
    inverseBindPoses.reserve(XR_HAND_JOINT_COUNT_EXT);

    for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
        const auto &pose = handMeshData.jointBindPoses[i];
        const QVector3D pos = OpenXRHelpers::toQVector(pose.position);
        const QQuaternion rot = OpenXRHelpers::toQQuaternion(pose.orientation);
        inverseBindPoses.append(transformMatrix(pos, rot).inverted());
        auto *joint = new QQuick3DNode(this);
        joint->setPosition(pos);
        joint->setRotation(rot);
        jointListProp.append(&jointListProp, joint);
    }
    skin->setInverseBindPoses(inverseBindPoses);
    setSkin(skin);

    connect(m_handTracker, &QOpenXRHandTrackerInput::jointDataUpdated, this, &QOpenXrHandModel::updatePose);
    connect(m_handTracker, &QOpenXRHandTrackerInput::isActiveChanged, this, [this](){
        setVisible(m_handTracker->isActive());
    });
    setVisible(m_handTracker->isActive());
    m_initialized = true;
#endif
}

void QOpenXrHandModel::componentComplete()
{
    setupModel();
    QQuick3DModel::componentComplete();
}

/*!
    \qmlproperty XrHandTrackerInput XrHandModel::handTracker
    \brief Contains the XrHandTrackerInput that animates this model.

    \warning Changing hand trackers is not supported.
*/

QOpenXRHandTrackerInput *QOpenXrHandModel::handTracker() const
{
    return m_handTracker;
}

/*!
    \qmlsignal XrHandModel::handChanged()
    Emitted when the associated hand changes.
*/

/*!
    \qmlsignal XrHandModel::handTrackerChanged()
    Emitted when the handTracker property changes.
*/

void QOpenXrHandModel::setHandTracker(QOpenXRHandTrackerInput *newHandTracker)
{
    if (m_handTracker == newHandTracker)
        return;
    m_handTracker = newHandTracker;
    //TODO: setupModel()
    emit handTrackerChanged();
}

QT_END_NAMESPACE
