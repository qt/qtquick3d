// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrhelpers_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

QString OpenXRHelpers::getXrResultAsString(XrResult result, XrInstance instance)
{
    QByteArray errorString(XR_MAX_RESULT_STRING_SIZE, 0);
    xrResultToString(instance, result, errorString.data());
    errorString.resize(qstrlen(errorString.constData()));
    return QString::fromUtf8(errorString).trimmed();
}

bool OpenXRHelpers::checkXrResult(XrResult result, XrInstance instance)
{
    if (result != XrResult::XR_SUCCESS) {
        qWarning().noquote().nospace() << "OpenXR call failed (" << result << "): " << getXrResultAsString(result, instance);
        return false;
    }
    return true;
}

QT_END_NAMESPACE
