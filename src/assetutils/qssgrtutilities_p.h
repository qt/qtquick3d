/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QSSG_RUNTIME_UTILITIES_H
#define QSSG_RUNTIME_UTILITIES_H

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

#include <QtQuick3DAssetUtils/private/qtquick3dassetutilsglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuick3DNode;
namespace QSSGSceneDesc { struct Scene; }

namespace QSSGRuntimeUtils
{

Q_QUICK3DASSETUTILS_EXPORT QQuick3DNode *createScene(QQuick3DNode &parent, const QSSGSceneDesc::Scene &scene);

}

QT_END_NAMESPACE

#endif // QSSG_RUNTIME_UTILITIES_H
