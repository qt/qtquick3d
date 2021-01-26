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

#ifndef QSSG_RENDER_MATERIAL_DIRTY_H
#define QSSG_RENDER_MATERIAL_DIRTY_H

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

QT_BEGIN_NAMESPACE

class QSSGMaterialDirty
{
private:
    enum Flag : quint8
    {
        Dirty = 1 << 0,
        DirtyWithinFrame = 1 << 1,
        AllDirty = Dirty | DirtyWithinFrame
    };
    Flag m_dirtyFlag = AllDirty;

public:
    QSSGMaterialDirty() = default;

    void setDirty() { m_dirtyFlag = AllDirty; }
    bool isDirty() const { return (m_dirtyFlag != 0); }
    void clearDirty() { m_dirtyFlag = Flag(0); }
    void updateDirtyForFrame()
    {
        m_dirtyFlag = DirtyWithinFrame;
        clearDirty();
    }
};

QT_END_NAMESPACE

#endif // QSSG_RENDER_MATERIAL_DIRTY_H
