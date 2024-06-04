// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXRHANDTRACKERINPUT_H
#define QQUICK3DXRHANDTRACKERINPUT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <QVector2D>
#include <QVector3D>
#include <QQuaternion>
#include <QtQml/qqml.h>
#include <private/qquick3dmodel_p.h>

#include <QQuick3DGeometry>
#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DXrHandTrackerInputPrivate;

class QQuick3DXrHandTrackerInput : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuick3DXrHandTrackerInput)

    Q_PROPERTY(bool isActive READ isActive NOTIFY isActiveChanged)
    Q_PROPERTY(HandPoseSpace poseSpace READ poseSpace WRITE setPoseSpace NOTIFY poseSpaceChanged)
    Q_PROPERTY(QVector3D posePosition READ posePosition NOTIFY posePositionChanged)
    Q_PROPERTY(QList<QVector3D> jointPositions READ jointPositions NOTIFY jointPositionsChanged FINAL)
    Q_PROPERTY(QList<QQuaternion> jointRotations READ jointRotations NOTIFY jointRotationsChanged FINAL)
    Q_PROPERTY(QVector3D pokePosition READ pokePosition NOTIFY pokePositionChanged FINAL)

    QML_NAMED_ELEMENT(XrHandTrackerInput)
    QML_UNCREATABLE("Created by XrView")
    QML_ADDED_IN_VERSION(6, 8)

public:
    using HandPoseSpace = QtQuick3DXr::HandPoseSpace;
    Q_ENUM(HandPoseSpace)

    explicit QQuick3DXrHandTrackerInput(QObject *parent = nullptr);
    ~QQuick3DXrHandTrackerInput() override;

    bool isActive() const;
    HandPoseSpace poseSpace() const;
    void setIsActive(bool isActive);

    const QVector3D &posePosition() const;
    const QQuaternion &poseRotation() const;

    QList<QVector3D> jointPositions() const;
    void setJointPositionsAndRotations(const QList<QVector3D> &newJointPositions, const QList<QQuaternion> &newJointRotations);

    QList<QQuaternion> jointRotations() const;

    QVector3D pokePosition() const;
    void setPokePosition(const QVector3D &newPokePosition);

public Q_SLOTS:
    void setPoseSpace(HandPoseSpace poseSpace);

Q_SIGNALS:
    void isActiveChanged();
    void poseSpaceChanged();

    void posePositionChanged();
    void poseRotationChanged();

    void jointPositionsChanged();
    void jointDataUpdated();

    void jointRotationsChanged();

    void pokePositionChanged();

private:
    bool m_isActive = false;
    HandPoseSpace m_poseSpace = HandPoseSpace::GripPose;

    QVector3D m_posePosition;
    QQuaternion m_poseRotation;

    QList<QVector3D> m_jointPositions;
    QList<QQuaternion> m_jointRotations;
    QVector3D m_pokePosition;

    std::unique_ptr<QQuick3DXrHandTrackerInputPrivate> d_ptr;
};

class QQuick3DXrHandModel : public QQuick3DModel
{
    Q_OBJECT

    Q_PROPERTY(QQuick3DXrHandTrackerInput *handTracker READ handTracker WRITE setHandTracker NOTIFY handTrackerChanged FINAL)

    QML_NAMED_ELEMENT(XrHandModel)
    QML_ADDED_IN_VERSION(6, 8)

public:
    QQuick3DXrHandModel(QQuick3DNode *parent = nullptr);

    void componentComplete() override;

    QQuick3DXrHandTrackerInput *handTracker() const;
    void setHandTracker(QQuick3DXrHandTrackerInput *newHandTracker);

Q_SIGNALS:
    void handChanged();
    void handTrackerChanged();

private Q_SLOTS:
    void updatePose();

private:
    void setupModel();
    QQuick3DXrHandTrackerInput *m_handTracker = nullptr;
    bool m_initialized = false;
};

QT_END_NAMESPACE

#endif // QQUICK3DXRHANDTRACKERINPUT_H
