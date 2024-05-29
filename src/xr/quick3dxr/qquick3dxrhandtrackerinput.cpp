// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrinputmanager_p.h"
#include "qquick3dxrhandtrackerinput_p.h"
#include "qquick3dxrhandtrackerinput_p_p.h"

#if defined(Q_OS_VISIONOS)
# include "visionos/qquick3dxrinputmanager_visionos_p.h"
#else
# include "openxr/qopenxrinputmanager_p.h"
#endif

QT_BEGIN_NAMESPACE

QQuick3DXrHandTrackerInputPrivate::QQuick3DXrHandTrackerInputPrivate(QQuick3DXrHandTrackerInput &handTrackerInput)
    : q_ptr(&handTrackerInput)
{

}

bool QQuick3DXrHandTrackerInputPrivate::isActive() const
{
    return m_isActive;
}

QQuick3DXrHandTrackerInputPrivate::HandPoseSpace QQuick3DXrHandTrackerInputPrivate::poseSpace() const
{
    return m_poseSpace;
}

void QQuick3DXrHandTrackerInputPrivate::setIsActive(bool isActive)
{
    Q_Q(QQuick3DXrHandTrackerInput);

    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    emit q->isActiveChanged();
}

void QQuick3DXrHandTrackerInputPrivate::setPoseSpace(QQuick3DXrHandTrackerInputPrivate::HandPoseSpace poseSpace)
{
    Q_Q(QQuick3DXrHandTrackerInput);

    if (poseSpace == m_poseSpace)
        return;

    m_poseSpace = poseSpace;
    emit q->poseSpaceChanged();
}

const QVector3D &QQuick3DXrHandTrackerInputPrivate::posePosition() const
{
    return m_posePosition;
}

const QQuaternion &QQuick3DXrHandTrackerInputPrivate::poseRotation() const
{
    return m_poseRotation;
}

QList<QVector3D> QQuick3DXrHandTrackerInputPrivate::jointPositions() const
{
    return m_jointPositions;
}

void QQuick3DXrHandTrackerInputPrivate::setJointPositionsAndRotations(const QList<QVector3D> &newJointPositions, const QList<QQuaternion> &newJointRotations)
{
    Q_Q(QQuick3DXrHandTrackerInput);

    m_jointPositions = newJointPositions;
    emit q->jointPositionsChanged();
    m_jointRotations = newJointRotations;
    emit q->jointRotationsChanged();
    emit q->jointDataUpdated();

    QQuick3DXrInputManager *inputMan = QQuick3DXrInputManager::instance();
    const auto pokeIndex = QQuick3DXrInputManagerPrivate::get(inputMan)->getPokeJointIndex();

    if (pokeIndex >= 0 && pokeIndex < m_jointPositions.size())
        setPokePosition(m_jointPositions[pokeIndex]);
}

QList<QQuaternion> QQuick3DXrHandTrackerInputPrivate::jointRotations() const
{
    return m_jointRotations;
}

QVector3D QQuick3DXrHandTrackerInputPrivate::pokePosition() const
{
    return m_pokePosition;
}

void QQuick3DXrHandTrackerInputPrivate::setPokePosition(const QVector3D &newPokePosition)
{
    Q_Q(QQuick3DXrHandTrackerInput);

    if (m_pokePosition == newPokePosition)
        return;
    m_pokePosition = newPokePosition;
    emit q->pokePositionChanged();
}

/*!
    \qmltype XrHandTrackerInput
    \inqmlmodule QtQuick3D.Xr
    \brief Represents hand tracking input for XR (extended reality) applications.

    The XrHandTrackerInput type provides information about hand poses, joint positions,
    and other relevant data for hand tracking.
*/

QQuick3DXrHandTrackerInput::QQuick3DXrHandTrackerInput(QObject *parent)
    : QObject(parent)
    , d_ptr(new QQuick3DXrHandTrackerInputPrivate(*this))
{

}

QQuick3DXrHandTrackerInput::~QQuick3DXrHandTrackerInput()
{

}

/*!
    \qmlproperty bool XrHandTrackerInput::isActive
    \brief  Indicates whether hand tracking is active.
*/

bool QQuick3DXrHandTrackerInput::isActive() const
{
    Q_D(const QQuick3DXrHandTrackerInput);
    return d->isActive();
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

QQuick3DXrHandTrackerInput::HandPoseSpace QQuick3DXrHandTrackerInput::poseSpace() const
{
    Q_D(const QQuick3DXrHandTrackerInput);

    return d->poseSpace();
}

void QQuick3DXrHandTrackerInput::setIsActive(bool isActive)
{
    Q_D(QQuick3DXrHandTrackerInput);
    d->setIsActive(isActive);
}

/*!
    \qmlproperty vector3d XrHandTrackerInput::posePosition
    \brief The position of the hand pose.
*/

void QQuick3DXrHandTrackerInput::setPoseSpace(QQuick3DXrHandTrackerInput::HandPoseSpace poseSpace)
{
    Q_D(QQuick3DXrHandTrackerInput);
    d->setPoseSpace(poseSpace);
}

const QVector3D &QQuick3DXrHandTrackerInput::posePosition() const
{
    Q_D(const QQuick3DXrHandTrackerInput);
    return d->posePosition();
}

/*!
    \qmlproperty list<vector3d> XrHandTrackerInput::jointPositions
    \brief List of joint positions for the hand.
*/

QList<QVector3D> QQuick3DXrHandTrackerInput::jointPositions() const
{
    Q_D(const QQuick3DXrHandTrackerInput);
    return d->jointPositions();
}

const QQuaternion &QQuick3DXrHandTrackerInput::poseRotation() const
{
    Q_D(const QQuick3DXrHandTrackerInput);
    return d->poseRotation();
}

/*!
    \qmlproperty list<quaternion> XrHandTrackerInput::jointRotations
    \brief List of joint rotations for the hand.
*/

QList<QQuaternion> QQuick3DXrHandTrackerInput::jointRotations() const
{
    Q_D(const QQuick3DXrHandTrackerInput);
    return d->jointRotations();
}

void QQuick3DXrHandTrackerInput::setJointPositionsAndRotations(const QList<QVector3D> &newJointPositions, const QList<QQuaternion> &newJointRotations)
{
    Q_D(QQuick3DXrHandTrackerInput);
    d->setJointPositionsAndRotations(newJointPositions, newJointRotations);
}

QVector3D QQuick3DXrHandTrackerInput::pokePosition() const
{
    Q_D(const QQuick3DXrHandTrackerInput);
    return d->pokePosition();
}

void QQuick3DXrHandTrackerInput::setPokePosition(const QVector3D &newPokePosition)
{
    Q_D(QQuick3DXrHandTrackerInput);
    d->setPokePosition(newPokePosition);
}

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

QQuick3DXrHandModel::QQuick3DXrHandModel(QQuick3DNode *parent)
    : QQuick3DModel(parent)
{

}

void QQuick3DXrHandModel::updatePose()
{
    if (auto *skin = QQuick3DModel::skin()) {
        auto jointListProp = skin->joints();
        int count = jointListProp.count(&jointListProp);
        const auto positions = m_handTracker->jointPositions();
        const auto rotations = m_handTracker->jointRotations();
        for (int i = 0; i < count; ++i) {
            auto *joint = jointListProp.at(&jointListProp, i);
            joint->setPosition(positions.at(i));
            joint->setRotation(rotations.at(i));
        }
    } else {
        static bool warned = false;
        if (!warned) {
            qWarning() << "No skin available for hand model";
            warned = true;
        }
    }
}

void QQuick3DXrHandModel::setupModel()
{
    if (!m_handTracker) {
        return;
    }
    if (m_initialized) {
        qWarning() << "XrHandModel does not support changing hand tracker";
        return;
    }
    QQuick3DXrInputManager *inputMan = QQuick3DXrInputManager::instance();

    QQuick3DXrInputManagerPrivate::get(inputMan)->setupHandModel(this);

    connect(m_handTracker, &QQuick3DXrHandTrackerInput::jointDataUpdated, this, &QQuick3DXrHandModel::updatePose);
    connect(m_handTracker, &QQuick3DXrHandTrackerInput::isActiveChanged, this, [this](){
        setVisible(m_handTracker->isActive());
    });
    setVisible(m_handTracker->isActive());
    m_initialized = true;
}

void QQuick3DXrHandModel::componentComplete()
{
    setupModel();
    QQuick3DModel::componentComplete();
}

/*!
    \qmlproperty XrHandTrackerInput XrHandModel::handTracker
    \brief Contains the XrHandTrackerInput that animates this model.

    \warning Changing hand trackers is not supported.
*/

QQuick3DXrHandTrackerInput *QQuick3DXrHandModel::handTracker() const
{
    return m_handTracker;
}

void QQuick3DXrHandModel::setHandTracker(QQuick3DXrHandTrackerInput *newHandTracker)
{
    if (m_handTracker == newHandTracker)
        return;
    m_handTracker = newHandTracker;
    //TODO: setupModel()
    emit handTrackerChanged();
}

/*!
    \qmlsignal XrHandModel::handChanged()
    Emitted when the associated hand changes.
*/

/*!
    \qmlsignal XrHandModel::handTrackerChanged()
    Emitted when the handTracker property changes.
*/


QT_END_NAMESPACE
