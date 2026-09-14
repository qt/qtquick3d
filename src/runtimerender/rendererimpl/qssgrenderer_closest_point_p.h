// Copyright (C) 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
// Qt-Security score:significant reason:default


#ifndef QSSG_RENDERER_CLOSEST_POINT_P_H
#define QSSG_RENDERER_CLOSEST_POINT_P_H

#include <QVector3D>

#include <cstdlib>

QT_BEGIN_NAMESPACE

namespace  {

// Closest point on triangle ABC to point p, using metric defined by template class
// This code is based on: https://github.com/RenderKit/embree/blob/master/tutorials/common/math/closest_point.h

template<class Dot>
static QVector3D closestPointOnTriangle(const QVector3D &p,
                                        const QVector3D &a,
                                        const QVector3D &b,
                                        const QVector3D &c,
                                        const Dot &dot,
                                        float &u, float &v, float &w)
{
    const QVector3D ab = b - a;
    const QVector3D ac = c - a;
    const QVector3D ap = p - a;

    // Vertex region A
    const float d1 = dot(ab, ap);
    const float d2 = dot(ac, ap);
    if (d1 <= 0.f && d2 <= 0.f) {
        u = 1.0f; v = 0.0f; w = 0.0f;
        return a;
    }

    // Vertex region B
    const QVector3D bp = p - b;
    const float d3 = dot(ab, bp);
    const float d4 = dot(ac, bp);
    if (d3 >= 0.f && d4 <= d3) {
        u = 0.0f; v = 1.0f; w = 0.0f;
        return b;
    }

    // Edge AB
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f) {
        const float v_edge = d1 / (d1 - d3);
        u = 1.0f - v_edge; v = v_edge; w = 0.0f;
        return a + v_edge * ab;
    }

    // Vertex region C
    const QVector3D cp = p - c;
    const float d5 = dot(ab, cp);
    const float d6 = dot(ac, cp);
    if (d6 >= 0.f && d5 <= d6) {
        u = 0.0f; v = 0.0f; w = 1.0f;
        return c;
    }

    // Edge AC
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f) {
        const float w_edge = d2 / (d2 - d6);
        u = 1.0f - w_edge; v = 0.0f; w = w_edge;
        return a + w_edge * ac;
    }

    // Edge BC
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f) {
        const QVector3D bc = c - b;
        const float w_edge = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        u = 0.0f; v = 1.0f - w_edge; w = w_edge;
        return b + w_edge * bc;
    }

    // Inside face region
    const float denom = va + vb + vc;

    // Check for degenerate case
    if (std::abs(denom) < 1e-20f) {
        // Degenerate triangle in metric space: fall back to closest among vertices
        const float da = dot(ap, ap);
        const float db = dot(bp, bp);
        const float dc = dot(cp, cp);
        if (da <= db && da <= dc) {
            u = 1.0f; v = 0.0f; w = 0.0f;
            return a;
        }
        if (db <= dc) {
            u = 0.0f; v = 1.0f; w = 0.0f;
            return b;
        }
        u = 0.0f; v = 0.0f; w = 1.0f;
        return c;
    }

    const float invDenom = 1.0f / denom;
    u = va * invDenom;
    v = vb * invDenom;
    w = vc * invDenom;
    return a + v * ab + w * ac;
}

} // namespace (anonymous)

QT_END_NAMESPACE

#endif // QSSG_RENDERER_CLOSEST_POINT_P_H
