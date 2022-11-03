// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick3D

Node {
    required property Camera camera
    required property var distances

    onChildrenChanged: {
        // Add distance threshold values to instanced children
        var distIndex = 0; // Handle distance index separately to allow non-node children
        for (var i = 0; i < children.length; i++) {
            if (!(children[i] instanceof Model) || !children[i].instancing)
                continue;

            if (distIndex - 1 >= 0)
                children[i].instancingLodMin = distances[distIndex - 1];

            if (distances.length > distIndex)
                children[i].instancingLodMax = distances[distIndex];

             distIndex++;
        }
    }

    Connections {
        target: camera
        function onPositionChanged() {
            var distIndex = 0; // Handle distance index separately to allow non-node children
            for (var i = 0; i < children.length; i++) {
                var node = children[i];
                if (!(node instanceof Node))
                    continue;
                if (node instanceof Model && node.instancing)
                    continue;
                if (distIndex > distances.length)
                    break;

                // Hide all nodes by default
                node.visible = false;

                var minThreshold = 0;
                var maxThreshold = -1;

                if (distIndex - 1 >= 0)
                    minThreshold = distances[distIndex - 1];

                if (distances.length > distIndex)
                    maxThreshold = distances[distIndex];

                // Show nodes that are inside the minimum and maximum distance thresholds
                var distance = node.scenePosition.minus(camera.scenePosition).length();
                if (distance >= minThreshold && (maxThreshold < 0 || distance < maxThreshold))
                    node.visible = true;

                distIndex++;
            }
        }
    }
}
