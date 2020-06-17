:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2020 The Qt Company Ltd.
:: Contact: https://www.qt.io/licensing/
::
:: This file is part of the QtQuick3D module of the Qt Toolkit.
::
:: $QT_BEGIN_LICENSE:LGPL$
:: Commercial License Usage
:: Licensees holding valid commercial Qt licenses may use this file in
:: accordance with the commercial license agreement provided with the
:: Software or, alternatively, in accordance with the terms contained in
:: a written agreement between you and The Qt Company. For licensing terms
:: and conditions see https://www.qt.io/terms-conditions. For further
:: information use the contact form at https://www.qt.io/contact-us.
::
:: GNU Lesser General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU Lesser
:: General Public License version 3 as published by the Free Software
:: Foundation and appearing in the file LICENSE.LGPL3 included in the
:: packaging of this file. Please review the following information to
:: ensure the GNU Lesser General Public License version 3 requirements
:: will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
::
:: GNU General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU
:: General Public License version 2.0 or (at your option) the GNU General
:: Public license version 3 or any later version approved by the KDE Free
:: Qt Foundation. The licenses are as published by the Free Software
:: Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
:: included in the packaging of this file. Please review the following
:: information to ensure the GNU General Public License requirements will
:: be met: https://www.gnu.org/licenses/gpl-2.0.html and
:: https://www.gnu.org/licenses/gpl-3.0.html.
::
:: $QT_END_LICENSE$
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: For HLSL we invoke fxc.exe (-c argument) and store the resulting intermediate format
:: instead of HLSL source, so this needs to be run on Windows from a developer command prompt.

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o cubeshadowdepth.vert.qsb cubeshadowdepth.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o cubeshadowdepth.frag.qsb cubeshadowdepth.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o orthoshadowdepth.vert.qsb orthoshadowdepth.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o orthoshadowdepth.frag.qsb orthoshadowdepth.frag

qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -c -o cubeshadowblurx.vert.qsb cubeshadowblurx.vert
qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -c -o cubeshadowblurx.frag.qsb cubeshadowblurx.frag

qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -c -o cubeshadowblury.vert.qsb cubeshadowblury.vert
qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -c -o cubeshadowblury.frag.qsb cubeshadowblury.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o orthoshadowblurx.vert.qsb orthoshadowblurx.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o orthoshadowblurx.frag.qsb orthoshadowblurx.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o orthoshadowblury.vert.qsb orthoshadowblury.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o orthoshadowblury.frag.qsb orthoshadowblury.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o depthprepass.vert.qsb depthprepass.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o depthprepass.frag.qsb depthprepass.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 -c -o ssao.vert.qsb ssao.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 -c -o ssao.frag.qsb ssao.frag

qsb --glsl "430,310 es" --hlsl 50 --msl 12 -c -o miprgbe8.comp.qsb miprgbe8.comp

qsb --glsl "300 es,150" --hlsl 50 --msl 12 -c -o skybox.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 -c -o skybox.frag.qsb skybox.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o ssaaresolve.vert.qsb ssaaresolve.vert
qsb -p --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o ssaaresolve.frag.qsb ssaaresolve.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o progressiveaa.vert.qsb progressiveaa.vert
qsb -p --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o progressiveaa.frag.qsb progressiveaa.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o texturedquad.vert.qsb texturedquad.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -c -o texturedquad.frag.qsb texturedquad.frag
