// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Multiview ComputePass test. Root is a plain Node (not View3D) per the
// tests/baseline/data_xr convention: the XR grabber harness supplies the
// XrView/stereo rendering context, and qmlxr_environment is the recognized
// hook for injecting a SceneEnvironment (with effects) into it -- see
// tests/baseline/data_xr/effects/projmatrix.qml for the same pattern.
//
// A ComputePass writes a pass/fail image (see multiview_compute.comp for
// the checks) that an Effect displays full-screen.

import QtQuick
import QtQuick3D

Node {
    property SceneEnvironment qmlxr_environment: SceneEnvironment {
        effects: [ e0 ]
    }

    DirectionalLight {
    }

    Model {
        source: "#Sphere"
        materials: PrincipledMaterial {
            baseColor: "red"
        }
    }

    ShaderImage {
        id: outputImageRes
        width: 400
        height: 400
        format: ShaderImage.RGBA8
    }

    ComputePass {
        computeShader: "multiview_compute.comp"
        property ShaderImage targetImage: outputImageRes
        commands: [
            Dispatch {
                groupCountX: (outputImageRes.width + 7) / 8
                groupCountY: (outputImageRes.height + 7) / 8
                groupCountZ: 1
            }
        ]
    }

    Effect {
        id: e0
        property ShaderImage sourceImage: outputImageRes
        passes: Pass {
            shaders: Shader {
                stage: Shader.Fragment
                shader: "displayimage.frag"
            }
        }
    }
}
