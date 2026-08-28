// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// This file exists for the sole purpose of letting qmlimportscanner (and thus
// androiddeployqt) discover which QML modules need to be deployed. The test
// only expresses these imports in C++ via QQmlComponent::setData(), which the
// import scanner cannot see, so on deployment-based platforms such as Android
// the QtQuick3D plugin would otherwise not be bundled.

import QtQuick
import QtQuick3D

QtObject { }
