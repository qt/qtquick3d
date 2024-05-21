// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxrspaceextension_p.h"
#include "openxr/qopenxrhelpers_p.h"
#include "qopenxrspatialanchor_p.h"

#if defined(Q_OS_ANDROID)
# include <QtCore/private/qandroidextras_p.h>
#endif

QT_BEGIN_NAMESPACE

//<uses-permission android:name="com.oculus.permission.USE_ANCHOR_API" />

static const uint32_t MAX_PERSISTENT_SPACES = 100;

QOpenXRSpaceExtension::QOpenXRSpaceExtension()
{

}

QOpenXRSpaceExtension::~QOpenXRSpaceExtension()
{

}

QOpenXRSpaceExtension *QOpenXRSpaceExtension::instance()
{
    static QOpenXRSpaceExtension instance;
    return &instance;
}

void QOpenXRSpaceExtension::initialize(XrInstance instance, XrSession session)
{
#if defined(Q_OS_ANDROID)
    auto res = QtAndroidPrivate::requestPermission(QLatin1StringView("com.oculus.permission.USE_SCENE"));
    res.waitForFinished();
#endif

    m_instance = instance;
    m_session = session;

    // Get the function pointers
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrEnumerateSpaceSupportedComponentsFB",
                                        (PFN_xrVoidFunction*)(&xrEnumerateSpaceSupportedComponentsFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceComponentStatusFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceComponentStatusFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrSetSpaceComponentStatusFB",
                                        (PFN_xrVoidFunction*)(&xrSetSpaceComponentStatusFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceUuidFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceUuidFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrQuerySpacesFB",
                                        (PFN_xrVoidFunction*)(&xrQuerySpacesFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrRetrieveSpaceQueryResultsFB",
                                        (PFN_xrVoidFunction*)(&xrRetrieveSpaceQueryResultsFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceBoundingBox2DFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceBoundingBox2DFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceBoundingBox3DFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceBoundingBox3DFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceSemanticLabelsFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceSemanticLabelsFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceBoundary2DFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceBoundary2DFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceRoomLayoutFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceRoomLayoutFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrGetSpaceContainerFB",
                                        (PFN_xrVoidFunction*)(&xrGetSpaceContainerFB)));
    checkXrResult(xrGetInstanceProcAddr(m_instance,
                                        "xrRequestSceneCaptureFB",
                                        (PFN_xrVoidFunction*)(&xrRequestSceneCaptureFB)));
}

void QOpenXRSpaceExtension::teardown()
{

}

QList<const char *> QOpenXRSpaceExtension::requiredExtensions() const
{
    return {
        // XR_EXT_UUID_EXTENSION_NAME, // ### Crashes on Quest 3. Theoretically required for XR_FB_spatial_entity (would work anyway, but is a validation error)
        XR_FB_SPATIAL_ENTITY_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_CONTAINER_EXTENSION_NAME,
        XR_FB_SCENE_EXTENSION_NAME,
#ifdef Q_OS_ANDROID // no scene capture with Simulator as of version 57
        XR_FB_SCENE_CAPTURE_EXTENSION_NAME
#endif
    };
}

void QOpenXRSpaceExtension::handleEvent(const XrEventDataBaseHeader *event)
{
    if (event->type == XR_TYPE_EVENT_DATA_SPACE_SET_STATUS_COMPLETE_FB) {
        qDebug("QOpenXRSpaceExtension::handleEvent: received XR_TYPE_EVENT_DATA_SPACE_SET_STATUS_COMPLETE_FB");
        const XrEventDataSpaceSetStatusCompleteFB* setStatusComplete = (const XrEventDataSpaceSetStatusCompleteFB*)(event);
        if (setStatusComplete->result == XR_SUCCESS) {
            if (setStatusComplete->componentType == XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB) {
                addAnchor(setStatusComplete->space, setStatusComplete->uuid);
            }
        }
    } else if (event->type == XR_TYPE_EVENT_DATA_SCENE_CAPTURE_COMPLETE_FB) {
        qDebug("QOpenXRSpaceExtension::handleEvent: received XR_TYPE_EVENT_DATA_SCENE_CAPTURE_COMPLETE_FB");
        const XrEventDataSceneCaptureCompleteFB* captureResult = (const XrEventDataSceneCaptureCompleteFB*)(event);
        if (captureResult->result == XR_SUCCESS) {
            Q_EMIT sceneCaptureCompleted();
            qDebug(
                "QOpenXRSpaceExtension::handleEvent: Scene capture (ID = %llu) succeeded",
                static_cast<long long unsigned int>(captureResult->requestId));
        } else {
            qDebug(
                "QOpenXRSpaceExtension::handleEvent: Scene capture (ID = %llu) failed with an error %d",
                static_cast<long long unsigned int>(captureResult->requestId),
                captureResult->result);
        }

    } else if (event->type == XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB) {
        qDebug("QOpenXRSpaceExtension::handleEvent: received XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB");
        const XrEventDataSpaceQueryResultsAvailableFB* resultsAvailable = (const XrEventDataSpaceQueryResultsAvailableFB*)(event);

        XrSpaceQueryResultsFB queryResults{};
        queryResults.type = XR_TYPE_SPACE_QUERY_RESULTS_FB;
        queryResults.resultCapacityInput = 0;
        queryResults.resultCountOutput = 0;
        queryResults.results = nullptr;

        checkXrResult(retrieveSpaceQueryResults(resultsAvailable->requestId, &queryResults));

        QVector<XrSpaceQueryResultFB> results(queryResults.resultCountOutput);
        queryResults.resultCapacityInput = results.size();
        queryResults.resultCountOutput = 0;
        queryResults.results = results.data();

        checkXrResult(retrieveSpaceQueryResults(resultsAvailable->requestId, &queryResults));

        qDebug("retrieveSpaceQueryResults: num of results received: %d", queryResults.resultCountOutput);
        for (const auto &result : results) {
            if (isComponentSupported(result.space, XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB)) {
                XrSpaceComponentStatusSetInfoFB request = {
                                                            XR_TYPE_SPACE_COMPONENT_STATUS_SET_INFO_FB,
                                                            nullptr,
                                                            XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB,
                                                            true,
                                                            0
                };
                XrAsyncRequestIdFB requestId;
                XrResult res = setSpaceComponentStatus(result.space, &request, &requestId);
                if (res == XR_ERROR_SPACE_COMPONENT_STATUS_ALREADY_SET_FB) {
                    addAnchor(result.space, result.uuid);
                }
            }
        }
    } else if (event->type == XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB) {
        qDebug("QOpenXRSpaceExtension::handleEvent: received XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB");
    }
}

void QOpenXRSpaceExtension::requestSceneCapture()
{
    XrAsyncRequestIdFB requestId;
    XrSceneCaptureRequestInfoFB request{};
    request.type = XR_TYPE_SCENE_CAPTURE_REQUEST_INFO_FB;
    request.requestByteCount = 0;
    request.request = nullptr;
    checkXrResult(requestSceneCapture(&request, &requestId));
}

bool QOpenXRSpaceExtension::isComponentSupported(XrSpace space, XrSpaceComponentTypeFB type) {
    uint32_t numComponents = 0;
    checkXrResult(enumerateSpaceSupportedComponents(space, 0, &numComponents, nullptr));
    QVector<XrSpaceComponentTypeFB> components(numComponents);
    checkXrResult(enumerateSpaceSupportedComponents(space, numComponents, &numComponents, components.data()));

    bool supported = false;
    for (const auto &component : components) {
        if (component == type) {
            supported = true;
            break;
        }
    }

    return supported;
}

bool QOpenXRSpaceExtension::isComponentEnabled(XrSpace space, XrSpaceComponentTypeFB type)
{
    XrSpaceComponentStatusFB status = {XR_TYPE_SPACE_COMPONENT_STATUS_FB, nullptr, 0, 0};
    checkXrResult(getSpaceComponentStatus(space, type, &status));
    return (status.enabled && !status.changePending);
}

bool QOpenXRSpaceExtension::getBoundingBox2D(XrSpace space, QVector2D &offset, QVector2D &extent)
{
    if (isComponentEnabled(space, XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB)) {
        // Get the 2D bounds
        XrRect2Df boundingBox2D;
        checkXrResult(getSpaceBoundingBox2D(space, &boundingBox2D));
        offset = QVector2D(boundingBox2D.offset.x, boundingBox2D.offset.y);
        extent = QVector2D(boundingBox2D.extent.width, boundingBox2D.extent.height);
        return true;
    }
    return false;
}

bool QOpenXRSpaceExtension::getBoundingBox3D(XrSpace space, QVector3D &offset, QVector3D &extent)
{
    if (isComponentEnabled(space, XR_SPACE_COMPONENT_TYPE_BOUNDED_3D_FB)) {
        // Get the 3D bounds
        XrRect3DfFB boundingBox3D;
        checkXrResult(getSpaceBoundingBox3D(space, &boundingBox3D));
        offset = QVector3D(boundingBox3D.offset.x, boundingBox3D.offset.y, boundingBox3D.offset.z);
        extent = QVector3D(boundingBox3D.extent.width, boundingBox3D.extent.height, boundingBox3D.extent.depth);
        return true;
    }

    return false;
}

namespace {
bool isValidUuid(const XrUuidEXT& uuid) {
    for (int i = 0; i < XR_UUID_SIZE_EXT; ++i) {
        if (uuid.data[i] > 0) {
            return true;
        }
    }
    return false;
}

QUuid fromXrUuidExt(XrUuidEXT uuid) {
    return QUuid::fromBytes(uuid.data);
}

XrUuidEXT fromQUuid(QUuid uuid) {
    XrUuidEXT xrUuid;
    auto bytes = uuid.toBytes();
    memcpy(xrUuid.data, bytes.data, XR_UUID_SIZE_EXT);
    return xrUuid;
}

}

QSet<QUuid> QOpenXRSpaceExtension::collectRoomLayoutUuids(XrSpace space) {
    XrRoomLayoutFB roomLayout{};
    roomLayout.type = XR_TYPE_ROOM_LAYOUT_FB;
    QVector<XrUuidEXT> wallUuids;
    QSet<QUuid> uuidSet;

    // First call
    checkXrResult(getSpaceRoomLayout(space, &roomLayout));
    // If wallUuidCountOutput == 0, no walls in the component. The UUIDs of the ceiling and floor
    // has been returned if available
    if (roomLayout.wallUuidCountOutput != 0) {
        // Second call
        wallUuids.resize(roomLayout.wallUuidCountOutput);
        roomLayout.wallUuidCapacityInput = wallUuids.size();
        roomLayout.wallUuids = wallUuids.data();
        checkXrResult(getSpaceRoomLayout(space, &roomLayout));
    }
    if (isValidUuid(roomLayout.floorUuid))
        uuidSet.insert(fromXrUuidExt(roomLayout.floorUuid));
    if (isValidUuid(roomLayout.ceilingUuid))
        uuidSet.insert(fromXrUuidExt(roomLayout.ceilingUuid));
    for (uint32_t i = 0; i < roomLayout.wallUuidCountOutput; i++)
        uuidSet.insert(fromXrUuidExt(roomLayout.wallUuids[i]));
    return uuidSet;
}

QSet<QUuid> QOpenXRSpaceExtension::collectSpaceContainerUuids(XrSpace space) {
    XrSpaceContainerFB spaceContainer{};
    spaceContainer.type = XR_TYPE_SPACE_CONTAINER_FB;
    QSet<QUuid> uuidSet;
    // First call
    checkXrResult(getSpaceContainer(space, &spaceContainer));
    if (spaceContainer.uuidCountOutput != 0) {
        // Second call
        QVector<XrUuidEXT> uuids(spaceContainer.uuidCountOutput);
        spaceContainer.uuidCapacityInput = uuids.size();
        spaceContainer.uuids = uuids.data();
        checkXrResult(getSpaceContainer(space, &spaceContainer));

        for (uint32_t i = 0; i < spaceContainer.uuidCountOutput; i++)
            uuidSet.insert(fromXrUuidExt(spaceContainer.uuids[i]));
    }

    return uuidSet;
}

QString QOpenXRSpaceExtension::getSemanticLabels(const XrSpace space) {
    static const std::string recognizedLabels = "TABLE,COUCH,FLOOR,CEILING,WALL_FACE,WINDOW_FRAME,DOOR_FRAME,STORAGE,BED,SCREEN,LAMP,PLANT,WALL_ART,OTHER";
    const XrSemanticLabelsSupportInfoFB semanticLabelsSupportInfo = {
                                                                     XR_TYPE_SEMANTIC_LABELS_SUPPORT_INFO_FB,
                                                                     nullptr,
                                                                     XR_SEMANTIC_LABELS_SUPPORT_MULTIPLE_SEMANTIC_LABELS_BIT_FB | XR_SEMANTIC_LABELS_SUPPORT_ACCEPT_DESK_TO_TABLE_MIGRATION_BIT_FB,
                                                                     recognizedLabels.c_str()
    };

    XrSemanticLabelsFB labels{};
    labels.type = XR_TYPE_SEMANTIC_LABELS_FB;
    labels.next = &semanticLabelsSupportInfo;

    if (!isComponentEnabled(space, XR_SPACE_COMPONENT_TYPE_SEMANTIC_LABELS_FB))
        return QString();

    // First call.
    checkXrResult(getSpaceSemanticLabels(space, &labels));
    // Second call
    QByteArray labelData(labels.bufferCountOutput, Qt::Uninitialized);
    labels.bufferCapacityInput = labelData.size();
    labels.buffer = labelData.data();
    checkXrResult(getSpaceSemanticLabels(space, &labels));

    return QString::fromLocal8Bit(labelData);
}

bool QOpenXRSpaceExtension::queryAllAnchors() {
    XrSpaceQueryInfoFB queryInfo = {
                                    XR_TYPE_SPACE_QUERY_INFO_FB,
                                    nullptr,
                                    XR_SPACE_QUERY_ACTION_LOAD_FB,
                                    MAX_PERSISTENT_SPACES,
                                    0,
                                    nullptr,
                                    nullptr};

    XrAsyncRequestIdFB requestId;
    return checkXrResult(querySpaces((XrSpaceQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
}

const QList<QOpenXRSpatialAnchor *> &QOpenXRSpaceExtension::anchors() const
{
    return m_anchors;
}

void QOpenXRSpaceExtension::updateAnchors(XrTime predictedDisplayTime, XrSpace appSpace)
{
    // basically for each anchor, we need to call xrLocateSpace
    for (auto &anchor : m_anchors) {
        XrSpaceLocation spaceLocation{};
        spaceLocation.type = XR_TYPE_SPACE_LOCATION;
        XrResult res = xrLocateSpace(anchor->space(), appSpace, predictedDisplayTime, &spaceLocation);
        checkXrResult(res);
        if (XR_UNQUALIFIED_SUCCESS(res)) {
            if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {

                // Update transform
                anchor->setPosition(QVector3D(spaceLocation.pose.position.x,
                                              spaceLocation.pose.position.y,
                                              spaceLocation.pose.position.z) * 100.0f);
                anchor->setRotation(QQuaternion(spaceLocation.pose.orientation.w,
                                                spaceLocation.pose.orientation.x,
                                                spaceLocation.pose.orientation.y,
                                                spaceLocation.pose.orientation.z));
            }
        }
    }
}

bool QOpenXRSpaceExtension::queryAllAnchorsWithSpecificComponentEnabled(const XrSpaceComponentTypeFB componentType) {
    XrSpaceStorageLocationFilterInfoFB storageLocationFilterInfo = {
                                                                        XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB,
                                                                        nullptr,
                                                                        XR_SPACE_STORAGE_LOCATION_LOCAL_FB
                                                                    };

    XrSpaceComponentFilterInfoFB componentFilterInfo = {
                                                            XR_TYPE_SPACE_COMPONENT_FILTER_INFO_FB,
                                                            &storageLocationFilterInfo,
                                                            componentType
                                                        };

    XrSpaceQueryInfoFB queryInfo = {
                                        XR_TYPE_SPACE_QUERY_INFO_FB,
                                        nullptr,
                                        XR_SPACE_QUERY_ACTION_LOAD_FB,
                                        MAX_PERSISTENT_SPACES,
                                        0,
                                        (XrSpaceFilterInfoBaseHeaderFB*)&componentFilterInfo,
                                        nullptr
                                    };

    XrAsyncRequestIdFB requestId;
    checkXrResult(querySpaces((XrSpaceQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
    return true;
}

bool QOpenXRSpaceExtension::queryAnchorsByUuids(const QSet<QUuid>& uuidSet) {
    if (uuidSet.isEmpty())
        return false;

    QVector<XrUuidEXT> uuidsToQuery;

    for (const auto &uuid : uuidSet) {
        XrUuidEXT xrUuid = fromQUuid(uuid);
        uuidsToQuery.append(xrUuid);
    }

    XrSpaceStorageLocationFilterInfoFB storageLocationFilterInfo = {
                                                                        XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB,
                                                                        nullptr,
                                                                        XR_SPACE_STORAGE_LOCATION_LOCAL_FB
                                                                    };

    XrSpaceUuidFilterInfoFB uuidFilterInfo = {
                                                XR_TYPE_SPACE_UUID_FILTER_INFO_FB,
                                                &storageLocationFilterInfo,
                                                (uint32_t)uuidsToQuery.size(),
                                                uuidsToQuery.data()
                                            };

    XrSpaceQueryInfoFB queryInfo = {
                                        XR_TYPE_SPACE_QUERY_INFO_FB,
                                        nullptr,
                                        XR_SPACE_QUERY_ACTION_LOAD_FB,
                                        MAX_PERSISTENT_SPACES,
                                        0,
                                        (XrSpaceFilterInfoBaseHeaderFB*)&uuidFilterInfo,
                                        nullptr
                                    };

    XrAsyncRequestIdFB requestId;
    checkXrResult(querySpaces((XrSpaceQueryInfoBaseHeaderFB*)&queryInfo, &requestId));
    return true;
}

void QOpenXRSpaceExtension::addAnchor(XrSpace space, XrUuidEXT uuid)
{
    auto quuid = fromXrUuidExt(uuid);
    // Check if we already have this anchor
    if (m_anchorsByUuid.contains(quuid))
        return;

    auto anchor = new QOpenXRSpatialAnchor(space, quuid, this);
    m_anchorsByUuid.insert(quuid, anchor);
    m_anchors.append(anchor);
    Q_EMIT anchorAdded(anchor);
}


XrResult QOpenXRSpaceExtension::enumerateSpaceSupportedComponents(XrSpace space, uint32_t componentTypeCapacityInput, uint32_t *componentTypeCountOutput, XrSpaceComponentTypeFB *componentTypes)
{
    return xrEnumerateSpaceSupportedComponentsFB(space, componentTypeCapacityInput, componentTypeCountOutput, componentTypes);
}

XrResult QOpenXRSpaceExtension::getSpaceComponentStatus(XrSpace space, XrSpaceComponentTypeFB componentType, XrSpaceComponentStatusFB *status)
{
    return xrGetSpaceComponentStatusFB(space, componentType, status);
}

XrResult QOpenXRSpaceExtension::setSpaceComponentStatus(XrSpace space, const XrSpaceComponentStatusSetInfoFB *info, XrAsyncRequestIdFB *requestId)
{
    return xrSetSpaceComponentStatusFB(space, info, requestId);
}

XrResult QOpenXRSpaceExtension::getSpaceUuid(XrSpace space, XrUuidEXT *uuid)
{
    return xrGetSpaceUuidFB(space, uuid);
}

XrResult QOpenXRSpaceExtension::querySpaces(const XrSpaceQueryInfoBaseHeaderFB *info, XrAsyncRequestIdFB *requestId)
{
    return xrQuerySpacesFB(m_session, info, requestId);
}

XrResult QOpenXRSpaceExtension::retrieveSpaceQueryResults(XrAsyncRequestIdFB requestId, XrSpaceQueryResultsFB *results)
{
    return xrRetrieveSpaceQueryResultsFB(m_session, requestId, results);
}

XrResult QOpenXRSpaceExtension::getSpaceBoundingBox2D(XrSpace space, XrRect2Df *boundingBox2DOutput)
{
    return xrGetSpaceBoundingBox2DFB(m_session, space, boundingBox2DOutput);
}

XrResult QOpenXRSpaceExtension::getSpaceBoundingBox3D(XrSpace space, XrRect3DfFB *boundingBox3DOutput)
{
    return xrGetSpaceBoundingBox3DFB(m_session, space, boundingBox3DOutput);
}

XrResult QOpenXRSpaceExtension::getSpaceSemanticLabels(XrSpace space, XrSemanticLabelsFB *semanticLabelsOutput)
{
    return xrGetSpaceSemanticLabelsFB(m_session, space, semanticLabelsOutput);
}

XrResult QOpenXRSpaceExtension::getSpaceBoundary2D(XrSpace space, XrBoundary2DFB *boundary2DOutput)
{
    return xrGetSpaceBoundary2DFB(m_session, space, boundary2DOutput);
}

XrResult QOpenXRSpaceExtension::getSpaceRoomLayout(XrSpace space, XrRoomLayoutFB *roomLayoutOutput)
{
    return xrGetSpaceRoomLayoutFB(m_session, space, roomLayoutOutput);
}

XrResult QOpenXRSpaceExtension::getSpaceContainer(XrSpace space, XrSpaceContainerFB *spaceContainerOutput)
{
    return xrGetSpaceContainerFB(m_session, space, spaceContainerOutput);
}

XrResult QOpenXRSpaceExtension::requestSceneCapture(const XrSceneCaptureRequestInfoFB *info, XrAsyncRequestIdFB *requestId)
{
    if (xrRequestSceneCaptureFB)
        return xrRequestSceneCaptureFB(m_session, info, requestId);
    else
        return XR_ERROR_FUNCTION_UNSUPPORTED;
}

bool QOpenXRSpaceExtension::checkXrResult(const XrResult &result)
{
    return OpenXRHelpers::checkXrResult(result, m_instance);
}

QT_END_NAMESPACE
