/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qssgrenderpath_p.h"
#include "qssgrendersubpath_p.h"

QT_BEGIN_NAMESPACE

void QSSGRenderPath::addSubPath(QSSGRenderSubPath &inSegment)
{
    QSSGRenderSubPath *lastSegment = nullptr;
    inSegment.m_path = this;
    inSegment.m_nextSubPath = nullptr;
    if (m_firstSubPath) {
        // find last segment
        for (lastSegment = m_firstSubPath; lastSegment && lastSegment->m_nextSubPath; lastSegment = lastSegment->m_nextSubPath)
            ;
        lastSegment->m_nextSubPath = &inSegment;
    } else
        m_firstSubPath = &inSegment;
}

void QSSGRenderPath::clearSubPaths()
{
    QSSGRenderSubPath *nextSegment = nullptr;
    for (QSSGRenderSubPath *theSegment = m_firstSubPath; theSegment; theSegment = nextSegment) {
        nextSegment = theSegment->m_nextSubPath;
        theSegment->m_path = nullptr;
        theSegment->m_nextSubPath = nullptr;
    }
    m_firstSubPath = nullptr;
}

QT_END_NAMESPACE
