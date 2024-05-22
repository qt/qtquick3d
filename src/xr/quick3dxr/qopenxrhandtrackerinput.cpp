// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrhandtrackerinput_p.h"

#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
#include "qopenxrhelpers_p.h"
#include "qopenxrinputmanager_p.h"
#endif

QT_BEGIN_NAMESPACE

QOpenXRHandTrackerInput::QOpenXRHandTrackerInput(QObject *parent)
    : QObject(parent)
{

}

bool QOpenXRHandTrackerInput::isActive() const
{
    return m_isActive;
}

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

const QVector3D &QOpenXRHandTrackerInput::posePosition() const
{
    return m_posePosition;
}

const QQuaternion &QOpenXRHandTrackerInput::poseRotation() const
{
    return m_poseRotation;
}

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

QOpenXRHandTrackerInput *QOpenXrHandModel::handTracker() const
{
    return m_handTracker;
}

void QOpenXrHandModel::setHandTracker(QOpenXRHandTrackerInput *newHandTracker)
{
    if (m_handTracker == newHandTracker)
        return;
    m_handTracker = newHandTracker;
    //TODO: setupModel()
    emit handTrackerChanged();
}

QT_END_NAMESPACE
