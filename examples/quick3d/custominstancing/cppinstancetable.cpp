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

#include "cppinstancetable.h"
#include <math.h>
#include <QMatrix4x4>
#include <QRandomGenerator>
#include <QColor>

// Quick-and-dirty smoothed out noise generation. Probably not suitable for general use.
static QVector<float> generateNoiseTable(int dimension, int randomSeed)
{
    const int tableSize = dimension * dimension;
    QVector<float> table(tableSize);
    QRandomGenerator rgen(randomSeed);

    for (float &f: table)
        f = rgen.bounded(1.0) * rgen.bounded(1.0);

    // We select some initial points that will not be modified. This is the distance between them: (power of two)
    constexpr int delta = 16;

    // Then we average out those points to the points half way between them,
    // and continue with the points half way between those, and so on.
    // Pattern:
    // STS
    // TTT
    // STS
    // where S = source and T = target
    auto smooth = [dimension, &table](int x, int y, int d) {
        auto lookup = [&table,dimension](int x, int y) -> float {
            return table[x + y*dimension];
        };
        auto assign = [&table,dimension,d](int x, int y, float v) {
            if (x < dimension && y < dimension) {
                float e = d*1.0/dimension;
                float &z = table[x + y*dimension];
                z = (e*z + v)/(e+1);
            }
        };

        int x1 = x + d/2;
        int y1 = y + d/2;
        int x2 = qMin(dimension-1, x + d);
        int y2 = qMin(dimension-1, y + d);
        float z1 = lookup(x,y);
        float z2 = lookup(x2, y);
        float z3 = lookup(x, y2);
        float z4 = lookup(x2, y2);
        assign(x1, y, (z1+z2)/2);
        assign(x, y1, (z1+z3)/2);
        assign(x1, y1, (z1+z2+z3+z4)/4);
        assign(x1, y2, (z3+z4)/2);
        assign(x2, y1, (z2+z4)/2);
    };

    int d = delta;
    while (d > 1) {
        for (int ix = 0; ix < dimension; ix += d) {
            for (int iy = 0; iy < dimension; iy += d) {
                smooth(ix, iy, d);
            }
        }
        d = d/2;
    }

    //low-pass filter
    for (int i = dimension + 1; i < tableSize; ++i)
        table[i] = (table[i] + table[i-1] + table[i-dimension])/3;

    //normalize
    float min = 1.0;
    float max = 0.0;
    for (auto z : table) {
        min = qMin(z, min);
        max = qMax(z, max);
    }
    for (auto &z : table)
        z = (z - min) / (max - min);

    return table;
}

CppInstanceTable::CppInstanceTable(QQuick3DObject *parent) : QQuick3DInstancing(parent)
{
    m_generator = new QRandomGenerator;
    m_randomSeed = QRandomGenerator::global()->generate();
}

CppInstanceTable::~CppInstanceTable()
{
    delete m_generator;
}

int CppInstanceTable::gridSize() const
{
    return m_gridSize;
}

float CppInstanceTable::gridSpacing() const
{
    return m_gridSpacing;
}

int CppInstanceTable::randomSeed() const
{
    return m_randomSeed;
}

void CppInstanceTable::setGridSize(int gridSize)
{
    if (m_gridSize == gridSize)
        return;

    m_gridSize = gridSize;
    emit gridSizeChanged(m_gridSize);
    markDirty();
    m_dirty = true;
}

void CppInstanceTable::setGridSpacing(float gridSpacing)
{
    if (qFuzzyCompare(m_gridSpacing, gridSpacing))
        return;

    m_gridSpacing = gridSpacing;
    emit gridSpacingChanged(m_gridSpacing);
    markDirty();
    m_dirty = true;
}

void CppInstanceTable::setRandomSeed(int randomSeed)
{
    if (m_randomSeed == randomSeed)
        return;

    m_randomSeed = randomSeed;
    emit randomSeedChanged(m_randomSeed);
    markDirty();
    m_dirty = true;
}

QByteArray CppInstanceTable::getInstanceBuffer(int *instanceCount)
{
    if (m_dirty) {
        m_generator->seed(m_randomSeed);
        const int maxHeight = m_gridSize/2;

        auto noiseMap = generateNoiseTable(m_gridSize, m_randomSeed);
        int seaLevel = maxHeight / 4;

        m_instanceData.resize(0);

        auto getHeight = [&noiseMap, maxHeight, this](int x, int y) -> int { return maxHeight * noiseMap[x + y*m_gridSize]; };
        int instanceNumber = 0;
        for (int blockX = 0; blockX < m_gridSize; ++blockX) {
            float xPos = m_gridSpacing * (blockX - m_gridSize/2);
            for (int blockY = 0; blockY < m_gridSize; ++blockY) {
                float zPos = m_gridSpacing * (blockY - m_gridSize/2);
                int terrainHeight = getHeight(blockX, blockY);
                int snowLine = maxHeight*4/5 - QRandomGenerator::global()->bounded(maxHeight/5);
                int treeLine = maxHeight*3/5 - QRandomGenerator::global()->bounded(maxHeight/5);

                // optimization: skip blocks that are obscured by neighbours
                int lowestVisible;
                if (blockX == 0 || blockY == 0 || blockX == m_gridSize - 1 || blockY == m_gridSize - 1) {
                    lowestVisible = 0;
                } else if (terrainHeight <= seaLevel) {
                    lowestVisible = seaLevel;
                } else {
                    lowestVisible = qMin(terrainHeight, getHeight(blockX-1, blockY));
                    lowestVisible = qMin(lowestVisible, getHeight(blockX, blockY-1));
                    lowestVisible = qMin(lowestVisible, getHeight(blockX+1, blockY));
                    lowestVisible = qMin(lowestVisible, getHeight(blockX, blockY+1));
                    lowestVisible = qMax(lowestVisible, seaLevel);
                }

                for (int blockZ = lowestVisible; blockZ <= qMax(terrainHeight, seaLevel); ++blockZ) {
                    float yPos = m_gridSpacing * (blockZ - m_gridSize/2);

                    float waterAnimation = 0.0;
                    QColor color;
                    if (blockZ > terrainHeight) {
                        color = Qt::blue;
                        if (blockZ == seaLevel)
                            waterAnimation = 1.0;
                    } else if (blockZ > snowLine) {
                        color = Qt::white;
                    } else if ( blockZ > treeLine) {
                        color = Qt::darkGray;
                    } else {
                        color = QColor::fromHsvF(blockZ*0.7/maxHeight, 0.7, 0.5, 1.0);
                    }
                    auto entry = calculateTableEntry({xPos,yPos,zPos}, {1.0, 1.0, 1.0}, {}, color, {waterAnimation,0,0,0});
                    m_instanceData.append(reinterpret_cast<const char *>(&entry), sizeof(entry));
                    instanceNumber++;
                }
            }
        }
        m_instanceCount = instanceNumber;
        m_dirty = false;
    }
    if (instanceCount)
        *instanceCount = m_instanceCount;

    return m_instanceData;
}
