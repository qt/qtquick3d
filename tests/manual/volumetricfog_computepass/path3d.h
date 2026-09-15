// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PATH3D_H
#define PATH3D_H

#include <qmath.h>
#include <qobject.h>
#include <qlist.h>
#include <qvector3d.h>
#include <qabstractanimation.h>
#include <private/qquick3dnode_p.h>

class Path3D : public QQuick3DNode
{
    Q_OBJECT
    Q_PROPERTY(int pathDuration READ pathDuration WRITE setPathDuration NOTIFY pathDurationChanged FINAL)
    Q_PROPERTY(int loopBackDuration READ loopBackDuration WRITE setLoopBackDuration NOTIFY loopBackDurationChanged FINAL)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged FINAL)
    Q_PROPERTY(QList<int> times READ times WRITE setTimes NOTIFY timesChanged FINAL)
    Q_PROPERTY(QList<QVector3D> positions READ positions WRITE setPositions NOTIFY positionsChanged FINAL)
    Q_PROPERTY(QList<QVector3D> orientations READ orientations WRITE setOrientations NOTIFY orientationsChanged FINAL)
    QML_NAMED_ELEMENT(Path3D)
public:
    explicit Path3D(QQuick3DNode *parent = nullptr);
    ~Path3D() override;

    QList<int> times() const;
    QList<QVector3D> positions() const;
    QList<QVector3D> orientations() const;
    int loopBackDuration() const;
    int duration() const;
    int pathDuration() const;

    Q_INVOKABLE QList<QVector3D> circlePath(const QVector3D &center, qreal radius, const QVector3D &orientation, int count);

public Q_SLOTS:
    void setTimes(const QList<int> &times);
    void setPositions(const QList<QVector3D> &positions);
    void setOrientations(const QList<QVector3D> &orientations);
    void setLoopBackDuration(int duration);
    void setPathDuration(int duration);
Q_SIGNALS:
    void timesChanged();
    void positionsChanged();
    void orientationsChanged();
    void loopBackDurationChanged();
    void durationChanged();
    void pathDurationChanged();

protected:
    void componentComplete() override;

private:
    void calculateImplicitTimes();
    void updateCurrentTime(int currentTime);
    class Animation : public QAbstractAnimation
    {
    public:
        Animation(Path3D *parent)
            : QAbstractAnimation(parent), m_path3d(parent) {}
        void updateCurrentTime(int currentTime) override
        {
            m_path3d->updateCurrentTime(currentTime);
        }
        int duration() const override
        {
            return -1;
        }
    private:
        Path3D *m_path3d;
    };

    QPointer<Animation> m_animation;
    QList<int> m_times;
    QList<int> m_implicitTimes;
    QList<QVector3D> m_positions;
    QList<QVector3D> m_orientations;
    QList<QQuick3DNode*> m_nodes;
    int m_implicitLoopBackDuration = 0;
    int m_loopBackDuration = 0;
    int m_duration = 0;
    int m_prevTime = -1;
    int m_pathDuration = 0;
    friend class Animation;
};

#endif

