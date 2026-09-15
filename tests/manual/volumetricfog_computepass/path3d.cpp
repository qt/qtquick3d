// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "path3d.h"
#include <private/qquick3dmodel_p.h>

Path3D::Path3D(QQuick3DNode *parent)
    : QQuick3DNode(parent), m_animation(new Animation(this))
{

}

Path3D::~Path3D()
{
    m_animation->stop();
}

QList<int> Path3D::times() const
{
    return m_times;
}

QList<QVector3D> Path3D::positions() const
{
    return m_positions;
}

QList<QVector3D> Path3D::orientations() const
{
    return m_orientations;
}

int Path3D::loopBackDuration() const
{
    return m_loopBackDuration;
}

int Path3D::duration() const
{
    return m_duration;
}

int Path3D::pathDuration() const
{
    return m_pathDuration;
}

void Path3D::setTimes(const QList<int> &times)
{
    m_times = times;
    emit timesChanged();
}

void Path3D::setPositions(const QList<QVector3D> &positions)
{
    m_positions = positions;
    emit positionsChanged();
}

void Path3D::setOrientations(const QList<QVector3D> &orientations)
{
    m_orientations = orientations;
    emit orientationsChanged();
}

void Path3D::setLoopBackDuration(int duration)
{
    m_loopBackDuration = duration;
    emit loopBackDurationChanged();
}

void Path3D::setPathDuration(int duration)
{
    m_pathDuration = duration;
    emit pathDurationChanged();
}

static inline QVector3D lerp(const QVector3D &a, const QVector3D &b, float t)
{
    return a + (b - a) * t;
}

static inline QVector3D eullerp(const QVector3D &a, const QVector3D &b, float t)
{
    QQuaternion qa = QQuaternion::fromEulerAngles(a);
    QQuaternion qb = QQuaternion::fromEulerAngles(b);
    QQuaternion l = QQuaternion::slerp(qa, qb, t);
    return l.toEulerAngles();
}

void Path3D::componentComplete()
{
    QQuick3DNode::componentComplete();
    auto models = findChildren<QQuick3DModel*>();
    for (auto &child : models) {
        QQuick3DModel *node = static_cast<QQuick3DModel*>(child);
        if (node)
            m_nodes.append(node);
    }
    m_duration = m_loopBackDuration;
    if (m_times.size() > 0)
        m_duration += m_times.last();
    updateCurrentTime(0);
    m_animation->start();
}

void Path3D::calculateImplicitTimes()
{
    if (m_implicitTimes.size())
        return;
    m_implicitTimes.resize(m_positions.size());
    float distance = 0.0f;
    float lpDist = 0.0f;
    for (int i = 1; i < m_positions.size(); i++)
        distance += (m_positions[i] - m_positions[i - 1]).length();
    distance += (m_positions.first() - m_positions.last()).length();

    float pathLen = 0.0f;
    m_duration = m_pathDuration;
    for (int i = 0; i < m_positions.size(); i++) {
        if (i > 0)
            pathLen += (m_positions[i] - m_positions[i - 1]).length();
        m_implicitTimes[i] = int(m_duration * pathLen / distance);
    }
    m_implicitLoopBackDuration = m_duration - (m_duration * pathLen / distance);
}

void Path3D::updateCurrentTime(int currentTime)
{
    if (currentTime == m_prevTime)
        return;
    if (m_times.size() == 0 && m_positions.size() != 0 && m_pathDuration > 0)
        calculateImplicitTimes();
    else
        if (!(m_times.size() == m_positions.size() && (!m_orientations.size() || m_times.size() == m_orientations.size())))
        return;
    m_prevTime = currentTime;
    const auto times = m_times.size() ? m_times : m_implicitTimes;
    for (int i = 0; i < m_nodes.size(); i++) {
        int pos = int(i * m_duration / m_nodes.size() + currentTime) % m_duration;
        int key = -1;
        for (int j = 0; j < times.size(); j++) {
            if (pos <= times[j]) {
                key = j;
                break;
            }
        }
        if (key == 0) {
            // first key
            m_nodes[i]->setPosition(m_positions[0]);
            if (m_orientations.size())
                m_nodes[i]->setEulerRotation(m_orientations[key]);
        } else if (key == -1) {
            // after last time
            float t = float(pos - times.last());
            if (m_implicitLoopBackDuration > 0)
                t /= m_implicitLoopBackDuration;
            else if (m_loopBackDuration > 0)
                t /= float(m_loopBackDuration);
            else
                continue;
            QVector3D pos = lerp(m_positions.last(), m_positions.first(), t);
            m_nodes[i]->setPosition(pos);
            if (m_orientations.size()) {
                QVector3D orient = eullerp(m_orientations.last(), m_orientations.first(), t);
                m_nodes[i]->setEulerRotation(orient);
            }
        } else {
            // inbetween
            float t = float(pos - times[key - 1]) / float(times[key] - times[key - 1]);
            QVector3D pos = lerp(m_positions[key - 1], m_positions[key], t);
            m_nodes[i]->setPosition(pos);
            if (m_orientations.size()) {
                QVector3D orient = eullerp(m_orientations[key - 1], m_orientations[key], t);
                m_nodes[i]->setEulerRotation(orient);
            }
        }
    }
}

static inline QMatrix4x4 toMat4(const QMatrix3x3 &matrix3x3)
{
    QMatrix4x4 matrix4x4;

    // Set the values from QMatrix3x3
    matrix4x4(0, 0) = matrix3x3(0, 0);
    matrix4x4(0, 1) = matrix3x3(0, 1);
    matrix4x4(0, 2) = matrix3x3(0, 2);
    matrix4x4(1, 0) = matrix3x3(1, 0);
    matrix4x4(1, 1) = matrix3x3(1, 1);
    matrix4x4(1, 2) = matrix3x3(1, 2);
    matrix4x4(2, 0) = matrix3x3(2, 0);
    matrix4x4(2, 1) = matrix3x3(2, 1);
    matrix4x4(2, 2) = matrix3x3(2, 2);

    // Set the last row and column
    matrix4x4(3, 0) = 0;
    matrix4x4(3, 1) = 0;
    matrix4x4(3, 2) = 0;
    matrix4x4(3, 3) = 1;
    return matrix4x4;
}

QList<QVector3D> Path3D::circlePath(const QVector3D &center, qreal radius, const QVector3D &orientation, int count)
{
    QList<QVector3D> ret;
    ret.resize(count);
    float aperc = 2.0f * M_PI / float(count);
    QQuaternion q = QQuaternion::fromEulerAngles(orientation);
    QMatrix4x4 mat = toMat4(q.toRotationMatrix());
    for (int i = 0; i < count; i++) {
        QVector3D p = QVector3D(cos(i * aperc) * radius, sin(i * aperc) * radius, 0);
        p = mat.map(QVector4D(p, 1.0)).toVector3D();
        p += center;
        ret[i] = p;
    }
    return ret;
}
