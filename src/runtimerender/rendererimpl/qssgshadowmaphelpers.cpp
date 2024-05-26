// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qssgshadowmaphelpers_p.h"

#include "qssgdebugdrawsystem_p.h"
#include <cfloat>

QT_BEGIN_NAMESPACE

static constexpr std::array<std::array<int, 2>, 12> BOX_LINE_INDICES = {
    std::array<int, 2> { 0, 1 }, std::array<int, 2> { 1, 2 }, std::array<int, 2> { 2, 3 }, std::array<int, 2> { 3, 0 },
    std::array<int, 2> { 4, 5 }, std::array<int, 2> { 5, 6 }, std::array<int, 2> { 6, 7 }, std::array<int, 2> { 7, 4 },
    std::array<int, 2> { 0, 4 }, std::array<int, 2> { 1, 5 }, std::array<int, 2> { 2, 6 }, std::array<int, 2> { 3, 7 },
};

static std::array<QVector3D, 8> sortBox(const QSSGBoxPoints &box)
{
    // Very scuffed but finds one side of the box by getting the first
    // 4 closest points. Then the other side is found by finding the closest
    // point for each point on the first side.
    std::array<int, 8> sortedIdx = { 0, -1, -1, -1, -1, -1, -1, -1 };
    QVector3D current = box[0];
    QSSGBoxPoints boxSorted;
    boxSorted[0] = box[0];
    for (int i = 1; i < 4; i++) {
        int idxFound = -1;
        float bestDist = FLT_MAX;

        for (int j = 0; j < 8; j++) {
            if (sortedIdx[j] != -1)
                continue;

            float dist = box[j].distanceToPoint(current);
            // Should not happen unless the box contains NaN or inf values
            if (!(dist < FLT_MAX))
                return box;

            if (dist < bestDist) {
                idxFound = j;
                bestDist = dist;
            }
        }

        sortedIdx[idxFound] = i;
        current = box[idxFound];
        boxSorted[i] = current;
    }

    // First 4 found
    for (int i = 0; i < 4; i++) {
        float bestDist = FLT_MAX;
        QVector3D current = boxSorted[i];

        for (int j = 0; j < 8; j++) {
            float dist = box[j].distanceToPoint(current);
            // Should not happen unless the box contains NaN or inf values
            if (!(dist < FLT_MAX))
                return box;
            if (dist < bestDist && dist != 0.f && sortedIdx[j] == -1) {
                boxSorted[i + 4] = box[j];
                bestDist = dist;
            }
        }
    }

    return boxSorted;
}

void ShadowmapHelpers::addDebugBox(const QSSGBoxPoints &boxUnsorted, const QColor &color, QSSGDebugDrawSystem *debugDrawSystem, bool sort)
{
    if (!debugDrawSystem)
        return;

    const auto box = sort ? sortBox(boxUnsorted) : boxUnsorted;
    for (const auto line : BOX_LINE_INDICES)
        debugDrawSystem->drawLine(box[line[0]], box[line[1]], color);

    debugDrawSystem->setEnabled(true);
}

void ShadowmapHelpers::addDebugFrustum(const QSSGBoxPoints &frustumPoints, const QColor &color, QSSGDebugDrawSystem *debugDrawSystem)
{
    ShadowmapHelpers::addDebugBox(frustumPoints, color, debugDrawSystem, false);
}

void ShadowmapHelpers::addDirectionalLightDebugBox(const QSSGBoxPoints &box, QSSGDebugDrawSystem *debugDrawSystem)
{
    if (!debugDrawSystem)
        return;

    constexpr QColor colorBox = QColorConstants::Yellow;

    //    .7------6
    //  .' |    .'|
    // 3---+--2'  |
    // |   |  |   |
    // |  .4--+---5
    // |.'    | .'
    // 0------1'

    // Find shortest axis line
    float shortestAxisSq = (box[0] - box[1]).lengthSquared();
    shortestAxisSq = std::min(shortestAxisSq, (box[0] - box[4]).lengthSquared());
    shortestAxisSq = std::min(shortestAxisSq, (box[0] - box[3]).lengthSquared());
    const float axisLength = qSqrt(shortestAxisSq) * 0.5f;

    auto drawAxisLine = [&](const QVector3D &from, const QVector3D &to, const QColor &axisColor) {
        const QVector3D dir = (to - from).normalized();
        const QVector3D mid = from + dir * axisLength;
        debugDrawSystem->drawLine(from, mid, axisColor);
        debugDrawSystem->drawLine(mid, to, colorBox);
    };

    // Draw x,y,z axis in r,g,b color and rest in box color

    // Closest to light (front)
    drawAxisLine(box[0], box[1], QColorConstants::Red);
    debugDrawSystem->drawLine(box[1], box[2], colorBox);
    drawAxisLine(box[0], box[3], QColorConstants::Green);
    debugDrawSystem->drawLine(box[2], box[3], colorBox);

    // Lines parallel to light direction
    drawAxisLine(box[0], box[4], QColorConstants::Blue);
    debugDrawSystem->drawLine(box[1], box[5], colorBox);
    debugDrawSystem->drawLine(box[2], box[6], colorBox);
    debugDrawSystem->drawLine(box[3], box[7], colorBox);

    // Furthest from light (back)
    debugDrawSystem->drawLine(box[4], box[5], colorBox);
    debugDrawSystem->drawLine(box[5], box[6], colorBox);
    debugDrawSystem->drawLine(box[6], box[7], colorBox);
    debugDrawSystem->drawLine(box[7], box[4], colorBox);

    debugDrawSystem->setEnabled(true);
}

QT_END_NAMESPACE
