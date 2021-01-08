/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick3D

View3D {
    DefaultMaterialA {
        id: defAMatUnused
    }

    DefaultMaterialA {
        id: defAMatOne
    }

    DefaultMaterialA {
        id: defAMatTwo
    }

    // Model component using inherited material (1)
    ModelA {
        opacity: 0.5
    }

    // Model using referenced material component (1)
    Model {
        source: "#Cube"
        materials: defAMatOne // Script binding -> materials -> Identifier: defAMatOne
    }

    // Model using inline material component (1)
    Model {
        materials: DefaultMaterialA { } // Object binding -> materials: DefaultMaterialA
        source: "#Cube"
    }

    // Component model overriding materials with refed material (1)
    ModelA {
        materials: [ defAMatOne ] // Script binding -> materials -> [ -> Identifier: defAMatOne ]
    }

    // Component model overriding materials with inline material (1)
    ModelA {
        materials: [ DefaultMaterial { } ] // Array binding(s) -> materials: [ ... ]
    }

    // Component model overriding materials with refed materials (2)
    ModelA {
        materials: [ defAMatOne, defAMatTwo ] // Script binding -> materials -> [ -> Identifier: defAMatOne, (...) defAMatTwo ]
    }

    // Component model overriding materials with inline materials (2)
    ModelA {
        id: modelWithInlineMaterials
        materials: [ DefaultMaterialA { // Array binding(s) -> materials: [ ... ]
                id: oneInline
            }, DefaultMaterialA {
                id: twoInline
            } ]
    }
}
