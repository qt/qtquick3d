// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrcontroller_p.h"
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
# include "qopenxrinputmanager_p.h"
#endif

#include "qquick3dxrview_p.h"

QT_BEGIN_NAMESPACE

QOpenXRController::QOpenXRController()
{
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    m_inputManager = QOpenXRInputManager::instance();
#endif
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

    auto *input = handInput();
    if (input) {
        setPosition(input->posePosition());
        setRotation(input->poseRotation());
        setVisible(input->isActive());

        m_posePositionConnection = connect(input, &QOpenXRHandInput::posePositionChanged, this, [this, input]{
            setPosition(input->posePosition());
        });
        m_poseRotationConnection = connect(input, &QOpenXRHandInput::poseRotationChanged, this, [this, input]{
            setRotation(input->poseRotation());
        });
        m_isActiveConnection = connect(input, &QOpenXRHandInput::isActiveChanged, this, [this, input]{
            setVisible(input->isActive());
        });
        m_inputActionConnection = connect(input, &QOpenXRHandInput::inputValueChange,
                                          this, [this](int id, const char *shortName, float value) {
                                              if (m_actionMapper)
                                                  m_actionMapper->handleInput(QOpenXRActionMapper::InputAction(id), shortName, value);
                                          });
    } else {
        setVisible(false);
        //TODO handle gamepad connections
    }
}

QOpenXRHandInput *QOpenXRController::handInput() const
{
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    if (m_controller == ControllerRight)
        return m_inputManager->rightHandInput();
    else if (m_controller == ControllerLeft)
        return m_inputManager->leftHandInput();
#endif
    return nullptr;
}

QOpenXRGamepadInput *QOpenXRController::gamepadInput() const
{
#if defined(Q_NO_TEMPORARY_DISABLE_XR_API)
    return m_inputManager->gamepadInput();
#else
    return nullptr;
#endif
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
