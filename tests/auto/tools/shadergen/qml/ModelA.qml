// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick3D

Model {
    DefaultMaterialA {
        id: defMatARefChld
        diffuseColor: "green"
        opacity: 0.0
    }

    source: "#Cube"
    materials: defMatARefChld
}
