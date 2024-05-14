// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrgamepadinput_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype XrGamepadInput
    \inherits Item
    \inqmlmodule QtQuick3D.Xr
    \brief Enables gamepad input.

    \note This is unrepeatable in QML. It is created under the hood by an XrView.
*/

QOpenXRGamepadInput::QOpenXRGamepadInput(QObject *parent) : QObject(parent)
{

}

/*!
    \qmlsignal XrGamepadInput::inputValueChange(int id, const char *shortName, float value);
    Emitted when the inputValue property changes, provides the \a id, \a shortName
    ,and \a value parameters.
*/

QT_END_NAMESPACE
