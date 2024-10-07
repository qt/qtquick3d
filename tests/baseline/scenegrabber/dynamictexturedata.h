/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef DYNAMICTEXTUREDATA_H
#define DYNAMICTEXTUREDATA_H

#include <QtQuick3D/QQuick3DTextureData>

class DynamicTextureData : public QQuick3DTextureData
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DynamicTextureData)
public:
    DynamicTextureData();

public Q_SLOTS:
    void generateRGBA8Texture();
    void generateRGBE8Texture();
    void generateRGBA32FTexture();

private:
    struct LinearColor {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;
        LinearColor(const QColor &color);
        LinearColor() {}

        LinearColor interpolate(const LinearColor &color, float value) const;
        LinearColor blend(const LinearColor &color) const;
        quint32 toRGBE8() const;
    };
    static void generateHDRTexture(int width, int height, QByteArray &imageData, bool isRGBE);
};

#endif // DYNAMICTEXTUREDATA_H
