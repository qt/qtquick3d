// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrcontroller_p.h"

#include "qquick3dxrinputmanager_p.h"

#if !defined(Q_OS_VISIONOS)
# include "openxr/qopenxrinputmanager_p.h"
#endif

#include "qquick3dxrview_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrController
    \inherits Node
    \inqmlmodule QtQuick3D.Xr
    \brief A type enabling the use of different controller inputs.

*/

QOpenXRController::QOpenXRController()
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

        m_posePositionConnection = connect(input, &QQuick3DXrHandInput::posePositionChanged, this, [this, input]{
            setPosition(input->posePosition());
        });
        m_poseRotationConnection = connect(input, &QQuick3DXrHandInput::poseRotationChanged, this, [this, input]{
            setRotation(input->poseRotation());
        });
        m_isActiveConnection = connect(input, &QQuick3DXrHandInput::isActiveChanged, this, [this, input]{
            setVisible(input->isActive());
        });
        m_inputActionConnection = connect(input, &QQuick3DXrHandInput::inputValueChange,
                                          this, [this](int id, const char *shortName, float value) {
                                              if (m_actionMapper)
                                                  m_actionMapper->handleInput(QOpenXRActionMapper::InputAction(id), shortName, value);
                                          });
    } else {
        setVisible(false);
    }
}

/*!
    \qmlproperty XrHandInput XrController::handInput
    The hand input associated with this controller.
*/

QQuick3DXrHandInput *QOpenXRController::handInput() const
{
    if (m_inputManager && m_inputManager->isValid()) {
        if (m_controller == ControllerRight)
            return m_inputManager->rightHandInput();
        else if (m_controller == ControllerLeft)
            return m_inputManager->leftHandInput();
    }

    return nullptr;
}

/*!
    \qmlproperty XrActionMapper XrController::actionMapper
    The action mapper associated with this controller.
*/

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

/*!
    \qmlsignal XrController::controllerChanged()
    Emitted when the controller property changes.
*/

/*!
    \qmlsignal XrController::handInputChanged()
    Emitted when the handInput property changes.
*/

/*!
    \qmlsignal XrController::actionMapperChanged()
    Emitted when the actionMapper property changes.
*/
QT_END_NAMESPACE
