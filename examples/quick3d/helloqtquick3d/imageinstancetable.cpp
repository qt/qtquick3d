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

#include "imageinstancetable.h"
#include <math.h>
#include <QMatrix4x4>
#include <QColor>
#include <QImage>
#include <QQmlFile>

ImageInstanceTable::ImageInstanceTable(QQuick3DObject *parent) : QQuick3DInstancing(parent)
{
}

ImageInstanceTable::~ImageInstanceTable()
{
}

int ImageInstanceTable::gridSize() const
{
    return m_gridSize;
}

float ImageInstanceTable::gridSpacing() const
{
    return m_gridSpacing;
}

QString ImageInstanceTable::image() const
{
    return m_imageSource;
}

void ImageInstanceTable::setGridSize(int gridSize)
{
    if (m_gridSize == gridSize)
        return;

    m_gridSize = gridSize;
    emit gridSizeChanged(m_gridSize);
    markDirty();
    m_dirty = true;
}

void ImageInstanceTable::setGridSpacing(float gridSpacing)
{
    if (qFuzzyCompare(m_gridSpacing, gridSpacing))
        return;

    m_gridSpacing = gridSpacing;
    emit gridSpacingChanged(m_gridSpacing);
    markDirty();
    m_dirty = true;
}

void ImageInstanceTable::setImage(QString image)
{
    if (m_imageSource == image)
        return;

    m_imageSource = image;
    emit imageChanged(m_imageSource);
    markDirty();
    m_dirty = true;
}

QByteArray ImageInstanceTable::getInstanceBuffer(int *instanceCount)
{
    if (m_dirty) {
        QImage image(m_imageSource);
        if (image.isNull())
            qWarning() << "Could not load image" << m_imageSource;
        m_instanceData.resize(0);
        image = image.scaledToWidth(m_gridSize, Qt::SmoothTransformation);
        int instanceNumber = 0;
        for (int blockX = 0; blockX < image.width(); ++blockX) {
            float xPos = m_gridSpacing * (blockX - m_gridSize/2);
            for (int blockY = 0; blockY < image.width(); ++blockY) {
                QColor color = image.pixel(blockX, blockY);
                float yPos = -m_gridSpacing * (blockY - m_gridSize/2);

                float zPos = 10 * m_gridSpacing * (color.lightnessF() - 1.0);

                auto entry = calculateTableEntry({xPos,yPos,zPos}, {1.0, 1.0, 1.0}, {0,0,0}, color, {});
                m_instanceData.append(reinterpret_cast<const char *>(&entry), sizeof(entry));
                instanceNumber++;
            }
        }
        m_instanceCount = instanceNumber;
        m_dirty = false;
    }
    if (instanceCount)
        *instanceCount = m_instanceCount;

    return m_instanceData;
}
