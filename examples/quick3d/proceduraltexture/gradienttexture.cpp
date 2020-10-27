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

#include "gradienttexture.h"

#include <QtCore/QSize>
#include <QtGui/QGradient>

GradientTexture::GradientTexture()
{
    updateTexture();
}

int GradientTexture::height() const
{
    return m_height;
}

int GradientTexture::width() const
{
    return m_width;
}

QColor GradientTexture::startColor() const
{
    return m_startColor;
}

QColor GradientTexture::endColor() const
{
    return m_endColor;
}

void GradientTexture::setHeight(int height)
{
    if (m_height == height)
        return;

    m_height = height;
    emit heightChanged(m_height);
    updateTexture();
}

void GradientTexture::setWidth(int width)
{
    if (m_width == width)
        return;

    m_width = width;
    emit widthChanged(m_width);
    updateTexture();
}

//! [property]
void GradientTexture::setStartColor(QColor startColor)
{
    if (m_startColor == startColor)
        return;

    m_startColor = startColor;
    emit startColorChanged(m_startColor);
    updateTexture();
}
//! [property]

void GradientTexture::setEndColor(QColor endColor)
{
    if (m_endColor == endColor)
        return;

    m_endColor = endColor;
    emit endColorChanged(m_endColor);
    updateTexture();
}

//! [updateTexture]
void GradientTexture::updateTexture()
{
    setSize(QSize(m_width, m_height));
    setFormat(QQuick3DTextureData::RGBA8);
    setHasTransparency(false);
    setTextureData(generateTexture());
}
//! [updateTexture]

//! [generateTexture]
QByteArray GradientTexture::generateTexture()
{
    QByteArray imageData;
    // Create a horizontal gradient between startColor and endColor

    // Create a single scanline and reuse that data for each
    QByteArray gradientScanline;
    gradientScanline.resize(m_width * 4); // RGBA8

    for (int x = 0; x < m_width; ++x) {
        QColor color = linearInterpolate(m_startColor, m_endColor, x / float(m_width));
        int offset = x * 4;
        gradientScanline.data()[offset + 0] = char(color.red());
        gradientScanline.data()[offset + 1] = char(color.green());
        gradientScanline.data()[offset + 2] = char(color.blue());
        gradientScanline.data()[offset + 3] = char(255);
    }

    for (int y = 0; y < m_height; ++y)
        imageData += gradientScanline;

    return imageData;
}
//! [generateTexture]

QColor GradientTexture::linearInterpolate(const QColor &color1, const QColor &color2, float value)
{
    QColor output;

    output.setRedF(color1.redF() + (value * (color2.redF() - color1.redF())));
    output.setGreenF(color1.greenF() + (value * (color2.greenF() - color1.greenF())));
    output.setBlueF(color1.blueF() + (value * (color2.blueF() - color1.blueF())));

    return output;
}
