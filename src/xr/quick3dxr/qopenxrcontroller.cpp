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
    disconnect(m_inputActionConnection);

    if (m_controller == ControllerLeft) {
        setPosition(m_inputManager->leftHandInput()->posePosition());
        setRotation(m_inputManager->leftHandInput()->poseRotation());
        setVisible(m_inputManager->leftHandInput()->isActive());

        m_posePositionConnection = connect(m_inputManager->leftHandInput(), &QOpenXRHandInput::posePositionChanged, this, [this]{
            setPosition(m_inputManager->leftHandInput()->posePosition());
        });
        m_poseRotationConnection = connect(m_inputManager->leftHandInput(), &QOpenXRHandInput::poseRotationChanged, this, [this]{
            setRotation(m_inputManager->leftHandInput()->poseRotation());
        });
        m_isActiveConnection = connect(m_inputManager->leftHandInput(), &QOpenXRHandInput::isActiveChanged, this, [this]{
           setVisible(m_inputManager->leftHandInput()->isActive());
        });
        m_inputActionConnection = connect(m_inputManager->leftHandInput(), &QOpenXRHandInput::inputValueChange,
                                          this, [this](int id, const char *shortName, float value) {
            if (m_actionMapper)
                emit m_actionMapper->inputValueChange(QOpenXRActionMapper::InputAction(id), QString::fromLatin1(shortName), value);
        });
    } else if (m_controller == ControllerRight) {
        setPosition(m_inputManager->rightHandInput()->posePosition());
        setRotation(m_inputManager->rightHandInput()->poseRotation());
        setVisible(m_inputManager->rightHandInput()->isActive());
        m_posePositionConnection = connect(m_inputManager->rightHandInput(), &QOpenXRHandInput::posePositionChanged, this, [this]{
            setPosition(m_inputManager->rightHandInput()->posePosition());
        });
        m_poseRotationConnection = connect(m_inputManager->rightHandInput(), &QOpenXRHandInput::poseRotationChanged, this, [this]{
            setRotation(m_inputManager->rightHandInput()->poseRotation());
        });
        m_isActiveConnection = connect(m_inputManager->rightHandInput(), &QOpenXRHandInput::isActiveChanged, this, [this]{
           setVisible(m_inputManager->rightHandInput()->isActive());
        });
        m_inputActionConnection = connect(m_inputManager->rightHandInput(), &QOpenXRHandInput::inputValueChange,
                                          this, [this](int id, const char *shortName, float value){
            if (m_actionMapper)
                emit m_actionMapper->inputValueChange(QOpenXRActionMapper::InputAction(id), QString::fromLatin1(shortName), value);
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

QOpenXRActionMapper *QOpenXRController::actionMapper() const
{
    return m_actionMapper;
}

void QOpenXRController::setActionMapper(QOpenXRActionMapper *newActionMapper)
{
    if (m_actionMapper == newActionMapper)
        return;

    if (m_actionMapperConnection) {
        QObject::disconnect(m_actionMapperConnection);
        m_actionMapperConnection = {};
    }

    if (newActionMapper)
        m_actionMapperConnection = QObject::connect(newActionMapper, &QObject::destroyed, this, [this](QObject *destroyedMapper) {
            if (m_actionMapper == destroyedMapper) {
                m_actionMapper = nullptr;
                emit actionMapperChanged();
            }
        });

    m_actionMapper = newActionMapper;
    emit actionMapperChanged();
}

QT_END_NAMESPACE
