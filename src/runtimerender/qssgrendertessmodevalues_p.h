/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSG_RENDER_TESS_MODE_VALUES_H
#define QSSG_RENDER_TESS_MODE_VALUES_H

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

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

enum class TessellationModeValues : quint8
{
    NoTessellation = 0,
    Linear = 1,
    Phong = 2,
    NPatch = 3,
};

inline uint qHash(TessellationModeValues v) { return qHash(static_cast<uint>(v)); }

inline const char *toString(TessellationModeValues value)
{
    switch (value) {
    case TessellationModeValues::Linear:
        return "Linear";
    case TessellationModeValues::Phong:
        return "Phong";
    case TessellationModeValues::NPatch:
        return "NPatch";
    default:
        return "NoTessellation";
    }
}

QT_END_NAMESPACE

#endif
