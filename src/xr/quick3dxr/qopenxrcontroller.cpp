// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrcontroller_p.h"
#include "qopenxrinputmanager_p.h"

#include "qopenxrview_p.h"

QT_BEGIN_NAMESPACE

QOpenXRController::QOpenXRController()
{
    m_inputManager = QOpenXRInputManager::instance();
}

QOpenXRController::Controller QOpenXRController::controller() const
{
    return m_controller;
}

void QOpenXRController::setController(QOpenXRController::Controller newController)
{
    if (m_controller == newController)
        return;
    m_controller = newController;
    emit controllerChanged();
    emit handInputChanged();

    disconnect(m_posePositionConnection);
    disconnect(m_poseRotationConnection);
    disconnect(m_isActiveConnection);

    if (m_controller == ControllerLeft) {
        setPosition(m_inputManager->leftHandInput()->posePosition());
        setRotation(m_inputManager->leftHandInput()->poseRotation());
        setVisible(m_inputManager->leftHandInput()->isActive());

        m_posePositionConnection = connect(m_inputManager->leftHandInput(), &QOpenXRHandInput::posePositionChanged, this, [=]{
            setPosition(m_inputManager->leftHandInput()->posePosition());
        });
        m_poseRotationConnection = connect(m_inputManager->leftHandInput(), &QOpenXRHandInput::poseRotationChanged, this, [=]{
            setRotation(m_inputManager->leftHandInput()->poseRotation());
        });
        m_isActiveConnection = connect(m_inputManager->leftHandInput(), &QOpenXRHandInput::isActiveChanged, this, [=]{
           setVisible(m_inputManager->leftHandInput()->isActive());
        });
    } else if (m_controller == ControllerRight) {
        setPosition(m_inputManager->rightHandInput()->posePosition());
        setRotation(m_inputManager->rightHandInput()->poseRotation());
        setVisible(m_inputManager->rightHandInput()->isActive());
        m_posePositionConnection = connect(m_inputManager->rightHandInput(), &QOpenXRHandInput::posePositionChanged, this, [=]{
            setPosition(m_inputManager->rightHandInput()->posePosition());
        });
        m_poseRotationConnection = connect(m_inputManager->rightHandInput(), &QOpenXRHandInput::poseRotationChanged, this, [=]{
            setRotation(m_inputManager->rightHandInput()->poseRotation());
        });
        m_isActiveConnection = connect(m_inputManager->rightHandInput(), &QOpenXRHandInput::isActiveChanged, this, [=]{
           setVisible(m_inputManager->rightHandInput()->isActive());
        });
    } else {
        setVisible(false);
    }
}

QOpenXRHandInput *QOpenXRController::handInput() const
{
    if (m_controller == ControllerRight)
        return m_inputManager->rightHandInput();
    else if (m_controller == ControllerLeft)
        return m_inputManager->leftHandInput();
    return nullptr;
}

QOpenXRGamepadInput *QOpenXRController::gamepadInput() const
{
    return m_inputManager->gamepadInput();
}

QOpenXRHandInput::HandPoseSpace QOpenXRController::poseSpace() const
{
    const auto input = handInput();
    if (input)
        return input->poseSpace();
    else
        return QOpenXRHandInput::HandPoseSpace::GripPose;
}

void QOpenXRController::setPoseSpace(const QOpenXRHandInput::HandPoseSpace &newPoseSpace)
{
    if (poseSpace() == newPoseSpace)
        return;
    auto input = handInput();
    if (input) {
        input->setPoseSpace(newPoseSpace);
        emit poseSpaceChanged();
    }
}

QT_END_NAMESPACE
