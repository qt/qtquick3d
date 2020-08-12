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

uniform sampler2D BlurBuffer;

void main()
{
    vec2 texSize = vec2( textureSize( BlurBuffer, 0 ) );
    texSize = vec2(1.0) / texSize;
    texCoord0.z = 0.0;
    texCoord0.xy = vec2(gl_FragCoord.xy * texSize);

    float sigma = clamp(blur_size * 0.5, 0.5, 100.0);
    int smpCount = int(ceil( sigma ));
    vec4 value = texture(BlurBuffer, texCoord0.xy);
    float wtsum = 1.0;
    for (int i = 1; i <= smpCount; ++i)
    {
        // Base 2 Gaussian blur
        float wt = float(i) / (sigma * 0.5);
        wt = exp2( -wt*wt );
        vec2 texOfs = vec2(i, 0) * texSize;
        value += wt * texture(BlurBuffer, texCoord0.xy+texOfs);
        value += wt * texture(BlurBuffer, texCoord0.xy-texOfs);
        wtsum += wt * 2.0;
    }

    fragColor = value / wtsum;
    fragColor.a = 1.0;
}
