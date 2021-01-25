/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "custominstancing.h"
#include <math.h>
#include <QMatrix4x4>

struct InstanceTableEntry {
    QVector4D row0;
    QVector4D row1;
    QVector4D row2;
    QVector4D color;
    QVector4D instanceData;
};

void CustomInstancing::setInstanceCount(int instanceCount)
{
    if (m_instanceCount == instanceCount)
        return;

    m_instanceCount = instanceCount;
    emit instanceCountChanged(m_instanceCount);
}

QByteArray CustomInstancing::getInstanceBuffer(int *instanceCount)
{
    QByteArray instanceData;
    if (instanceCount)
        *instanceCount = m_instanceCount;
    if (m_instanceCount == 0)
        return instanceData;

    instanceData.resize(m_instanceCount * 20 * sizeof(float));
    auto array = reinterpret_cast<InstanceTableEntry*>(instanceData.data());

    const int rows = floor(sqrt(m_instanceCount));
    const int columns = m_instanceCount/rows;

    for (int i = 0; i < m_instanceCount; ++i) {
        QMatrix4x4 xform;

        int ix = i % columns;
        int iy = i / columns;
        float x = 100.0 * (ix - columns/2);
        float y = 100.0 * (iy - rows/2);
        float z = (x*x + y*y)/1000;

        xform(0, 3) += x;
        xform(1, 3) += y;
        xform(2, 3) += z;

        float scale = 0.75;
        xform(0, 0) = scale;
        xform(1, 1) = scale;
        xform(2, 2) = scale;

        QVector4D color(float(ix)/columns, float(iy)/columns, 1.0, float(i) / m_instanceCount);

        array[i] = {
            xform.row(0),
            xform.row(1),
            xform.row(2),
            color,
            {}
        };
    }

    return instanceData;
}
