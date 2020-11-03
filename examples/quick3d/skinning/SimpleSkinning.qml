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

import QtQuick
import QtQuick3D
import Example
import QtQuick.Timeline

Node {
    PrincipledMaterial {
        id: material
        lighting: PrincipledMaterial.NoLighting
        baseColor: "orange"
    }

    Model {
        geometry: SkinGeometry {
            id: geometry

            //! [positions]
            positions: [
                Qt.vector3d(0.0, 0.0, 0.0), // vertex 0
                Qt.vector3d(1.0, 0.0, 0.0), // vertex 1
                Qt.vector3d(0.0, 0.5, 0.0), // vertex 2
                Qt.vector3d(1.0, 0.5, 0.0), // vertex 3
                Qt.vector3d(0.0, 1.0, 0.0), // vertex 4
                Qt.vector3d(1.0, 1.0, 0.0), // vertex 5
                Qt.vector3d(0.0, 1.5, 0.0), // vertex 6
                Qt.vector3d(1.0, 1.5, 0.0), // vertex 7
                Qt.vector3d(0.0, 2.0, 0.0), // vertex 8
                Qt.vector3d(1.0, 2.0, 0.0)  // vertex 9
            ]
            //! [positions]
            //! [joints]
            joints: [
                0, 1, 0, 0, // vertex 0
                0, 1, 0, 0, // vertex 1
                0, 1, 0, 0, // vertex 2
                0, 1, 0, 0, // vertex 3
                0, 1, 0, 0, // vertex 4
                0, 1, 0, 0, // vertex 5
                0, 1, 0, 0, // vertex 6
                0, 1, 0, 0, // vertex 7
                0, 1, 0, 0, // vertex 8
                0, 1, 0, 0  // vertex 9
            ]
            //! [joints]
            //! [weights]
            weights: [
                1.00, 0.00, 0.0, 0.0, // vertex 0
                1.00, 0.00, 0.0, 0.0, // vertex 1
                0.75, 0.25, 0.0, 0.0, // vertex 2
                0.75, 0.25, 0.0, 0.0, // vertex 3
                0.50, 0.50, 0.0, 0.0, // vertex 4
                0.50, 0.50, 0.0, 0.0, // vertex 5
                0.25, 0.75, 0.0, 0.0, // vertex 6
                0.25, 0.75, 0.0, 0.0, // vertex 7
                0.00, 1.00, 0.0, 0.0, // vertex 8
                0.00, 1.00, 0.0, 0.0  // vertex 9
            ]
            //! [weights]
            //! [triangles]
            indexes: [
                0, 1, 3, // triangle 0
                0, 3, 2, // triangle 1
                2, 3, 5, // triangle 2
                2, 5, 4, // triangle 3
                4, 5, 7, // triangle 4
                4, 7, 6, // triangle 5
                6, 7, 9, // triangle 6
                6, 9, 8  // triangle 7
            ]
            //! [triangles]
        }
        materials: [
            material
        ]

        //! [skeleton]
        skeleton: Skeleton {
            id: qmlskeleton
            Joint {
                id: joint0
                index: 0
                skeletonRoot: qmlskeleton
                Joint {
                    id: joint1
                    index: 1
                    skeletonRoot: qmlskeleton
                    eulerRotation.z: 45
                }
            }
        }
        //! [skeleton]
        //! [poses]
        inverseBindPoses: [
            Qt.matrix4x4(1, 0, 0, -0.5,
                         0, 1, 0, -1,
                         0, 0, 1, 0,
                         0, 0, 0, 1),
            Qt.matrix4x4(1, 0, 0, -0.5,
                         0, 1, 0, -1,
                         0, 0, 1, 0,
                         0, 0, 0, 1)
        ]
        //! [poses]
    }

    //! [animation]
    Timeline {
        id: timeline0
        startFrame: 0
        endFrame: 1000
        currentFrame: 0
        enabled: true
        animations: [
            TimelineAnimation {
                duration: 5000
                from: 0
                to: 1000
                running: true
            }
        ]

        KeyframeGroup {
            target: joint1
            property: "eulerRotation.z"

            Keyframe {
                frame: 0
                value: 0
            }
            Keyframe {
                frame: 250
                value: 90
            }
            Keyframe {
                frame: 750
                value: -90
            }
            Keyframe {
                frame: 1000
                value: 0
            }
        }
    }
    //! [animation]
}
