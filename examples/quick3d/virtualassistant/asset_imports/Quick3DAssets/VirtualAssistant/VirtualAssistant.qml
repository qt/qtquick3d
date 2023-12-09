// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D
import QtQuick.Timeline 1.0
import Quick3DAssets.RobotHeart 1.0

Node {
    id: node

    enum ANIMATION {
        ENTRY,
        BACKFLIP,
        BOUNCING,
        RIGHTHAND,
        LEFTHAND,
        EXPLORE,
        EXIT,
        FACE,
        HEART
    }

    property int currentAnim: -1

    property var timelineList: [entryTimeline, backflipTimeline, bouncingTimeline,
        rightHandWavingTimeline, leftHandWavingTimeline, exitTimeline, exploreTimeline,
        faceTimeline, heart.heartTimeline]

    function restoreDefaults() {
        base.x = 0;
        base.z = 0;
        base.y = 0;
        base.eulerRotation.x = 0.12;
        base.eulerRotation.y = 0;
        base.eulerRotation.z = 0;

        loweBody.y = -0.73;
        loweBody.z = -0.25;
        loweBody.eulerRotation.x = -6;
        loweBody.eulerRotation.y = -144;
        loweBody.eulerRotation.z = 175;

        hand_l.x = 1.89;
        hand_l.y = 0.5;
        hand_l.z = 0;
        hand_l.eulerRotation.x = -0.18;
        hand_l.eulerRotation.y = -145;
        hand_l.eulerRotation.z = -178.92;

        hand_r.x = -1.89;
        hand_r.y = 0.5;
        hand_r.z = 0;
        hand_r.eulerRotation.x = 10;
        hand_r.eulerRotation.y = 210;
        hand_r.eulerRotation.z = 185;

        node.state = "";
        node.currentAnim = -1;
    }

    function stopAnimation(timeline) {
        timeline.currentFrame = 0;
        timeline.enabled = false;
    }

    function stopAnimations() {
        node.timelineList.forEach((timeline) => node.stopAnimation(timeline))
        node.restoreDefaults();
    }

    function runAnimation(index: int) {
        stopAnimations();
        currentAnim = index;
    }

    function animateObject(pickedObject: string) {
        node.stopAnimations();
        switch (pickedObject) {
        case "rightHand":
            currentAnim = VirtualAssistant.ANIMATION.RIGHTHAND;
            break;
        case "leftHand":
            currentAnim = VirtualAssistant.ANIMATION.LEFTHAND;
            break;
        case "lowerBody":
            currentAnim = VirtualAssistant.ANIMATION.BOUNCING;
            break;
        case "face":
            currentAnim = VirtualAssistant.ANIMATION.FACE;
            break;
        case "chest":
            currentAnim = VirtualAssistant.ANIMATION.HEART;
            break;
        default:
            break;
        }
    }

    Skin {
        id: skin
        joints: [
            base,
            chest,
            loweBody,
            head,
            face,
            hand_l,
            hand_l_thumb1,
            hand_l_thumb2,
            hand_l_index1,
            hand_l_index2,
            hand_l_middle1,
            hand_l_middle2,
            hand_l_pinky1,
            hand_l_pinky2,
            hand_r,
            hand_r_thumb1,
            hand_r_thumb2,
            hand_r_index1,
            hand_r_index2,
            hand_r_middle1,
            hand_r_middle2,
            hand_r_pinky1,
            hand_r_pinky2
        ]
        inverseBindPoses: [
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, -1.61245, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(0.813694, 0.0862685, -0.574857, -0.218272, 0.0100981, -0.990875, -0.134407, 0.844787, -0.581196, 0.103562, -0.807129, -0.29807, 0, 0, 0, 1),
            Qt.matrix4x4(1, 1.1016e-07, -3.48602e-07, 1.04242e-06, -1.10184e-07, 1, -6.87816e-05, -3.40188, 3.48594e-07, 6.87816e-05, 1, 0.302907, 0, 0, 0, 1),
            Qt.matrix4x4(1, 3.89414e-07, 0, -4.74221e-07, -2.75972e-14, -2.1173e-09, 1, 0.303307, 3.89414e-07, -1, 0, 5.80965, 0, 0, 0, 1),
            Qt.matrix4x4(0.807687, 0.0365695, -0.588477, -1.58809, -0.00161026, -0.997934, -0.0642243, 1.59698, -0.589607, 0.0528207, -0.805956, 1.03229, 0, 0, 0, 1),
            Qt.matrix4x4(0.471935, -0.631058, -0.615665, -0.244144, -0.802655, -0.59643, -0.0039282, 2.15297, -0.364719, 0.496022, -0.787995, 0.136329, 0, 0, 0, 1),
            Qt.matrix4x4(0.36442, -0.489526, -0.792187, -0.198617, -0.802136, -0.59714, 3.03982e-06, 1.54657, -0.473046, 0.635443, -0.610273, 0.197515, 0, 0, 0, 1),
            Qt.matrix4x4(0.80901, 0.00417731, -0.58778, -1.55616, 0.00231045, -0.99999, -0.00392679, 1.05789, -0.587788, 0.0018186, -0.809008, 1.08293, 0, 0, 0, 1),
            Qt.matrix4x4(0.809015, 6.30314e-07, -0.587788, -1.55427, 4.87695e-07, -1, -4.01388e-07, 0.455413, -0.587785, -1.46701e-07, -0.809012, 1.08375, 0, 0, 0, 1),
            Qt.matrix4x4(0.727117, 0.374232, -0.575545, -1.79386, 0.455163, -0.890399, -0.00392529, 0.0841881, -0.513931, -0.259114, -0.817758, 1.22003, 0, 0, 0, 1),
            Qt.matrix4x4(0.652225, 0.330656, -0.682106, -1.61257, 0.452176, -0.891928, -2.05635e-06, -0.515896, -0.608389, -0.308432, -0.731249, 1.45386, 0, 0, 0, 1),
            Qt.matrix4x4(0.468741, 0.681976, -0.561418, -1.6312, 0.822592, -0.568617, -0.00392005, -0.953233, -0.321902, -0.459983, -0.827519, 1.06958, 0, 0, 0, 1),
            Qt.matrix4x4(0.24794, 0.355874, -0.901042, -0.881577, 0.820497, -0.571648, -3.36765e-06, -1.55446, -0.515079, -0.739305, -0.433726, 1.74657, 0, 0, 0, 1),
            Qt.matrix4x4(0.812441, 0.0011587, -0.583043, 1.54508, 0.00957059, -0.99989, 0.011349, 1.60009, -0.582965, -0.0148005, -0.812362, -1.11435, 0, 0, 0, 1),
            Qt.matrix4x4(0.474129, 0.644705, -0.59963, 0.22274, 0.808336, -0.588689, 0.00621243, 2.16486, -0.34899, -0.487648, -0.800253, -0.170718, 0, 0, 0, 1),
            Qt.matrix4x4(0.370615, 0.494446, -0.786237, 0.176631, 0.807807, -0.58936, 0.010149, 1.5583, -0.458358, -0.638889, -0.617842, -0.219792, 0, 0, 0, 1),
            Qt.matrix4x4(0.829668, 0.0243892, -0.557723, 1.55416, 0.0343586, -0.999382, 0.00740889, 1.11285, -0.557198, -0.0253095, -0.829994, -1.05429, 0, 0, 0, 1),
            Qt.matrix4x4(0.814631, 0.00121515, -0.579979, 1.53562, 0.00957154, -0.99989, 0.0113491, 0.458829, -0.579901, -0.0147966, -0.814552, -1.10174, 0, 0, 0, 1),
            Qt.matrix4x4(0.733453, -0.368831, -0.570973, 1.7816, -0.446605, -0.894721, 0.00426911, 0.0820569, -0.512437, 0.251868, -0.820958, -1.25878, 0, 0, 0, 1),
            Qt.matrix4x4(0.658706, -0.332259, -0.675064, 1.59829, -0.443617, -0.896179, 0.00822179, -0.518127, -0.60771, 0.294054, -0.737714, -1.48719, 0, 0, 0, 1),
            Qt.matrix4x4(0.476947, -0.675082, -0.562837, 1.61132, -0.817091, -0.576508, -0.000921175, -0.960906, -0.323859, 0.460329, -0.826567, -1.1161, 0, 0, 0, 1),
            Qt.matrix4x4(0.254105, -0.362079, -0.896844, 0.849964, -0.814982, -0.579478, 0.00303894, -1.56224, -0.520802, 0.73014, -0.442337, -1.77263, 0, 0, 0, 1)
        ]
    }
    Skin {
        id: skin15
        joints: [
            base,
            chest,
            loweBody,
            head,
            face,
            hand_l,
            hand_l_thumb1,
            hand_l_thumb2,
            hand_l_index1,
            hand_l_index2,
            hand_l_middle1,
            hand_l_middle2,
            hand_l_pinky1,
            hand_l_pinky2,
            hand_r,
            hand_r_thumb1,
            hand_r_thumb2,
            hand_r_index1,
            hand_r_index2,
            hand_r_middle1,
            hand_r_middle2,
            hand_r_pinky1,
            hand_r_pinky2
        ]
        inverseBindPoses: [
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, -1.61245, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(0.813694, 0.0862685, -0.574857, -0.218272, 0.0100981, -0.990875, -0.134407, 0.844787, -0.581196, 0.103562, -0.807129, -0.29807, 0, 0, 0, 1),
            Qt.matrix4x4(1, 1.1016e-07, -3.48602e-07, 1.04242e-06, -1.10184e-07, 1, -6.87816e-05, -3.40188, 3.48594e-07, 6.87816e-05, 1, 0.302907, 0, 0, 0, 1),
            Qt.matrix4x4(1, 3.89414e-07, 0, -4.74221e-07, -2.75972e-14, -2.1173e-09, 1, 0.303307, 3.89414e-07, -1, 0, 5.80965, 0, 0, 0, 1),
            Qt.matrix4x4(0.807687, 0.0365695, -0.588477, -1.58809, -0.00161026, -0.997934, -0.0642243, 1.59698, -0.589607, 0.0528207, -0.805956, 1.03229, 0, 0, 0, 1),
            Qt.matrix4x4(0.471935, -0.631058, -0.615665, -0.244144, -0.802655, -0.59643, -0.0039282, 2.15297, -0.364719, 0.496022, -0.787995, 0.136329, 0, 0, 0, 1),
            Qt.matrix4x4(0.36442, -0.489526, -0.792187, -0.198617, -0.802136, -0.59714, 3.03982e-06, 1.54657, -0.473046, 0.635443, -0.610273, 0.197515, 0, 0, 0, 1),
            Qt.matrix4x4(0.80901, 0.00417731, -0.58778, -1.55616, 0.00231045, -0.99999, -0.00392679, 1.05789, -0.587788, 0.0018186, -0.809008, 1.08293, 0, 0, 0, 1),
            Qt.matrix4x4(0.809015, 6.30314e-07, -0.587788, -1.55427, 4.87695e-07, -1, -4.01388e-07, 0.455413, -0.587785, -1.46701e-07, -0.809012, 1.08375, 0, 0, 0, 1),
            Qt.matrix4x4(0.727117, 0.374232, -0.575545, -1.79386, 0.455163, -0.890399, -0.00392529, 0.0841881, -0.513931, -0.259114, -0.817758, 1.22003, 0, 0, 0, 1),
            Qt.matrix4x4(0.652225, 0.330656, -0.682106, -1.61257, 0.452176, -0.891928, -2.05635e-06, -0.515896, -0.608389, -0.308432, -0.731249, 1.45386, 0, 0, 0, 1),
            Qt.matrix4x4(0.468741, 0.681976, -0.561418, -1.6312, 0.822592, -0.568617, -0.00392005, -0.953233, -0.321902, -0.459983, -0.827519, 1.06958, 0, 0, 0, 1),
            Qt.matrix4x4(0.24794, 0.355874, -0.901042, -0.881577, 0.820497, -0.571648, -3.36765e-06, -1.55446, -0.515079, -0.739305, -0.433726, 1.74657, 0, 0, 0, 1),
            Qt.matrix4x4(0.812441, 0.0011587, -0.583043, 1.54508, 0.00957059, -0.99989, 0.011349, 1.60009, -0.582965, -0.0148005, -0.812362, -1.11435, 0, 0, 0, 1),
            Qt.matrix4x4(0.474129, 0.644705, -0.59963, 0.22274, 0.808336, -0.588689, 0.00621243, 2.16486, -0.34899, -0.487648, -0.800253, -0.170718, 0, 0, 0, 1),
            Qt.matrix4x4(0.370615, 0.494446, -0.786237, 0.176631, 0.807807, -0.58936, 0.010149, 1.5583, -0.458358, -0.638889, -0.617842, -0.219792, 0, 0, 0, 1),
            Qt.matrix4x4(0.829668, 0.0243892, -0.557723, 1.55416, 0.0343586, -0.999382, 0.00740889, 1.11285, -0.557198, -0.0253095, -0.829994, -1.05429, 0, 0, 0, 1),
            Qt.matrix4x4(0.814631, 0.00121515, -0.579979, 1.53562, 0.00957154, -0.99989, 0.0113491, 0.458829, -0.579901, -0.0147966, -0.814552, -1.10174, 0, 0, 0, 1),
            Qt.matrix4x4(0.733453, -0.368831, -0.570973, 1.7816, -0.446605, -0.894721, 0.00426911, 0.0820569, -0.512437, 0.251868, -0.820958, -1.25878, 0, 0, 0, 1),
            Qt.matrix4x4(0.658706, -0.332259, -0.675064, 1.59829, -0.443617, -0.896179, 0.00822179, -0.518127, -0.60771, 0.294054, -0.737714, -1.48719, 0, 0, 0, 1),
            Qt.matrix4x4(0.476947, -0.675082, -0.562837, 1.61132, -0.817091, -0.576508, -0.000921175, -0.960906, -0.323859, 0.460329, -0.826567, -1.1161, 0, 0, 0, 1),
            Qt.matrix4x4(0.254105, -0.362079, -0.896844, 0.849964, -0.814982, -0.579478, 0.00303894, -1.56224, -0.520802, 0.73014, -0.442337, -1.77263, 0, 0, 0, 1)
        ]
    }
    MorphTarget {
        id: morphTarget
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget17
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget18
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget19
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget20
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget21
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget22
        weight: 1
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget23
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    Skin {
        id: skin26
        joints: [
            base,
            chest,
            loweBody,
            head,
            face,
            hand_l,
            hand_l_thumb1,
            hand_l_thumb2,
            hand_l_index1,
            hand_l_index2,
            hand_l_middle1,
            hand_l_middle2,
            hand_l_pinky1,
            hand_l_pinky2,
            hand_r,
            hand_r_thumb1,
            hand_r_thumb2,
            hand_r_index1,
            hand_r_index2,
            hand_r_middle1,
            hand_r_middle2,
            hand_r_pinky1,
            hand_r_pinky2
        ]
        inverseBindPoses: [
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, -1.61245, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(0.813694, 0.0862685, -0.574857, -0.218272, 0.0100981, -0.990875, -0.134407, 0.844787, -0.581196, 0.103562, -0.807129, -0.29807, 0, 0, 0, 1),
            Qt.matrix4x4(1, 1.1016e-07, -3.48602e-07, 1.04242e-06, -1.10184e-07, 1, -6.87816e-05, -3.40188, 3.48594e-07, 6.87816e-05, 1, 0.302907, 0, 0, 0, 1),
            Qt.matrix4x4(1, 3.89414e-07, 0, -4.74221e-07, -2.75972e-14, -2.1173e-09, 1, 0.303307, 3.89414e-07, -1, 0, 5.80965, 0, 0, 0, 1),
            Qt.matrix4x4(0.807687, 0.0365695, -0.588477, -1.58809, -0.00161026, -0.997934, -0.0642243, 1.59698, -0.589607, 0.0528207, -0.805956, 1.03229, 0, 0, 0, 1),
            Qt.matrix4x4(0.471935, -0.631058, -0.615665, -0.244144, -0.802655, -0.59643, -0.0039282, 2.15297, -0.364719, 0.496022, -0.787995, 0.136329, 0, 0, 0, 1),
            Qt.matrix4x4(0.36442, -0.489526, -0.792187, -0.198617, -0.802136, -0.59714, 3.03982e-06, 1.54657, -0.473046, 0.635443, -0.610273, 0.197515, 0, 0, 0, 1),
            Qt.matrix4x4(0.80901, 0.00417731, -0.58778, -1.55616, 0.00231045, -0.99999, -0.00392679, 1.05789, -0.587788, 0.0018186, -0.809008, 1.08293, 0, 0, 0, 1),
            Qt.matrix4x4(0.809015, 6.30314e-07, -0.587788, -1.55427, 4.87695e-07, -1, -4.01388e-07, 0.455413, -0.587785, -1.46701e-07, -0.809012, 1.08375, 0, 0, 0, 1),
            Qt.matrix4x4(0.727117, 0.374232, -0.575545, -1.79386, 0.455163, -0.890399, -0.00392529, 0.0841881, -0.513931, -0.259114, -0.817758, 1.22003, 0, 0, 0, 1),
            Qt.matrix4x4(0.652225, 0.330656, -0.682106, -1.61257, 0.452176, -0.891928, -2.05635e-06, -0.515896, -0.608389, -0.308432, -0.731249, 1.45386, 0, 0, 0, 1),
            Qt.matrix4x4(0.468741, 0.681976, -0.561418, -1.6312, 0.822592, -0.568617, -0.00392005, -0.953233, -0.321902, -0.459983, -0.827519, 1.06958, 0, 0, 0, 1),
            Qt.matrix4x4(0.24794, 0.355874, -0.901042, -0.881577, 0.820497, -0.571648, -3.36765e-06, -1.55446, -0.515079, -0.739305, -0.433726, 1.74657, 0, 0, 0, 1),
            Qt.matrix4x4(0.812441, 0.0011587, -0.583043, 1.54508, 0.00957059, -0.99989, 0.011349, 1.60009, -0.582965, -0.0148005, -0.812362, -1.11435, 0, 0, 0, 1),
            Qt.matrix4x4(0.474129, 0.644705, -0.59963, 0.22274, 0.808336, -0.588689, 0.00621243, 2.16486, -0.34899, -0.487648, -0.800253, -0.170718, 0, 0, 0, 1),
            Qt.matrix4x4(0.370615, 0.494446, -0.786237, 0.176631, 0.807807, -0.58936, 0.010149, 1.5583, -0.458358, -0.638889, -0.617842, -0.219792, 0, 0, 0, 1),
            Qt.matrix4x4(0.829668, 0.0243892, -0.557723, 1.55416, 0.0343586, -0.999382, 0.00740889, 1.11285, -0.557198, -0.0253095, -0.829994, -1.05429, 0, 0, 0, 1),
            Qt.matrix4x4(0.814631, 0.00121515, -0.579979, 1.53562, 0.00957154, -0.99989, 0.0113491, 0.458829, -0.579901, -0.0147966, -0.814552, -1.10174, 0, 0, 0, 1),
            Qt.matrix4x4(0.733453, -0.368831, -0.570973, 1.7816, -0.446605, -0.894721, 0.00426911, 0.0820569, -0.512437, 0.251868, -0.820958, -1.25878, 0, 0, 0, 1),
            Qt.matrix4x4(0.658706, -0.332259, -0.675064, 1.59829, -0.443617, -0.896179, 0.00822179, -0.518127, -0.60771, 0.294054, -0.737714, -1.48719, 0, 0, 0, 1),
            Qt.matrix4x4(0.476947, -0.675082, -0.562837, 1.61132, -0.817091, -0.576508, -0.000921175, -0.960906, -0.323859, 0.460329, -0.826567, -1.1161, 0, 0, 0, 1),
            Qt.matrix4x4(0.254105, -0.362079, -0.896844, 0.849964, -0.814982, -0.579478, 0.00303894, -1.56224, -0.520802, 0.73014, -0.442337, -1.77263, 0, 0, 0, 1)
        ]
    }
    MorphTarget {
        id: morphTarget27
        weight: 0
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget28
        weight: 1
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget29
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget30
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget31
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget32
        weight: 0
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget33
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget34
        weight: 0
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    Skin {
        id: skin37
        joints: [
            base,
            chest,
            loweBody,
            head,
            face,
            hand_l,
            hand_l_thumb1,
            hand_l_thumb2,
            hand_l_index1,
            hand_l_index2,
            hand_l_middle1,
            hand_l_middle2,
            hand_l_pinky1,
            hand_l_pinky2,
            hand_r,
            hand_r_thumb1,
            hand_r_thumb2,
            hand_r_index1,
            hand_r_index2,
            hand_r_middle1,
            hand_r_middle2,
            hand_r_pinky1,
            hand_r_pinky2
        ]
        inverseBindPoses: [
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(1, 0, 0, 0, 0, 1, 0, -1.61245, 0, 0, 1, 0, 0, 0, 0, 1),
            Qt.matrix4x4(0.813694, 0.0862685, -0.574857, -0.218272, 0.0100981, -0.990875, -0.134407, 0.844787, -0.581196, 0.103562, -0.807129, -0.29807, 0, 0, 0, 1),
            Qt.matrix4x4(1, 1.1016e-07, -3.48602e-07, 1.04242e-06, -1.10184e-07, 1, -6.87816e-05, -3.40188, 3.48594e-07, 6.87816e-05, 1, 0.302907, 0, 0, 0, 1),
            Qt.matrix4x4(1, 3.89414e-07, 0, -4.74221e-07, -2.75972e-14, -2.1173e-09, 1, 0.303307, 3.89414e-07, -1, 0, 5.80965, 0, 0, 0, 1),
            Qt.matrix4x4(0.807687, 0.0365695, -0.588477, -1.58809, -0.00161026, -0.997934, -0.0642243, 1.59698, -0.589607, 0.0528207, -0.805956, 1.03229, 0, 0, 0, 1),
            Qt.matrix4x4(0.471935, -0.631058, -0.615665, -0.244144, -0.802655, -0.59643, -0.0039282, 2.15297, -0.364719, 0.496022, -0.787995, 0.136329, 0, 0, 0, 1),
            Qt.matrix4x4(0.36442, -0.489526, -0.792187, -0.198617, -0.802136, -0.59714, 3.03982e-06, 1.54657, -0.473046, 0.635443, -0.610273, 0.197515, 0, 0, 0, 1),
            Qt.matrix4x4(0.80901, 0.00417731, -0.58778, -1.55616, 0.00231045, -0.99999, -0.00392679, 1.05789, -0.587788, 0.0018186, -0.809008, 1.08293, 0, 0, 0, 1),
            Qt.matrix4x4(0.809015, 6.30314e-07, -0.587788, -1.55427, 4.87695e-07, -1, -4.01388e-07, 0.455413, -0.587785, -1.46701e-07, -0.809012, 1.08375, 0, 0, 0, 1),
            Qt.matrix4x4(0.727117, 0.374232, -0.575545, -1.79386, 0.455163, -0.890399, -0.00392529, 0.0841881, -0.513931, -0.259114, -0.817758, 1.22003, 0, 0, 0, 1),
            Qt.matrix4x4(0.652225, 0.330656, -0.682106, -1.61257, 0.452176, -0.891928, -2.05635e-06, -0.515896, -0.608389, -0.308432, -0.731249, 1.45386, 0, 0, 0, 1),
            Qt.matrix4x4(0.468741, 0.681976, -0.561418, -1.6312, 0.822592, -0.568617, -0.00392005, -0.953233, -0.321902, -0.459983, -0.827519, 1.06958, 0, 0, 0, 1),
            Qt.matrix4x4(0.24794, 0.355874, -0.901042, -0.881577, 0.820497, -0.571648, -3.36765e-06, -1.55446, -0.515079, -0.739305, -0.433726, 1.74657, 0, 0, 0, 1),
            Qt.matrix4x4(0.812441, 0.0011587, -0.583043, 1.54508, 0.00957059, -0.99989, 0.011349, 1.60009, -0.582965, -0.0148005, -0.812362, -1.11435, 0, 0, 0, 1),
            Qt.matrix4x4(0.474129, 0.644705, -0.59963, 0.22274, 0.808336, -0.588689, 0.00621243, 2.16486, -0.34899, -0.487648, -0.800253, -0.170718, 0, 0, 0, 1),
            Qt.matrix4x4(0.370615, 0.494446, -0.786237, 0.176631, 0.807807, -0.58936, 0.010149, 1.5583, -0.458358, -0.638889, -0.617842, -0.219792, 0, 0, 0, 1),
            Qt.matrix4x4(0.829668, 0.0243892, -0.557723, 1.55416, 0.0343586, -0.999382, 0.00740889, 1.11285, -0.557198, -0.0253095, -0.829994, -1.05429, 0, 0, 0, 1),
            Qt.matrix4x4(0.814631, 0.00121515, -0.579979, 1.53562, 0.00957154, -0.99989, 0.0113491, 0.458829, -0.579901, -0.0147966, -0.814552, -1.10174, 0, 0, 0, 1),
            Qt.matrix4x4(0.733453, -0.368831, -0.570973, 1.7816, -0.446605, -0.894721, 0.00426911, 0.0820569, -0.512437, 0.251868, -0.820958, -1.25878, 0, 0, 0, 1),
            Qt.matrix4x4(0.658706, -0.332259, -0.675064, 1.59829, -0.443617, -0.896179, 0.00822179, -0.518127, -0.60771, 0.294054, -0.737714, -1.48719, 0, 0, 0, 1),
            Qt.matrix4x4(0.476947, -0.675082, -0.562837, 1.61132, -0.817091, -0.576508, -0.000921175, -0.960906, -0.323859, 0.460329, -0.826567, -1.1161, 0, 0, 0, 1),
            Qt.matrix4x4(0.254105, -0.362079, -0.896844, 0.849964, -0.814982, -0.579478, 0.00303894, -1.56224, -0.520802, 0.73014, -0.442337, -1.77263, 0, 0, 0, 1)
        ]
    }
    MorphTarget {
        id: morphTarget38
        weight: 1
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget39
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget40
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget41
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget42
        attributes: MorphTarget.Position | MorphTarget.Normal
    }
    MorphTarget {
        id: morphTarget43
        attributes: MorphTarget.Position | MorphTarget.Normal
    }

    // Nodes:
    Node {
        id: assistantArmature
        Model {
            id: body
            x: 0.018
            y: -0.215
            source: "meshes/body.mesh"
            z: 0.03802
            skin: skin
            materials: [
                headGlass_material,
                bodyMaterial_material,
                faceScreen_material,
                bodyDarker_material,
                chestScreen_m_material
            ]
        }
        Model {
            id: eye_l
            source: "meshes/mesh_109.mesh"
            skin: skin15
            materials: surfaceShader1_material
            morphTargets: [
                morphTarget,
                morphTarget17,
                morphTarget18,
                morphTarget19,
                morphTarget20,
                morphTarget21,
                morphTarget22,
                morphTarget23
            ]
        }
        Model {
            id: eye_r
            source: "meshes/mesh_107.mesh"
            skin: skin26
            materials: surfaceShader1_material
            morphTargets: [
                morphTarget27,
                morphTarget28,
                morphTarget29,
                morphTarget30,
                morphTarget31,
                morphTarget32,
                morphTarget33,
                morphTarget34
            ]
        }
        Model {
            id: mouth
            source: "meshes/mesh_108.mesh"
            skin: skin37
            materials: surfaceShader1_material
            morphTargets: [
                morphTarget38,
                morphTarget39,
                morphTarget40,
                morphTarget41,
                morphTarget42,
                morphTarget43
            ]
        }
        Node {
            id: base
            x: 0
            eulerRotation.z: 0
            Node {
                id: chest
                y: 1.6124529838562012

                RobotHeart {
                    id: heart
                    x: 0
                    y: 0.5
                    z: 0.78

                    heartAnimation.onFinished: node.restoreDefaults()
                }

                Node {
                    id: loweBody
                    x: -0.004165101796388626
                    y: -0.7256757020950317
                    z: -0.25251561403274536
                    rotation: Qt.quaternion(-0.0626087, 0.950225, 0.0253533, -0.304155)
                    scale.x: 1
                    scale.y: 1
                    scale.z: 0.84
                }
                Node {
                    id: head
                    x: -1.5228409893097705e-06
                    y: 1.7894086837768555
                    z: -0.3031412661075592
                    rotation: Qt.quaternion(1, -3.43908e-05, 1.74299e-07, 5.50858e-08)
                    Node {
                        id: face
                        x: 1.0658141036401503e-13
                        y: 2.4077866077423096
                        z: 7.595476247956867e-09
                        rotation: Qt.quaternion(0.707082, 0.707131, -2.45256e-08, 2.2198e-07)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                    }
                }
                Node {
                    id: hand_l
                    x: 1.89
                    y: 0.5
                    eulerRotation.z: -180
                    eulerRotation.y: -145
                    eulerRotation.x: 0
                    z: 0
                    scale.x: 1
                    scale.y: 1
                    scale.z: 1
                    Node {
                        id: hand_l_thumb1
                        x: -1.3783574104309082e-07
                        y: 0.5359349250793457
                        z: 1.4901161193847656e-07
                        rotation: Qt.quaternion(0.893516, 0.248658, 0.000745812, 0.373896)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_l_thumb2
                            x: -8.568167686462402e-08
                            y: 0.6063140034675598
                            z: 8.195638656616211e-08
                            rotation: Qt.quaternion(0.992124, -0.00169711, -0.125246, 0.00108482)
                            scale.x: 1
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                    Node {
                        id: hand_l_index1
                        x: -1.3783574104309082e-07
                        y: 0.5359349250793457
                        z: 1.4901161193847656e-07
                        rotation: Qt.quaternion(0.999542, -0.0255386, 0.00111382, 0.016175)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_l_index2
                            x: 6.01867213845253e-08
                            y: 0.6069974303245544
                            z: 5.390029400587082e-08
                            rotation: Qt.quaternion(0.999997, -0.000909374, 2.75425e-07, 0.00208834)
                            scale.x: 1
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                    Node {
                        id: hand_l_middle1
                        x: -1.3783574104309082e-07
                        y: 0.5359349250793457
                        z: 1.4901161193847656e-07
                        rotation: Qt.quaternion(0.971617, -0.160599, 0.000448361, -0.173691)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_l_middle2
                            x: -2.2351741790771484e-08
                            y: 0.6075053811073303
                            z: -1.4901161193847656e-08
                            rotation: Qt.quaternion(0.997639, -0.000465468, -0.0686245, 0.00253881)
                            scale.x: 0.99
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                    Node {
                        id: hand_l_pinky1
                        x: -1.3783574104309082e-07
                        y: 0.5359349250793457
                        z: 1.4901161193847656e-07
                        rotation: Qt.quaternion(0.884977, -0.289438, -0.000556779, -0.364748)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_l_pinky2
                            x: 2.421438694000244e-08
                            y: 0.6085596084594727
                            z: -7.82310962677002e-08
                            rotation: Qt.quaternion(0.965603, 0.000116328, -0.260006, 0.00268633)
                            scale.x: 1
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                }
                Node {
                    id: hand_r
                    x: -1.89
                    y: 0.5
                    eulerRotation.z: 185
                    eulerRotation.y: 210
                    eulerRotation.x: 10
                    z: 0
                    scale.x: 1
                    scale.y: 1
                    scale.z: 1
                    Node {
                        id: hand_r_thumb1
                        x: -5.855690687894821e-08
                        y: 0.5341170430183411
                        z: 2.2351741790771484e-08
                        rotation: Qt.quaternion(0.893414, -0.263802, -0.00523041, -0.363584)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_r_thumb2
                            x: -5.960464477539063e-08
                            y: 0.6063126921653748
                            z: 8.195638656616211e-08
                            rotation: Qt.quaternion(0.991448, -0.0011092, -0.130483, 0.0016812)
                            scale.x: 1
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                    Node {
                        id: hand_r_index1
                        x: -5.855690687894821e-08
                        y: 0.5341170430183411
                        z: 2.2351741790771484e-08
                        rotation: Qt.quaternion(0.9998, -0.00545401, 0.0155425, -0.0113054)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_r_index2
                            x: 7.450580596923828e-09
                            y: 0.6073394417762756
                            z: 2.421438694000244e-08
                            rotation: Qt.quaternion(0.999828, 0.0054324, -0.0136597, 0.0113152)
                            scale.x: 1
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                    Node {
                        id: hand_r_middle1
                        x: -5.855690687894821e-08
                        y: 0.5341170430183411
                        z: 2.2351741790771484e-08
                        rotation: Qt.quaternion(0.972198, 0.137881, 0.00537494, 0.189183)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_r_middle2
                            x: 4.377216100692749e-08
                            y: 0.607505738735199
                            z: -1.5832483768463135e-07
                            rotation: Qt.quaternion(0.997774, -0.00258153, -0.0666323, -6.41172e-05)
                            scale.x: 1
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                    Node {
                        id: hand_r_pinky1
                        x: -5.855690687894821e-08
                        y: 0.5341170430183411
                        z: 2.2351741790771484e-08
                        rotation: Qt.quaternion(0.88557, 0.270553, 0.00851581, 0.377484)
                        scale.x: 1
                        scale.y: 1
                        scale.z: 1
                        Node {
                            id: hand_r_pinky2
                            x: -1.4808028936386108e-07
                            y: 0.6085614562034607
                            z: -2.1420419216156006e-08
                            rotation: Qt.quaternion(0.967055, -0.00267332, -0.254553, 0.000299689)
                            scale.x: 1
                            scale.y: 1
                            scale.z: 1
                        }
                    }
                }
            }
        }
    }

    Node {
        id: __materialLibrary__

        PrincipledMaterial {
            id: headGlass_material
            objectName: "headGlass_material"
            baseColor: "#5e000000"
            roughness: 0.10892388224601746
            cullMode: PrincipledMaterial.NoCulling
            alphaMode: PrincipledMaterial.Blend
            transmissionFactor: 1
            indexOfRefraction: 1.4500000476837158
        }

        PrincipledMaterial {
            id: bodyMaterial_material
            objectName: "bodyMaterial_material"
            baseColor: "#ff808080"
            roughness: 0.5527864098548889
            cullMode: PrincipledMaterial.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }

        PrincipledMaterial {
            id: faceScreen_material
            objectName: "faceScreen_material"
            baseColor: "#ff000000"
            metalness: 0.5
            cullMode: PrincipledMaterial.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }

        PrincipledMaterial {
            id: bodyDarker_material
            objectName: "bodyDarker_material"
            baseColor: "#ff292929"
            roughness: 0.5
            cullMode: PrincipledMaterial.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }

        PrincipledMaterial {
            id: chestScreen_m_material
            objectName: "chestScreen_m_material"
            baseColor: "#000000"
            roughness: 0.5
            cullMode: PrincipledMaterial.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }

        PrincipledMaterial {
            id: surfaceShader1_material
            objectName: "surfaceShader1_material"
            baseColor: "#ff079314"
            roughness: 0.5527864098548889
            cullMode: PrincipledMaterial.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
    }

    Model {
        id: rightHand
        objectName: "rightHand"
        position: hand_r.position
        opacity: 0
        source: "#Sphere"
        pivot.y: -40
        scale.x: 0.02
        scale.y: 0.02
        scale.z: 0.02
        pickable: node.state === ""
    }

    Model {
        id: leftHand
        objectName: "leftHand"
        position: hand_l.position
        opacity: 0
        source: "#Sphere"
        pivot.y: -40
        scale.x: 0.02
        scale.y: 0.02
        scale.z: 0.02
        pickable: node.state === ""
    }

    Model {
        id: lowerBody
        objectName: "lowerBody"
        position: loweBody.position
        opacity: 0
        source: "#Sphere"
        pivot.y: -120
        scale.x: 0.01
        scale.z: 0.01
        scale.y: 0.01
        pickable: node.state === ""
    }

    Model {
        id: faceSphere
        objectName: "face"
        position: face.position
        opacity: 0
        source: "#Sphere"
        pivot.y: -80
        scale.x: 0.04
        scale.z: 0.04
        scale.y: 0.04
        pickable: node.state === ""
    }

    Model {
        id: chestSphere
        objectName: "chest"
        position: node.position
        opacity: 0
        source: "#Sphere"
        pivot.y: -110
        scale.x: 0.02
        scale.z: 0.02
        scale.y: 0.02
        pickable: node.state === ""
    }

    // Animations:
    Timeline {
        id: entryTimeline
        animations: [
            TimelineAnimation {
                id: entryAnimation
                onFinished: node.restoreDefaults()
                running: false
                loops: 1
                duration: 10500
                to: 10500
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 10500
        enabled: false

        KeyframeGroup {
            target: morphTarget38
            property: "weight"

            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 1300
            }

            Keyframe {
                value: 0.25
                frame: 1800
            }

            Keyframe {
                value: 0.25
                frame: 10000
            }

            Keyframe {
                value: 1
                frame: 10500
            }
        }

        KeyframeGroup {
            target: morphTarget42
            property: "weight"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1300
            }

            Keyframe {
                value: 0.75
                frame: 1800
            }

            Keyframe {
                value: 0.75
                frame: 10000
            }

            Keyframe {
                value: 0
                frame: 10500
            }
        }

        KeyframeGroup {
            target: morphTarget27
            property: "weight"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1300
            }

            Keyframe {
                value: 1
                frame: 1800
            }

            Keyframe {
                frame: 7499
                value: 0
            }

            Keyframe {
                frame: 7252
                value: 0
            }

            Keyframe {
                frame: 7000
                value: 1
            }
        }

        KeyframeGroup {
            target: morphTarget28
            property: "weight"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 1300
            }

            Keyframe {
                value: 0
                frame: 1800
            }

            Keyframe {
                value: 0
                frame: 9750
            }

            Keyframe {
                value: 1
                frame: 10250
            }
        }

        KeyframeGroup {
            target: morphTarget
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1310
            }

            Keyframe {
                value: 1
                frame: 1800
            }

            Keyframe {
                value: 1
                frame: 7000
            }

            Keyframe {
                value: 0
                frame: 7500
            }

            Keyframe {
                frame: 7250
                value: 0
            }
        }

        KeyframeGroup {
            target: morphTarget17
            property: "weight"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 1300
            }

            Keyframe {
                value: 0
                frame: 1800
            }
        }

        KeyframeGroup {
            target: morphTarget21
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 7000
            }

            Keyframe {
                value: 1
                frame: 7250
            }

            Keyframe {
                frame: 7500
                value: 0
            }
        }

        KeyframeGroup {
            target: morphTarget22
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                frame: 7250
                value: 0
            }

            Keyframe {
                frame: 7500
                value: 1
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "x"

            Keyframe {
                value: -0.07
                frame: 0
            }

            Keyframe {
                value: -0.07
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.77,0,0.175,1,1,1]
                value: 2.49
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.77,0,0.175,1,1,1]
                value: 0.59
                frame: 2300
            }

            Keyframe {
                value: 1.89
                frame: 2500
            }

            Keyframe {
                value: 1.89
                frame: 7000
            }

            Keyframe {
                value: 0.75
                frame: 7400
            }

            Keyframe {
                value: 0.75
                frame: 9700
            }

            Keyframe {
                value: 1.89
                frame: 10500
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "y"

            Keyframe {
                value: 0.5
                frame: 0
            }

            Keyframe {
                value: 0.5
                frame: 1000
            }

            Keyframe {
                value: 0.4
                frame: 1500
            }

            Keyframe {
                value: -0.2
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.77,0,0.175,1,1,1]
                value: 0.3
                frame: 2300
            }

            Keyframe {
                value: 0.5
                frame: 2500
            }

            Keyframe {
                easing.bezierCurve: [0.39,0.575,0.565,1,1,1]
                value: -0.25
                frame: 4100
            }

            Keyframe {
                easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                value: 0.5
                frame: 5150
            }

            Keyframe {
                value: 0.5
                frame: 7000
            }

            Keyframe {
                value: 0.3
                frame: 7400
            }

            Keyframe {
                value: 0.3
                frame: 9700
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.085,0.68,0.53,1,1]
                value: 0.5
                frame: 10500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "x"

            Keyframe {
                value: 0.07
                frame: 0
            }

            Keyframe {
                value: 0.07
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.77,0,0.175,1,1,1]
                value: -2.49
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.77,0,0.175,1,1,1]
                value: -0.59
                frame: 2300
            }

            Keyframe {
                value: -1.89
                frame: 2500
            }

            Keyframe {
                value: -1.89
                frame: 4100
            }

            Keyframe {
                value: -2.89
                frame: 5150
            }

            Keyframe {
                value: -2.89
                frame: 7000
            }

            Keyframe {
                value: -0.75
                frame: 7400
            }

            Keyframe {
                value: -0.75
                frame: 9700
            }

            Keyframe {
                value: -1.89
                frame: 10500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "y"

            Keyframe {
                value:0.5
                frame: 0
            }

            Keyframe {
                value: 0.5
                frame: 1000
            }

            Keyframe {
                value: 0.45
                frame: 1500
            }

            Keyframe {
                value: -0.2
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.77,0,0.175,1,1,1]
                value: 0.3
                frame: 2300
            }

            Keyframe {
                value: 0.5
                frame: 2500
            }

            Keyframe {
                easing.bezierCurve: [0.39,0.575,0.565,1,1,1]
                value: -0.25
                frame: 4100
            }

            Keyframe {
                easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                value: 0.5
                frame: 5150
            }

            Keyframe {
                value: 0.3
                frame: 7400
            }

            Keyframe {
                value: 0.3
                frame: 9700
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.085,0.68,0.53,1,1]
                value: 0.5
                frame: 10500
            }
        }

        KeyframeGroup {
            target: base
            property: "z"
            Keyframe {
                value: -50
                frame: 0
            }

            Keyframe {
                value: 2
                frame: 850
            }

            Keyframe {
                value: 0
                frame: 1572
            }

            Keyframe {
                value: 0.88989
                frame: 1200
            }

            Keyframe {
                value: -1
                frame: 2700
            }

            Keyframe {
                value: -2.16239
                frame: 3150
            }

            Keyframe {
                value: -5
                frame: 4100
            }

            Keyframe {
                value: -4
                frame: 5150
            }


            Keyframe {
                easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                value: -3
                frame: 6250
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.085,0.68,0.53,1,1]
                value: -4
                frame: 7420
            }


            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: -1.89753
                frame: 8950
            }

            Keyframe {
                easing.bezierCurve: [0.215,0.61,0.355,1,1,1]
                value: 0
                frame: 9750
            }

            Keyframe {
                easing.bezierCurve: [0.455,0.03,0.515,0.955,1,1]
                value: -0.5
                frame: 10000
            }

            Keyframe {
                easing.bezierCurve: [0.645,0.045,0.355,1,1,1]
                value: 0
                frame: 10500
            }
        }

        KeyframeGroup {
            target: base
            property: "y"
            Keyframe {
                value: -10
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 850
            }

            Keyframe {
                value: 0.44494
                frame: 1200
            }

            Keyframe {
                value: 0
                frame: 1572
            }

            Keyframe {
                value: -0.5
                frame: 2700
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: -0.25
                frame: 3400
            }

            Keyframe {
                value: 1
                frame: 4100
            }

            Keyframe {
                value: 1
                frame: 5150
            }

            Keyframe {
                easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                value: 0.5
                frame: 6250
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.085,0.68,0.53,1,1]
                value: 1
                frame: 7420
            }

            Keyframe {
                value: 1
                frame: 8950
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: 0
                frame: 9750
            }

            Keyframe {
                value: -0.25
                frame: 10000
            }

            Keyframe {
                value: 0
                frame: 10500
            }
        }

        KeyframeGroup {
            target: base
            property: "x"
            Keyframe {
                value: -50
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.215,0.61,0.355,1,1,1]
                value: 3
                frame: 850
            }

            Keyframe {
                value: 0
                frame: 1572
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: 1.33483
                frame: 1200
            }

            Keyframe {
                value: -1
                frame: 2700
            }

            Keyframe {
                value: 0
                frame: 3150
            }

            Keyframe {
                value: 12
                frame: 4100
            }

            Keyframe {
                value: 11
                frame: 5150
            }

            Keyframe {
                value: 11
                frame: 7400
            }

            Keyframe {
                easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                value: 0
                frame: 8950
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: 0
                frame: 9750
            }
        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.z"
            Keyframe {
                easing.bezierCurve: [0.645,0.045,0.355,1,1,1]
                value: -20
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: 0
                frame: 850
            }

            Keyframe {
                value: 4
                frame: 1000
            }

            Keyframe {
                value: 0
                frame: 1572
            }

            Keyframe {
                value: -3
                frame: 2700
            }

            Keyframe {
                value: -2.95444
                frame: 3150
            }

            Keyframe {
                value: 0
                frame: 4100
            }

            Keyframe {
                value: 0
                frame: 5150
            }

            Keyframe {
                value: 0
                frame: 7400
            }

            Keyframe {
                value: 5
                frame: 7950
            }

            Keyframe {
                value: 0
                frame: 8950
            }

            Keyframe {
                easing.bezierCurve: [0.455,0.03,0.515,0.955,1,1]
                value: 0
                frame: 9750
            }

        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.y"
            Keyframe {
                easing.bezierCurve: [0.895,0.03,0.685,0.22,1,1]
                value: 70
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: 0
                frame: 850
            }

            Keyframe {
                value: -10
                frame: 1000
            }

            Keyframe {
                value: 0
                frame: 1550
            }

            Keyframe {
                value: 10
                frame: 2700
            }

            Keyframe {
                value: 90
                frame: 3150
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.085,0.68,0.53,1,1]
                value: -40
                frame: 4100
            }

            Keyframe {
                value: -30
                frame: 5150
            }

            Keyframe {
                value: -19.22502
                frame: 7400
            }

            Keyframe {
                value: -25
                frame: 7950
            }

            Keyframe {
                easing.bezierCurve: [0.39,0.575,0.565,1,1,1]
                value: -360
                frame: 9750
            }

        }

        KeyframeGroup {
            target: loweBody
            property: "y"

            Keyframe {
                value: 0.3
                frame: 0
            }

            Keyframe {
                value: 0.3
                frame: 1000
            }

            Keyframe {
                value: -0.2
                frame: 1200
            }

            Keyframe {
                easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                value: -1.25
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: -0.72568
                frame: 2700
            }

            Keyframe {
                easing.bezierCurve: [0.39,0.575,0.565,1,1,1]
                value: -2
                frame: 4110
            }

            Keyframe {
                easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                value: -0.72568
                frame: 5150
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: -1.2
                frame: 7400
            }

            Keyframe {
                easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                value: -0.72568
                frame: 9750
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.y"

            Keyframe {
                value: 28
                frame: 0
            }

            Keyframe {
                value: 28
                frame: 1500
            }

            Keyframe {
                value: -100
                frame: 2300
            }

            Keyframe {
                value: -15
                frame: 2500
            }

            Keyframe {
                value: -145
                frame: 2700
            }

            Keyframe {
                value: -145
                frame: 7000
            }

            Keyframe {
                value: -118
                frame: 7400
            }

            Keyframe {
                value: -118
                frame: 9700
            }

            Keyframe {
                value: -145
                frame: 10500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.y"

            Keyframe {
                value: -10
                frame: 1500
            }

            Keyframe {
                value: -10
                frame: 0
            }

            Keyframe {
                value: 93
                frame: 2300
            }

            Keyframe {
                value: 3
                frame: 2500
            }

            Keyframe {
                value: 210
                frame: 2700
            }

            Keyframe {
                value: 210
                frame: 4097
            }

            Keyframe {
                value: 3
                frame: 5149
            }

            Keyframe {
                value: 3
                frame: 6989
            }

            Keyframe {
                value: 110
                frame: 7422
            }

            Keyframe {
                value: 110
                frame: 9695
            }

            Keyframe {
                value: 210
                frame: 10499
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "z"
            Keyframe {
                value: -0.1
                frame: 0
            }
            Keyframe {
                value: -0.1
                frame: 1000
            }
            Keyframe {
                value: 0
                frame: 2000
            }

            Keyframe {
                value: 1.2
                frame: 2300
            }

            Keyframe {
                value: 0
                frame: 2500
            }

            Keyframe {
                value: -0.9
                frame: 1500
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: -0.3
                frame: 1300
            }

            Keyframe {
                value: -1
                frame: 4113
            }

            Keyframe {
                value: 0
                frame: 5149
            }

            Keyframe {
                value: 0
                frame: 6989
            }

            Keyframe {
                value: 1.2
                frame: 7422
            }

            Keyframe {
                value: 1.2
                frame: 9679
            }

            Keyframe {
                value: 0
                frame: 10500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "z"
            Keyframe {
                value: -0.1
                frame: 0
            }
            Keyframe {
                value: -0.1
                frame: 1000
            }
            Keyframe {
                value: 0
                frame: 2000
            }

            Keyframe {
                value: 1.2
                frame: 2300
            }

            Keyframe {
                value: 0
                frame: 2500
            }

            Keyframe {
                value: -0.9
                frame: 1500
            }

            Keyframe {
                value: -0.4
                frame: 1296
            }

            Keyframe {
                value: -1
                frame: 4113
            }

            Keyframe {
                value: 0
                frame: 5149
            }

            Keyframe {
                value: 1.2
                frame: 7422
            }

            Keyframe {
                value: 1.2
                frame: 9695
            }

            Keyframe {
                value: 0
                frame: 10499
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.x"
            Keyframe {
                value: 86
                frame: 1500
            }

            Keyframe {
                value: 86
                frame: 0
            }

            Keyframe {
                value: 40
                frame: 2300
            }

            Keyframe {
                value: 49
                frame: 2500
            }

            Keyframe {
                value: 10
                frame: 2700
            }

            Keyframe {
                value: 10
                frame: 4097
            }

            Keyframe {
                value: 49
                frame: 5149
            }

            Keyframe {
                value: 49
                frame: 5149
            }

            Keyframe {
                value: 9
                frame: 5600
            }

            Keyframe {
                value: 49
                frame: 6046
            }

            Keyframe {
                value: 9
                frame: 6500
            }

            Keyframe {
                value: 49
                frame: 6989
            }

            Keyframe {
                value: 36
                frame: 7422
            }

            Keyframe {
                value: 36
                frame: 9695
            }

            Keyframe {
                value: 10
                frame: 10499
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.z"
            Keyframe {
                value: 108
                frame: 1500
            }

            Keyframe {
                value: 108
                frame: 0
            }

            Keyframe {
                value: 90
                frame: 2300
            }

            Keyframe {
                value: 84
                frame: 2500
            }

            Keyframe {
                value: 185
                frame: 2700
            }

            Keyframe {
                value: 185
                frame: 4097
            }

            Keyframe {
                value: 84
                frame: 5149
            }

            Keyframe {
                value: 4
                frame: 5600
            }

            Keyframe {
                value: 84
                frame: 6050
            }

            Keyframe {
                value: 4
                frame: 6500
            }

            Keyframe {
                value: 84
                frame: 6989
            }

            Keyframe {
                value: 86
                frame: 7422
            }

            Keyframe {
                value: 86
                frame: 9695
            }

            Keyframe {
                value: 185
                frame: 10499
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.x"
            Keyframe {
                value: -3.02782
                frame: 1500
            }

            Keyframe {
                value: -37
                frame: 2300
            }

            Keyframe {
                value: 0
                frame: 2700
            }

            Keyframe {
                value: -15
                frame: 2500
            }

            Keyframe {
                value: -3.02998
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 6989
            }

            Keyframe {
                value: -37
                frame: 7422
            }

            Keyframe {
                value: -37
                frame: 9679
            }

            Keyframe {
                value: 0
                frame: 10500
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.z"
            Keyframe {
                value: -93
                frame: 2300
            }

            Keyframe {
                value: -86.04612
                frame: 1500
            }

            Keyframe {
                value: -180
                frame: 2700
            }

            Keyframe {
                value: -88
                frame: 2500
            }

            Keyframe {
                value: -86.05
                frame: 0
            }

            Keyframe {
                value: -180
                frame: 6989
            }

            Keyframe {
                value: -86
                frame: 7422
            }

            Keyframe {
                value: -86
                frame: 9679
            }

            Keyframe {
                value: -180
                frame: 10500
            }
        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.x"

            Keyframe {
                value: 0
                frame: 2700
            }

            Keyframe {
                value: 35
                frame: 3154
            }

            Keyframe {
                value: 0
                frame: 4100
            }

            Keyframe {
                value: 5
                frame: 5150
            }

            Keyframe {
                value: 4.97468
                frame: 7400
            }

            Keyframe {
                value: 0
                frame: 9750
            }

            Keyframe {
                easing.bezierCurve: [0.165,0.84,0.44,1,1,1]
                value: 2
                frame: 10000
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: 0
                frame: 10500
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "scale.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "scale.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "scale.z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "scale.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "scale.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "scale.z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 1550
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 0
                frame: 0
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 1550
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 0
                frame: 0
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 1550
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 0
                frame: 0
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "scale.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "scale.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "scale.z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "scale.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "scale.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "scale.z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "scale.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "scale.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "scale.z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "scale.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "scale.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "scale.z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "scale.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "scale.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "scale.z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1200
            }

            Keyframe {
                value: 1
                frame: 1550
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: -0.6
                frame: 1200
            }

            Keyframe {
                value: 0
                frame: 1000
            }

            Keyframe {
                value: -0.25
                frame: 2000
            }

            Keyframe {
                value: -0.25
                frame: 9726
            }

            Keyframe {
                value: -0.25
                frame: 2706
            }

            Keyframe {
                easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                value: -1
                frame: 4113
            }

            Keyframe {
                value: -0.25
                frame: 5149
            }

            Keyframe {
                value: -0.25
                frame: 7422
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "eulerRotation.x"
            Keyframe {
                value: -50
                frame: 0
            }

            Keyframe {
                value: -6
                frame: 2000
            }

            Keyframe {
                value: -50
                frame: 1000
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "eulerRotation.y"
            Keyframe {
                value: -172
                frame: 0
            }

            Keyframe {
                value: -144
                frame: 2000
            }

            Keyframe {
                value: -172
                frame: 1000
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "eulerRotation.z"
            Keyframe {
                value: 173
                frame: 0
            }

            Keyframe {
                value: 175
                frame: 2000
            }

            Keyframe {
                value: 173
                frame: 1000
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "eulerRotation.x"
            Keyframe {
                value: -0.17737
                frame: 7422
            }

            Keyframe {
                value: 15
                frame: 7700
            }

            Keyframe {
                value: 15
                frame: 9664
            }

            Keyframe {
                value: -0.17737
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "eulerRotation.z"
            Keyframe {
                value: 0.14769
                frame: 7422
            }

            Keyframe {
                value: -88
                frame: 7700
            }

            Keyframe {
                value: -88
                frame: 9664
            }

            Keyframe {
                value: 0.14769
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "eulerRotation.x"
            Keyframe {
                value: -0.10421
                frame: 7422
            }

            Keyframe {
                value: 18
                frame: 7700
            }

            Keyframe {
                value: 18
                frame: 9664
            }

            Keyframe {
                value: -0.10421
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "eulerRotation.z"
            Keyframe {
                value: 0.23931
                frame: 7422
            }

            Keyframe {
                value: -74
                frame: 7700
            }

            Keyframe {
                value: -74
                frame: 9664
            }

            Keyframe {
                value: 0.23931
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "eulerRotation.x"
            Keyframe {
                value: -0.03325
                frame: 7422
            }

            Keyframe {
                value: 2
                frame: 7700
            }

            Keyframe {
                value: 2
                frame: 9664
            }

            Keyframe {
                value: -0.03325
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "eulerRotation.y"
            Keyframe {
                value: -7.87009
                frame: 7422
            }

            Keyframe {
                value: -47.87009
                frame: 7700
            }

            Keyframe {
                value: -47.87009
                frame: 9664
            }

            Keyframe {
                value: -7.87009
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "eulerRotation.z"
            Keyframe {
                value: 0.2939
                frame: 7422
            }

            Keyframe {
                value: -87
                frame: 7700
            }

            Keyframe {
                value: -87
                frame: 9664
            }

            Keyframe {
                value: 0.2939
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "eulerRotation.x"
            Keyframe {
                value: 0.09291
                frame: 7422
            }

            Keyframe {
                value: 45
                frame: 7700
            }

            Keyframe {
                value: 45
                frame: 9664
            }

            Keyframe {
                value: 0.09291
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "eulerRotation.y"
            Keyframe {
                value: -30.14072
                frame: 7422
            }

            Keyframe {
                value: -30.14072
                frame: 7700
            }

            Keyframe {
                value: -30.14072
                frame: 9664
            }

            Keyframe {
                value: -30.14072
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "eulerRotation.z"
            Keyframe {
                value: 0.29378
                frame: 7422
            }

            Keyframe {
                value: -94
                frame: 7700
            }

            Keyframe {
                value: -94
                frame: 9664
            }

            Keyframe {
                value: 0.29378
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "eulerRotation.y"
            Keyframe {
                value: -58.1828
                frame: 7700
            }

            Keyframe {
                value: 0
                frame: 7422
            }

            Keyframe {
                value: -58.1828
                frame: 9664
            }

            Keyframe {
                value: 0
                frame: 9896
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "eulerRotation.x"
            Keyframe {
                frame: 7422
                value: -0.10088
            }

            Keyframe {
                frame: 7700
                value: 25
            }

            Keyframe {
                frame: 9664
                value: 25
            }

            Keyframe {
                frame: 9896
                value: -0.10088
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "eulerRotation.y"
            Keyframe {
                frame: 7422
                value: -14.99523
            }

            Keyframe {
                frame: 7700
                value: -80
            }

            Keyframe {
                frame: 9664
                value: -80
            }

            Keyframe {
                frame: 9896
                value: -14.99523
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "eulerRotation.z"
            Keyframe {
                frame: 7422
                value: 0.20759
            }

            Keyframe {
                frame: 7700
                value: -79
            }

            Keyframe {
                frame: 9664
                value: -79
            }

            Keyframe {
                frame: 9896
                value: 0.20759
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "eulerRotation.x"
            Keyframe {
                frame: 7422
                value: 0.64012
            }

            Keyframe {
                frame: 7700
                value: 71
            }

            Keyframe {
                frame: 9664
                value: 71
            }

            Keyframe {
                frame: 9896
                value: 0.64012
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "eulerRotation.y"
            Keyframe {
                frame: 7422
                value: -1.55826
            }

            Keyframe {
                frame: 7700
                value: -12
            }

            Keyframe {
                frame: 9664
                value: -12
            }

            Keyframe {
                frame: 9896
                value: -1.55826
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "eulerRotation.z"
            Keyframe {
                frame: 7422
                value: 1.28809
            }

            Keyframe {
                frame: 7700
                value: -31
            }

            Keyframe {
                frame: 9664
                value: -31
            }

            Keyframe {
                frame: 9896
                value: 1.28809
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "eulerRotation.x"
            Keyframe {
                frame: 7422
                value: -0.29565
            }

            Keyframe {
                frame: 7700
                value: 62
            }

            Keyframe {
                frame: 9664
                value: 62
            }

            Keyframe {
                frame: 9896
                value: -0.29565
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "eulerRotation.y"
            Keyframe {
                frame: 7422
                value: -7.64122
            }

            Keyframe {
                frame: 7700
                value: -21
            }

            Keyframe {
                frame: 9664
                value: -21
            }

            Keyframe {
                frame: 9896
                value: -7.64122
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "eulerRotation.z"
            Keyframe {
                frame: 7422
                value: 0.01238
            }

            Keyframe {
                frame: 7700
                value: -46
            }

            Keyframe {
                frame: 9664
                value: -46
            }

            Keyframe {
                frame: 9896
                value: 0.01238
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "eulerRotation.x"
            Keyframe {
                frame: 7422
                value: -0.28751
            }

            Keyframe {
                frame: 7700
                value: 6
            }

            Keyframe {
                frame: 9664
                value: 6
            }

            Keyframe {
                frame: 9896
                value: -0.28751
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "eulerRotation.y"
            Keyframe {
                frame: 7422
                value: -29.49459
            }

            Keyframe {
                frame: 7700
                value: -47
            }

            Keyframe {
                frame: 9664
                value: -47
            }

            Keyframe {
                frame: 9896
                value: -29.49459
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "eulerRotation.z"
            Keyframe {
                frame: 7422
                value: 0.11119
            }

            Keyframe {
                frame: 7700
                value: -74
            }

            Keyframe {
                frame: 9664
                value: -74
            }

            Keyframe {
                frame: 9896
                value: 0.11119
            }
        }

        KeyframeGroup {
            target: morphTarget32
            property: "weight"
            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 7000
                value: 0
            }

            Keyframe {
                frame: 7250
                value: 1
            }

            Keyframe {
                frame: 7500
                value: 0
            }
        }

        KeyframeGroup {
            target: morphTarget33
            property: "weight"
            Keyframe {
                frame: 7500
                value: 1
            }

            Keyframe {
                frame: 7250
                value: 0
            }

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 9750
                value: 1
            }

            Keyframe {
                frame: 10251
                value: 0
            }
        }
    }

    Timeline {
        id: backflipTimeline
        animations: [
            TimelineAnimation {
                id: backflipAnimation
                onFinished: node.restoreDefaults()
                running: false
                loops: 1
                duration: 3000
                to: 3000
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 3000
        enabled: false

        KeyframeGroup {
            target: base
            property: "z"
            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: -20
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.77,0,0.327,0.749,1,1]
                value: 0
                frame: 3000
            }
        }

        KeyframeGroup {
            target: base
            property: "pivot.y"
            Keyframe {
                value: 10
                frame: 1000
            }

            Keyframe {
                value: 5
                frame: 2000
            }

            Keyframe {
                value: 0
                frame: 3000
            }
        }

        KeyframeGroup {
            target: base
            property: "y"
            Keyframe {
                value: 10
                frame: 1000
            }

            Keyframe {
                value: 5
                frame: 2000
            }

            Keyframe {
                value: 0
                frame: 3000
            }
        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.x"
            Keyframe {
                easing.bezierCurve: [0.77,0,0.175,1,1,1]
                value: -350
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.215,0.61,0.355,1,1,1]
                value: 10
                frame: 1000
            }

            Keyframe {
                value: -360
                frame: 3000
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "y"
            Keyframe {
                value: -0.9
                frame: 500
            }

            Keyframe {
                easing.bezierCurve: [0.0553,0.0385,0.0796,-0.0381,0.109,-0.0381,0.138,-0.0381,0.185,0.0276,0.222,0.00353,0.259,-0.0205,0.332,-0.25,0.385,-0.252,0.462,-0.14,0.543,1.18,0.617,1.24,0.676,1.24,0.738,1,0.778,0.977,0.817,0.953,0.855,1.02,0.889,1.03,0.924,1.03,0.951,0.964,1,1]
                value: 0
                frame: 1400
            }

            Keyframe {
                easing.bezierCurve: [0.0485,0.00571,0.0592,0.0429,0.0854,0.0451,0.111,0.0473,0.115,-0.0316,0.153,-0.0316,0.191,-0.0316,0.212,0.0991,0.244,0.1,0.277,0.101,0.283,-0.0454,0.337,-0.0454,0.391,-0.0454,0.427,0.495,0.478,0.534,0.529,0.573,0.55,1.13,0.622,1.13,0.694,1.13,0.729,0.934,0.761,0.934,0.793,0.934,0.811,1.04,0.85,1.04,0.889,1.03,0.902,0.962,0.926,0.962,0.949,0.962,0.959,1,1,1]
                value: -0.72568
                frame: 2400
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.x"
            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: -15
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: -0.18
                frame: 3000
            }

            Keyframe {
                value: 21.06211
                frame: 500
            }

            Keyframe {
                value: 0
                frame: 0
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.z"
            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: -88
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: -178.92
                frame: 3000
            }

            Keyframe {
                value: -182.90512
                frame: 500
            }

            Keyframe {
                value: -178.92
                frame: 0
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "x"

            Keyframe {
                value: 1.89
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: 1.42
                frame: 1000
            }

            Keyframe {
                value: 1.89
                frame: 3000
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "x"
            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: -1.42
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.2,0.2,0.8,0.8,1,1]
                value: -1.92
                frame: 3000
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.z"
            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: 84
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: 185
                frame: 3000
            }

            Keyframe {
                value: 185
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.x"
            Keyframe {
                easing.bezierCurve: [0.55,0.055,0.675,0.19,1,1]
                value: 49
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: 10
                frame: 3000
            }

            Keyframe {
                value: 10
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.y"
            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: 3
                frame: 999
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: 210
                frame: 3001
            }

            Keyframe {
                value: 210
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.y"
            Keyframe {
                value: -145
                frame: 500
            }
            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: -15
                frame: 1000
            }
            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: -145
                frame: 3000
            }
            Keyframe {
                value: -145
                frame: 0
            }
        }
    }

    Timeline {
        id: bouncingTimeline
        animations: [
            TimelineAnimation {
                id: bouncingAnimation
                onFinished: node.restoreDefaults()
                running: false
                loops: 1
                duration: 2000
                to: 2000
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 2000
        enabled: false

        KeyframeGroup {
            target: loweBody
            property: "y"
            Keyframe {
                easing.bezierCurve: [0.233,0.161,0.264,0.997,0.393,0.997,0.522,0.997,0.555,0.752,0.61,0.75,0.664,0.748,0.736,1,0.775,1,0.814,0.999,0.861,0.901,0.888,0.901,0.916,0.901,0.923,0.995,1,1]
                value: -2
                frame: 750
            }

            Keyframe {
                easing.bezierCurve: [0.0699,0.00134,0.0845,0.0779,0.112,0.0779,0.139,0.0779,0.183,-0.00523,0.224,-0.00523,0.266,-0.00523,0.326,0.253,0.387,0.253,0.449,0.253,0.492,0.00134,0.594,-0.00304,0.696,-0.00742,0.767,0.8,1,1]
                value: -0.73
                frame: 1500
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "scale.x"
            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: 0.7
                frame: 750
            }

            Keyframe {
                value: 1
                frame: 1500
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "scale.y"
            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: 0.7
                frame: 750
            }

            Keyframe {
                value: 1
                frame: 1500
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "scale.z"
            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: 0.7
                frame: 750
            }

            Keyframe {
                value: 0.84
                frame: 1500
            }
        }

        KeyframeGroup {
            target: base
            property: "y"
            Keyframe {
                easing.bezierCurve: [0.645,0.045,0.355,1,1,1]
                value: -1
                frame: 750
            }

            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: 0
                frame: 1500
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "y"
            Keyframe {
                easing.bezierCurve: [0.455,0.03,0.515,0.955,1,1]
                value: 0
                frame: 750
            }

            Keyframe {
                value: 0.5
                frame: 1500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "y"
            Keyframe {
                easing.bezierCurve: [0.455,0.03,0.515,0.955,1,1]
                value: 0
                frame: 750
            }

            Keyframe {
                value: 0.5
                frame: 1500
            }
        }
    }

    Timeline {
        id: rightHandWavingTimeline
        animations: [
            TimelineAnimation {
                id: rightHandWavingAnimation
                onFinished: node.restoreDefaults()
                pingPong: false
                running: false
                loops: 1
                duration: 2000
                to: 2000
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 2000
        enabled: false

        KeyframeGroup {
            target: hand_r
            property: "x"
            Keyframe {
                value: -2.89
                frame: 400
            }

            Keyframe {
                value: -1.89
                frame: 2000
            }

            Keyframe {
                value: -2.89
                frame: 1600
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "y"
            Keyframe {
                value: 1
                frame: 400
            }

            Keyframe {
                value: 0.5
                frame: 2000
            }

            Keyframe {
                value: 1
                frame: 1600
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "z"
            Keyframe {
                value: 1
                frame: 400
            }

            Keyframe {
                value: 0
                frame: 2000
            }

            Keyframe {
                value: 1
                frame: 1600
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.x"
            Keyframe {
                value: 49
                frame: 400
            }

            Keyframe {
                value: 10
                frame: 2000
            }

            Keyframe {
                value: 49
                frame: 1600
            }

            Keyframe {
                value: 9
                frame: 700
            }

            Keyframe {
                value: 9
                frame: 1300
            }

            Keyframe {
                value: 49
                frame: 1000
            }

            Keyframe {
                value: 10
                frame: 0
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.y"
            Keyframe {
                value: 3
                frame: 400
            }

            Keyframe {
                value: 210
                frame: 2000
            }

            Keyframe {
                value: 3
                frame: 1600
            }

            Keyframe {
                value: 210
                frame: 0
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.z"
            Keyframe {
                value: 84
                frame: 400
            }

            Keyframe {
                value: 185
                frame: 2000
            }

            Keyframe {
                value: 84
                frame: 1600
            }

            Keyframe {
                value: 4
                frame: 700
            }

            Keyframe {
                value: 4
                frame: 1300
            }

            Keyframe {
                value: 84
                frame: 1000
            }

            Keyframe {
                value: 184
                frame: 0
            }
        }

        KeyframeGroup {
            target: morphTarget27
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 400
            }

            Keyframe {
                value: 1
                frame: 1600
            }

            Keyframe {
                value: 0
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget28
            property: "weight"
            Keyframe {
                value: 1
                frame: 2000
            }

            Keyframe {
                value: 0
                frame: 1600
            }

            Keyframe {
                value: 0
                frame: 400
            }

            Keyframe {
                value: 1
                frame: 0
            }
        }

        KeyframeGroup {
            target: morphTarget38
            property: "weight"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 0.25
                frame: 400
            }

            Keyframe {
                value: 0.25
                frame: 1600
            }

            Keyframe {
                value: 1
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget42
            property: "weight"
            Keyframe {
                value: 0
                frame: 2000
            }

            Keyframe {
                value: 0.75
                frame: 1600
            }

            Keyframe {
                value: 0.75
                frame: 400
            }

            Keyframe {
                value: 0
                frame: 0
            }
        }
    }

    Timeline {
        id: exitTimeline
        animations: [
            TimelineAnimation {
                id: exitAnimation
                onFinished: node.restoreDefaults()
                running: false
                loops: 1
                duration: 3000
                to: 3000
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 3000
        enabled: false

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.x"
            Keyframe {
                value: -3.03
                frame: 300
            }

            Keyframe {
                value: -3.03
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.y"
            Keyframe {
                value: 28
                frame: 300
            }

            Keyframe {
                value: 28
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.z"
            Keyframe {
                value: -86.05
                frame: 300
            }

            Keyframe {
                value: -86.05
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "x"
            Keyframe {
                value: 1.89
                frame: 500
            }

            Keyframe {
                value: -0.07
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "y"
            Keyframe {
                value: 0.5
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "z"
            Keyframe {
                value: -0.1
                frame: 300
            }

            Keyframe {
                value: -0.1
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "x"
            Keyframe {
                value: -1.89
                frame: 500
            }

            Keyframe {
                value: 0.07
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "y"
            Keyframe {
                value: 0.5
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "z"
            Keyframe {
                value: -0.1
                frame: 300
            }

            Keyframe {
                value: -0.1
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.x"
            Keyframe {
                value: 86
                frame: 300
            }

            Keyframe {
                value: 86
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.y"
            Keyframe {
                value: -10
                frame: 300
            }

            Keyframe {
                value: -10
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_r
            property: "eulerRotation.z"
            Keyframe {
                value: 108
                frame: 300
            }

            Keyframe {
                value: 108
                frame: 750
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_thumb2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_index2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "scale.x"
            Keyframe {
                value: 0.99
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_middle2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_l_pinky2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_thumb2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_index2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_middle2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "scale.x"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "scale.y"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: hand_r_pinky2
            property: "scale.z"
            Keyframe {
                value: 1
                frame: 300
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "y"
            Keyframe {
                value: -0.72568
                frame: 500
            }

            Keyframe {
                value: -0.2
                frame: 750
            }

            Keyframe {
                value: 0.3
                frame: 850
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "z"
            Keyframe {
                value: -0.5
                frame: 500
            }

            Keyframe {
                value: -0.5
                frame: 750
            }

            Keyframe {
                value: 0
                frame: 850
            }
        }

        KeyframeGroup {
            target: base
            property: "z"
            Keyframe {
                value: -3
                frame: 850
            }

            Keyframe {
                value: -80
                frame: 3000
            }
        }

        KeyframeGroup {
            target: base
            property: "x"
            Keyframe {
                value: -2
                frame: 850
            }

            Keyframe {
                easing.bezierCurve: [0.215,0.61,0.355,1,1,1]
                value: 30
                frame: 3000
            }
        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.x"
            Keyframe {
                value: 10
                frame: 850
            }
        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.y"
            Keyframe {
                value: 30
                frame: 850
            }

            Keyframe {
                value: 130
                frame: 1200
            }
        }

        KeyframeGroup {
            target: base
            property: "y"
            Keyframe {
                easing.bezierCurve: [0.316,0.525,0.744,1.42,1,1]
                value: -1
                frame: 850
            }

            Keyframe {
                value: -2
                frame: 3000
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "eulerRotation.x"
            Keyframe {
                value: -50
                frame: 500
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "eulerRotation.y"
            Keyframe {
                value: -172
                frame: 500
            }
        }

        KeyframeGroup {
            target: loweBody
            property: "eulerRotation.z"
            Keyframe {
                value: 173
                frame: 500
            }
        }
    }

    //! [wavingAnim]
    Timeline {
        id: leftHandWavingTimeline
        animations: [
            TimelineAnimation {
                id: leftHandWavingAnimation
                onFinished: node.restoreDefaults()
                running: false
                loops: 1
                duration: 2000
                to: 2000
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 2000
        enabled: false

        KeyframeGroup {
            target: hand_l
            property: "x"
            Keyframe {
                value: 2.89
                frame: 400
            }

            Keyframe {
                value: 2.89
                frame: 1600
            }

            Keyframe {
                value: 1.89
                frame: 2000
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "y"
            Keyframe {
                value: 1
                frame: 400
            }

            Keyframe {
                value: 1
                frame: 1600
            }

            Keyframe {
                value: 0.5
                frame: 2000
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "z"
            Keyframe {
                value: 1
                frame: 400
            }

            Keyframe {
                value: 1
                frame: 1600
            }

            Keyframe {
                value: -0.1
                frame: 2000
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.x"
            Keyframe {
                value: -15
                frame: 400
            }

            Keyframe {
                value: -5
                frame: 700
            }

            Keyframe {
                value: -15
                frame: 1000
            }

            Keyframe {
                value: -5
                frame: 1300
            }

            Keyframe {
                value: -15
                frame: 1600
            }

            Keyframe {
                value: -0.18
                frame: 2000
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.y"
            Keyframe {
                value: -15
                frame: 400
            }

            Keyframe {
                value: -30
                frame: 1600
            }

            Keyframe {
                value: -145
                frame: 2000
            }

            Keyframe {
                value: -40
                frame: 700
            }
        }

        KeyframeGroup {
            target: hand_l
            property: "eulerRotation.z"
            Keyframe {
                value: -88
                frame: 400
            }

            Keyframe {
                value: -30
                frame: 700
            }

            Keyframe {
                value: -86.05
                frame: 1000
            }

            Keyframe {
                value: -30
                frame: 1300
            }

            Keyframe {
                value: -86.05
                frame: 1600
            }

            Keyframe {
                value: -178.92
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget38
            property: "weight"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 0.25
                frame: 400
            }

            Keyframe {
                value: 0.25
                frame: 1600
            }

            Keyframe {
                value: 1
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget42
            property: "weight"
            Keyframe {
                value: 0
                frame: 2000
            }

            Keyframe {
                value: 0.75
                frame: 1600
            }

            Keyframe {
                value: 0.75
                frame: 400
            }

            Keyframe {
                value: 0
                frame: 0
            }
        }

        KeyframeGroup {
            target: morphTarget27
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 400
            }

            Keyframe {
                value: 1
                frame: 1600
            }

            Keyframe {
                value: 0
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget28
            property: "weight"
            Keyframe {
                value: 1
                frame: 2000
            }

            Keyframe {
                value: 0
                frame: 1600
            }

            Keyframe {
                value: 0
                frame: 400
            }

            Keyframe {
                value: 1
                frame: 0
            }
        }
    }
    //! [wavingAnim]

    Timeline {
        id: exploreTimeline
        animations: [
            TimelineAnimation {
                id: exploreAnimation
                onFinished: node.restoreDefaults()
                running: false
                loops: 1
                duration: 10250
                to: 10250
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 10250
        enabled: false

        KeyframeGroup {
            target: base
            property: "x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                value: 10
                frame: 1000
            }

            Keyframe {
                value: 11
                frame: 1500
            }

            Keyframe {
                value: 50
                frame: 3500
            }

            Keyframe {
                easing.bezierCurve: [0.25,0.46,0.45,0.94,1,1]
                value: 0
                frame: 5500
            }

            Keyframe {
                easing.bezierCurve: [0.445,0.05,0.55,0.95,1,1]
                value: -30
                frame: 8000
            }

            Keyframe {
                easing.bezierCurve: [0.6,-0.28,0.735,0.045,1,1]
                value: 0
                frame: 9700
            }
        }

        KeyframeGroup {
            target: base
            property: "y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.6,-0.28,0.735,0.045,1,1]
                value: 1
                frame: 1000
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.085,0.68,0.53,1,1]
                value: 2
                frame: 1500
            }

            Keyframe {
                easing.bezierCurve: [0.175,0.885,0.32,1.27,1,1]
                value: 10
                frame: 3500
            }

            Keyframe {
                easing.bezierCurve: [0.215,0.61,0.355,1,1,1]
                value: 0
                frame: 5500
            }

            Keyframe {
                easing.bezierCurve: [0.895,0.03,0.685,0.22,1,1]
                value: 20
                frame: 8000
            }

            Keyframe {
                easing.bezierCurve: [0.55,0.085,0.68,0.53,1,1]
                value: 0
                frame: 9700
            }
        }

        KeyframeGroup {
            target: base
            property: "z"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 1000
            }

            Keyframe {
                value: 0
                frame: 1500
            }

            Keyframe {
                value: -50
                frame: 3500
            }

            Keyframe {
                value: -50
                frame: 5500
            }

            Keyframe {
                value: -70
                frame: 8000
            }

            Keyframe {
                easing.bezierCurve: [0.47,0,0.745,0.715,1,1]
                value: 0
                frame: 9700
            }
        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.x"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 5
                frame: 1000
            }

            Keyframe {
                value: -5
                frame: 1500
            }

            Keyframe {
                value: 10
                frame: 3500
            }

            Keyframe {
                value: 10
                frame: 5500
            }

            Keyframe {
                value: -10
                frame: 8000
            }

            Keyframe {
                value: 0
                frame: 9700
            }
        }

        KeyframeGroup {
            target: base
            property: "eulerRotation.y"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 40
                frame: 1000
            }

            Keyframe {
                value: 40
                frame: 1500
            }

            Keyframe {
                value: 140
                frame: 3500
            }

            Keyframe {
                value: 180
                frame: 5500
            }

            Keyframe {
                value: 270
                frame: 4000
            }

            Keyframe {
                value: 680
                frame: 8000
            }

            Keyframe {
                value: 590
                frame: 6500
            }

            Keyframe {
                easing.bezierCurve: [0.68,-0.55,0.265,1.55,1,1]
                value: 720
                frame: 9700
            }
        }

        KeyframeGroup {
            target: morphTarget38
            property: "weight"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 500
            }

            Keyframe {
                value: 0
                frame: 9700
            }

            Keyframe {
                value: 1
                frame: 10250
            }
        }

        KeyframeGroup {
            target: morphTarget39
            property: "weight"
            Keyframe {
                value: 0
                frame: 10250
            }

            Keyframe {
                value: 1
                frame: 9700
            }

            Keyframe {
                value: 1
                frame: 500
            }

            Keyframe {
                value: 0
                frame: 0
            }
        }

        KeyframeGroup {
            target: morphTarget28
            property: "weight"
            Keyframe {
                value: 1
                frame: 10250
            }

            Keyframe {
                value: 0
                frame: 9700
            }

            Keyframe {
                value: 0
                frame: 500
            }

            Keyframe {
                value: 1
                frame: 0
            }
        }

        KeyframeGroup {
            target: morphTarget27
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 500
            }

            Keyframe {
                value: 1
                frame: 9700
            }

            Keyframe {
                value: 0
                frame: 10250
            }
        }
    }

    Timeline {
        id: faceTimeline
        animations: [
            TimelineAnimation {
                id: faceAnimation
                onFinished: node.restoreDefaults()
                running: false
                loops: 1
                duration: 5000
                to: 5000
                from: 0
            }
        ]
        startFrame: 0
        endFrame: 5000
        enabled: false

        KeyframeGroup {
            target: morphTarget27
            property: "weight"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 250
            }

            Keyframe {
                value: 1
                frame: 500
            }

            Keyframe {
                value: 1
                frame: 1500
            }

            Keyframe {
                value: 0
                frame: 1750
            }

            Keyframe {
                value: 0
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget28
            property: "weight"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 250
            }

            Keyframe {
                value: 0
                frame: 1750
            }

            Keyframe {
                value: 1
                frame: 2000
            }

            Keyframe {
                value: 1
                frame: 3000
            }

            Keyframe {
                value: 0
                frame: 3250
            }

            Keyframe {
                value: 0
                frame: 4750
            }

            Keyframe {
                value: 1
                frame: 5000
            }
        }

        KeyframeGroup {
            target: morphTarget38
            property: "weight"

            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 0.25
                frame: 500
            }

            Keyframe {
                value: 0.25
                frame: 1500
            }

            Keyframe {
                value: 1
                frame: 2000
            }

            Keyframe {
                value: 1
                frame: 3000
            }

            Keyframe {
                value: 0
                frame: 3500
            }

            Keyframe {
                value: 0
                frame: 4500
            }

            Keyframe {
                value: 1
                frame: 5000
            }
        }

        KeyframeGroup {
            target: morphTarget42
            property: "weight"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0.75
                frame: 500
            }

            Keyframe {
                value: 0.75
                frame: 1500
            }

            Keyframe {
                value: 0
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget32
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 250
            }

            Keyframe {
                value: 0
                frame: 500
            }

            Keyframe {
                value: 0
                frame: 1500
            }

            Keyframe {
                value: 1
                frame: 1750
            }

            Keyframe {
                value: 0
                frame: 2000
            }

            Keyframe {
                value: 0
                frame: 3000
            }

            Keyframe {
                value: 1
                frame: 3250
            }

            Keyframe {
                value: 0
                frame: 3500
            }

            Keyframe {
                value: 0
                frame: 4500
            }

            Keyframe {
                value: 1
                frame: 4750
            }

            Keyframe {
                value: 0
                frame: 5000
            }
        }

        KeyframeGroup {
            target: morphTarget22
            property: "weight"
            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 250
            }

            Keyframe {
                value: 0
                frame: 1750
            }

            Keyframe {
                value: 1
                frame: 2000
            }

            Keyframe {
                value: 1
                frame: 3000
            }

            Keyframe {
                value: 0
                frame: 3250
            }

            Keyframe {
                value: 0
                frame: 4750
            }

            Keyframe {
                value: 1
                frame: 5000
            }
        }

        KeyframeGroup {
            target: morphTarget
            property: "weight"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 250
            }

            Keyframe {
                value: 1
                frame: 500
            }

            Keyframe {
                value: 1
                frame: 1500
            }

            Keyframe {
                value: 0
                frame: 1750
            }

            Keyframe {
                value: 0
                frame: 2000
            }
        }

        KeyframeGroup {
            target: morphTarget21
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 250
            }

            Keyframe {
                value: 0
                frame: 500
            }

            Keyframe {
                value: 0
                frame: 1500
            }

            Keyframe {
                value: 1
                frame: 1750
            }

            Keyframe {
                value: 0
                frame: 2000
            }

            Keyframe {
                value: 0
                frame: 3000
            }

            Keyframe {
                value: 1
                frame: 3250
            }

            Keyframe {
                value: 0
                frame: 3500
            }

            Keyframe {
                value: 0
                frame: 4500
            }

            Keyframe {
                value: 1
                frame: 4750
            }

            Keyframe {
                value: 0
                frame: 5000
            }
        }

        KeyframeGroup {
            target: morphTarget34
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 3250
            }

            Keyframe {
                value: 1
                frame: 3500
            }

            Keyframe {
                value: 1
                frame: 4500
            }

            Keyframe {
                value: 0
                frame: 4750
            }
        }

        KeyframeGroup {
            target: morphTarget23
            property: "weight"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 3250
            }

            Keyframe {
                value: 1
                frame: 3500
            }

            Keyframe {
                value: 1
                frame: 4500
            }

            Keyframe {
                value: 0
                frame: 4750
            }
        }

        KeyframeGroup {
            target: morphTarget40
            property: "weight"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 3000
            }

            Keyframe {
                value: 1
                frame: 3500
            }

            Keyframe {
                value: 1
                frame: 4500
            }

            Keyframe {
                value: 0
                frame: 5000
            }
        }
    }

    states: [
        State {
            name: "EntryAnimation"
            when: node.currentAnim === VirtualAssistant.ANIMATION.ENTRY

            PropertyChanges {
                target: entryTimeline
                enabled: true
            }

            PropertyChanges {
                target: entryAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "Backflip"
            when: node.currentAnim === VirtualAssistant.ANIMATION.BACKFLIP

            PropertyChanges {
                target: backflipTimeline
                enabled: true
            }

            PropertyChanges {
                target: backflipAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "LowBodyBouncing"
            when: node.currentAnim === VirtualAssistant.ANIMATION.BOUNCING

            PropertyChanges {
                target: bouncingTimeline
                enabled: true
            }

            PropertyChanges {
                target: bouncingAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "Right hand waving"
            when: node.currentAnim === VirtualAssistant.ANIMATION.RIGHTHAND

            PropertyChanges {
                target: rightHandWavingTimeline
                enabled: true
            }

            PropertyChanges {
                target: rightHandWavingAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "Left hand waving"
            when: node.currentAnim === VirtualAssistant.ANIMATION.LEFTHAND

            PropertyChanges {
                target: leftHandWavingTimeline
                enabled: true
            }

            PropertyChanges {
                target: leftHandWavingAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "Explore Scene"
            when: node.currentAnim === VirtualAssistant.ANIMATION.EXPLORE

            PropertyChanges {
                target: exploreTimeline
                enabled: true
            }

            PropertyChanges {
                target: exploreAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "Exit Animation"
            when: node.currentAnim === VirtualAssistant.ANIMATION.EXIT

            PropertyChanges {
                target: exitTimeline
                enabled: true
            }

            PropertyChanges {
                target: exitAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "FaceAnim"
            when: node.currentAnim === VirtualAssistant.ANIMATION.FACE

            PropertyChanges {
                target: faceTimeline
                enabled: true
            }

            PropertyChanges {
                target: faceAnimation
                running: true
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        },
        State {
            name: "HeartAnimation"
            when: node.currentAnim === VirtualAssistant.ANIMATION.HEART

            PropertyChanges {
                target: heart.heartAnimation
                loops: 4
            }

            PropertyChanges {
                target: heart.heartTimeline
                enabled: true
            }
        }]
}
