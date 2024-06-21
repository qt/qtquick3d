// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qquick3dxrcontroller_p.h"

#include "qquick3dxrinputmanager_p.h"

#if !defined(Q_OS_VISIONOS)
# include "openxr/qopenxrinputmanager_p.h"
#endif

#include "qquick3dxrview_p.h"
#include "qquick3dxractionmapper_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrController
    \inherits Node
    \inqmlmodule QtQuick3D.Xr
    \brief A type enabling the use of different controller inputs.

*/

QQuick3DXrController::QQuick3DXrController()
{
    m_inputManager = QQuick3DXrInputManager::instance();
}

/*!
    \qmlproperty enumeration QtQuick3D.Xr::XrController::controller

    Holds the controller.

    It can be one of:
    \value XrController.ControllerNone
    \value XrController.ControllerLeft
    \value XrController.ControllerRight
*/

QQuick3DXrController::Controller QQuick3DXrController::controller() const
{
    return m_controller;
}

void QQuick3DXrController::setController(QQuick3DXrController::Controller newController)
{
    if (m_controller == newController)
        return;
    m_controller = newController;
    emit controllerChanged();
    emit handInputChanged();

    disconnect(m_posePositionConnection);
    disconnect(m_poseRotationConnection);
    disconnect(m_isActiveConnection);
    disconnect(m_inputActionConnection);

    auto *input = handInput();
    if (input) {
        input->setPoseSpace(static_cast<QQuick3DXrHandInput::HandPoseSpace>(m_poseSpace));
        setPosition(input->posePosition());
        setRotation(input->poseRotation());
        setVisible(input->isActive());

        m_posePositionConnection = connect(input, &QQuick3DXrHandInput::posePositionChanged, this, [this, input]{
            setPosition(input->posePosition());
        });
        m_poseRotationConnection = connect(input, &QQuick3DXrHandInput::poseRotationChanged, this, [this, input]{
            setRotation(input->poseRotation());
        });
        m_isActiveConnection = connect(input, &QQuick3DXrHandInput::isActiveChanged, this, [this, input]{
            setVisible(input->isActive());
        });

        connect(input, &QQuick3DXrHandInput::pokePositionChanged, this, &QQuick3DXrController::pokePositionChanged);
        connect(input, &QQuick3DXrHandInput::jointPositionsChanged, this, &QQuick3DXrController::jointPositionsChanged);
        connect(input, &QQuick3DXrHandInput::jointRotationsChanged, this, &QQuick3DXrController::jointRotationsChanged);
        connect(input, &QQuick3DXrHandInput::jointDataUpdated, this, &QQuick3DXrController::jointDataUpdated);

        //### TODO invoke the action mapper directly from input manager
        m_inputActionConnection = connect(input, &QQuick3DXrHandInput::inputValueChange,
                                          this, [this](int id, const char *shortName, float value) {
            QQuick3DXrActionMapper::handleInput(QQuick3DXrInputAction::Action(id), QQuick3DXrInputAction::Hand(m_controller), shortName, value);
        });

    } else {
        setVisible(false);
    }
}

/*!
    \qmlproperty XrHandInput XrController::handInput
    The hand input associated with this controller.
*/

QQuick3DXrHandInput *QQuick3DXrController::handInput() const
{
    if (m_inputManager && m_inputManager->isValid()) {
        if (m_controller == ControllerRight)
            return m_inputManager->rightHandInput();
        else if (m_controller == ControllerLeft)
            return m_inputManager->leftHandInput();
    }

    return nullptr;
}

QQuick3DXrController::HandPoseSpace QQuick3DXrController::poseSpace() const
{
    return m_poseSpace;
}

void QQuick3DXrController::setPoseSpace(HandPoseSpace newPoseSpace)
{
    if (m_poseSpace == newPoseSpace)
        return;
    m_poseSpace = newPoseSpace;
    auto *input = handInput();
    if (input)
        input->setPoseSpace(static_cast<QQuick3DXrHandInput::HandPoseSpace>(m_poseSpace));
    emit poseSpaceChanged();
}

QVector3D QQuick3DXrController::pokePosition() const
{
    auto *input = handInput();
    if (input)
        return input->pokePosition();
    return {};
}

QList<QVector3D> QQuick3DXrController::jointPositions() const
{
    auto *input = handInput();
    if (input)
        return input->jointPositions();
    return {};
}

QList<QQuaternion> QQuick3DXrController::jointRotations() const
{
    auto *input = handInput();
    if (input)
        return input->jointRotations();
    return {};
}

bool QQuick3DXrController::isActive() const
{
    return m_isActive;
}

QT_END_NAMESPACE
