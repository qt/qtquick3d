/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

CustomMaterial {
    shadingMode: CustomMaterial.Unshaded
    vertexShader: "material.vert"
    fragmentShader: "material.frag"
    property var uVarInt: 60
    property bool uBoolFalse: false
    property bool uBoolTrue: true
    property int uInt: 33
    property real uReal: 3.3
    property point uPointS: "0.0,1.0"
    property point uPointF: Qt.point(1,0);
    property size uSizeS: "1x1"
    property size uSizeF: Qt.size(2,2)
    property rect uRectS: "0,1,100x101"
    property rect uRectF: Qt.rect(1,0,101,100)
    property vector2d uVec2: Qt.vector2d(1.0, 2.0)
    property vector3d uVec3: Qt.vector3d(1.0, 2.0, 3.0)
    property vector4d uVec4: Qt.vector4d(1.0, 2.0, 3.0, 4.0)
    property quaternion uQuat: Qt.quaternion(1, 2, 3, 4)
    property matrix4x4 uM44: Qt.matrix4x4(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)
    property color uColor1: "green"
    property color uColor2: "#ff0000"
    property Texture uTex: Texture { }
    property TextureInput uTexInput: TextureInput { texture: Texture { } }
}
