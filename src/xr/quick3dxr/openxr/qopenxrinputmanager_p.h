// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICK3DXINPUTMANAGER_OPENXR_P_H
#define QQUICK3DXINPUTMANAGER_OPENXR_P_H

#include <QObject>

#include <openxr/openxr.h>
#include <functional>
#include "qquick3dxractionmapper_p.h"
#include <QtQuick3DXr/private/qtquick3dxrglobal_p.h>

#include <private/qquick3dmodel_p.h>

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

QT_BEGIN_NAMESPACE

class QQuick3DXrHandInput;
class QQuick3DXrInputManager;
class QQuick3DXrHandModel;
class QQuick3DXrController;

class QQuick3DXrInputManagerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(QQuick3DXrInputManager)
public:
    explicit QQuick3DXrInputManagerPrivate(QQuick3DXrInputManager &manager);
    ~QQuick3DXrInputManagerPrivate();

    void init(XrInstance instance, XrSession session);
    void teardown();

    bool isValid() const { return m_initialized; }

    static QQuick3DXrInputManagerPrivate *get(QQuick3DXrInputManager *inputManager);

    using Hand = QtQuick3DXr::Hand;
    using HandPoseSpace = QtQuick3DXr::HandPoseSpace;

    void pollActions();
    void updatePoses(XrTime predictedDisplayTime, XrSpace appSpace);
    void updateHandtracking(XrTime predictedDisplayTime, XrSpace appSpace, bool aimExtensionEnabled);

    XrSpace handSpace(Hand hand, HandPoseSpace poseSpace);
    bool isHandActive(Hand hand);
    bool isHandTrackerActive(Hand hand);

    void setPosePositionAndRotation(Hand hand, HandPoseSpace poseSpace, const QVector3D &position, const QQuaternion &rotation);

    QQuick3DXrHandInput *leftHandInput() const;
    QQuick3DXrHandInput *rightHandInput() const;

    void setupHandModel(QQuick3DXrHandModel *model);
    void registerController(QQuick3DXrController *controller);
    void unregisterController(QQuick3DXrController *controller);

    bool isPoseInUse(Hand hand, HandPoseSpace poseSpace);

    // NOTE: Static for now...
    qsizetype getPokeJointIndex() const { return qsizetype(XR_HAND_JOINT_INDEX_TIP_EXT); }

    PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_;
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_;
    PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_;

    PFN_xrGetHandMeshFB xrGetHandMeshFB_;

    XrHandTrackerEXT handTracker[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};

    XrHandJointLocationEXT jointLocations[2][XR_HAND_JOINT_COUNT_EXT];
    XrHandJointVelocityEXT jointVelocities[2][XR_HAND_JOINT_COUNT_EXT];

private:
    void setupHandModelInternal(QQuick3DXrHandModel *model, Hand hand);

    void setupHandTracking();
    bool queryHandMesh(Hand hand);
    void setupActions();
    void destroyActions();
    [[nodiscard]] bool checkXrResult(const XrResult &result);
    bool resolveXrFunction(const char *name, PFN_xrVoidFunction *function);
    void setPath(XrPath &path, const QByteArray &pathString);

    void createAction(XrActionType type,
                      const char *name,
                      const char *localizedName,
                      int numSubactions,
                      XrPath *subactionPath,
                      XrAction &action);
    void getBoolInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(bool)> setter);
    void getFloatInputState(XrActionStateGetInfo &getInfo, const XrAction &action, std::function<void(float)> setter);

    void setInputValue(Hand hand, int id, const char *shortName, float value);

    QQuick3DXrInputManager *q_ptr = nullptr;

    XrInstance m_instance{XR_NULL_HANDLE};
    XrSession m_session{XR_NULL_HANDLE};

    enum SubPathSelector {NoSubPath = 0, LeftHandSubPath = 1, RightHandSubPath = 2, BothHandsSubPath = 3};

    struct QXRHandComponentPath
    {
        XrPath paths[2] = {{}, {}};
        QByteArray componentPathString;
    };
    QXRHandComponentPath makeHandInputPaths(const QByteArrayView path);
    XrPath makeInputPath(const QByteArrayView path);

    struct InputActionInfo {
        QQuick3DXrInputAction::Action id;
        const char *shortName;
        const char *localizedName;
        XrActionType type;
    };

    QList<InputActionInfo> m_handInputActionDefs;

    struct HandActions {
        XrAction gripPoseAction{XR_NULL_HANDLE};
        XrAction aimPoseAction{XR_NULL_HANDLE};
        XrAction hapticAction{XR_NULL_HANDLE};
    };

    // Input State
    XrActionSet m_actionSet{XR_NULL_HANDLE};
    XrPath m_handSubactionPath[2];
    XrSpace m_handGripSpace[2];
    XrSpace m_handAimSpace[2];

    QQuick3DXrHandInput *m_handInputState[2];
    HandActions m_handActions;
    XrAction m_inputActions[QQuick3DXrInputAction::NumActions] = {};
    QSet<QQuick3DXrController *> m_controllers;
    bool m_poseInUse[2][2] = {};
    bool m_poseUsageDirty = true;

    uint m_aimStateFlags[2] = {};
    bool m_initialized = false;
    bool m_validAimStateFromUpdatePoses[2] = {false, false};

    // Hand Mesh Data
    struct HandMeshData {
        QVector<XrVector3f> vertexPositions;
        QVector<XrVector3f> vertexNormals;
        QVector<XrVector2f> vertexUVs;
        QVector<XrVector4sFB> vertexBlendIndices;
        QVector<XrVector4f> vertexBlendWeights;
        QVector<int16_t> indices;
        XrPosef jointBindPoses[XR_HAND_JOINT_COUNT_EXT];
        XrHandJointEXT jointParents[XR_HAND_JOINT_COUNT_EXT];
        float jointRadii[XR_HAND_JOINT_COUNT_EXT];
    } m_handMeshData[2];


    struct HandGeometryData {
        QQuick3DGeometry *geometry = nullptr;
    } m_handGeometryData[2];

    QQuick3DGeometry *createHandMeshGeometry(const HandMeshData &handMeshData);
    void createHandModelData(Hand hand);
    friend class QOpenXrHandModel;
};

QT_END_NAMESPACE

#endif // QQUICK3DXINPUTMANAGER_OPENXR_P_H
