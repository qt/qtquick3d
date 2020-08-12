/****************************************************************************
**
** Copyright (C) 2014 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
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

#define QSSG_ENABLE_UV0 1
#define QSSG_ENABLE_WORLD_POSITION 1
#define QSSG_ENABLE_TEXTAN 1

vec3 texCoord0;
out vec4 fragColor;

uniform sampler2D OriginBuffer;

void main()
{
    vec2 texSize = vec2( textureSize( OriginBuffer, 0 ) );
    texSize = vec2(1.0) / texSize;
    texCoord0.z = 0.0;
    texCoord0.xy = vec2(gl_FragCoord.xy * 2.0 * texSize);

    float wtSum = 0.0;
    vec4 totSum = vec4(0.0);
    for (int ix = -1; ix <= 1; ++ix)
    {
       for (int iy = -1; iy <= 1; ++iy)
       {
        float wt = float(ix*ix + iy*iy) * 4.0;
        wt = exp2( -wt );
        vec2 texOfs = vec2(ix, iy) * texSize;
        totSum += wt * texture( OriginBuffer, texCoord0.xy + texOfs );
        wtSum += wt;
       }
    }

    totSum /= wtSum;
    fragColor = totSum;
}
