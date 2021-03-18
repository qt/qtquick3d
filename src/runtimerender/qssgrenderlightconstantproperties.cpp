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

#include "qssgrenderlightconstantproperties_p.h"

static const char *lconstantnames[] = { "position",
                                        "direction",
                                        "up",
                                        "right",
                                        "diffuse",
                                        "ambient",
                                        "specular",
                                        "coneAngle",
                                        "innerConeAngle",
                                        "constantAttenuation",
                                        "linearAttenuation",
                                        "quadraticAttenuation",
                                        "range",
                                        "width",
                                        "height",
                                        "shadowControls",
                                        "shadowView",
                                        "shadowIdx",
                                        "attenuation" };

QSSGLightConstants::QSSGLightConstants(const QByteArray &lightRef, const QSSGRef<QSSGRenderShaderProgram> &shader)
    : position(lightRef + lconstantnames[0], shader)
    , direction(lightRef + lconstantnames[1], shader)
    , up(lightRef + lconstantnames[2], shader)
    , right(lightRef + lconstantnames[3], shader)
    , diffuse(lightRef + lconstantnames[4], shader)
    , ambient(lightRef + lconstantnames[5], shader)
    , specular(lightRef + lconstantnames[6], shader)
    , coneAngle(lightRef + lconstantnames[7], shader)
    , innerConeAngle(lightRef + lconstantnames[8], shader)
    , constantAttenuation(lightRef + lconstantnames[9], shader)
    , linearAttenuation(lightRef + lconstantnames[10], shader)
    , quadraticAttenuation(lightRef + lconstantnames[11], shader)
    , range(lightRef + lconstantnames[12], shader)
    , width(lightRef + lconstantnames[13], shader)
    , height(lightRef + lconstantnames[14], shader)
    , shadowControls(lightRef + lconstantnames[15], shader)
    , shadowView(lightRef + lconstantnames[16], shader)
    , shadowIdx(lightRef + lconstantnames[17], shader)
    , attenuation(lightRef + lconstantnames[18], shader)
{
}
