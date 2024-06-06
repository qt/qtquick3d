// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTQUICK3DXRGLOBAL_P_H
#define QTQUICK3DXRGLOBAL_P_H

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

#include <QtQuick3DXr/qtquick3dxrglobal.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

// See, and match, QQuick3DXrView::FoveationLevel
namespace QtQuick3DXr
{

enum FoveationLevel {
    NoFoveation = 0,
    LowFoveation = 1,
    MediumFoveation = 2,
    HighFoveation = 3
};

// See, and match, QQuick3DXrView::ReferenceSpace
enum class ReferenceSpace {
    ReferenceSpaceUnknown,
    ReferenceSpaceLocal,
    ReferenceSpaceStage,
    ReferenceSpaceLocalFloor
};

enum Hand {
    LeftHand = 0,
    RightHand = 1,
};

enum class HandPoseSpace {
    GripPose,
    AimPose,
    PinchPose,
    PokePose
};

enum class XrSpaceId : quint64 { Invalid = 0 };

template<typename XrSpaceT>
XrSpaceT fromXrSpaceId(XrSpaceId space)
{
    static_assert(sizeof(XrSpaceT) <= sizeof(XrSpaceId), "XrSpaceT's size must equal or smaller than XrSpaceId");
    if constexpr (std::is_pointer_v<XrSpaceT>)
        return reinterpret_cast<XrSpaceT>(space);
    else
        return static_cast<XrSpaceT>(space);
}

template<typename XrSpaceT>
XrSpaceId toXrSpaceId(XrSpaceT space)
{
    static_assert(sizeof(XrSpaceT) <= sizeof(XrSpaceId), "XrSpaceT's size must equal or smaller than XrSpaceId");
    return static_cast<XrSpaceId>(quint64(space));
}

} // namespace QtQuick3DXr

QT_END_NAMESPACE

#endif // QTQUICK3DXRGLOBAL_P_H
