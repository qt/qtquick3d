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

:: For SPIR-V the optimizer is requested (-O argument) which means spirv-opt must be
:: invokable (e.g. because it's in the PATH from the Vulkan SDK)

qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -O -c -o cubeshadowblurx.vert.qsb cubeshadowblurx.vert
qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -O -c -o cubeshadowblurx.frag.qsb cubeshadowblurx.frag

qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -O -c -o cubeshadowblury.vert.qsb cubeshadowblury.vert
qsb --glsl "300 es,120,150" --hlsl 50 --msl 12 -O -c -o cubeshadowblury.frag.qsb cubeshadowblury.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o orthoshadowblurx.vert.qsb orthoshadowblurx.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o orthoshadowblurx.frag.qsb orthoshadowblurx.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o orthoshadowblury.vert.qsb orthoshadowblury.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o orthoshadowblury.frag.qsb orthoshadowblury.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o ssao.vert.qsb ssao.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o ssao.frag.qsb ssao.frag

:: Skybox shader has several variants for both RGBE vs RGBA and tonemapping modes
:: No tonemapping (skipped)
qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o skybox_hdr_none.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o skybox_hdr_none.frag.qsb skybox.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_none.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_none.frag.qsb skybox.frag
:: Linear Tonemapping
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_LINEAR_TONEMAPPING -O -c -o skybox_hdr_linear.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_LINEAR_TONEMAPPING -O -c -o skybox_hdr_linear.frag.qsb skybox.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_LINEAR_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_linear.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_LINEAR_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_linear.frag.qsb skybox.frag
:: ACES Tonemapping
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_ACES_TONEMAPPING -O -c -o skybox_hdr_aces.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_ACES_TONEMAPPING -O -c -o skybox_hdr_aces.frag.qsb skybox.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_ACES_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_aces.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_ACES_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_aces.frag.qsb skybox.frag
:: Hejl Dawson Tonemapping
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_HEJLDAWSON_TONEMAPPING -O -c -o skybox_hdr_hejldawson.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_HEJLDAWSON_TONEMAPPING -O -c -o skybox_hdr_hejldawson.frag.qsb skybox.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_HEJLDAWSON_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_hejldawson.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_HEJLDAWSON_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_hejldawson.frag.qsb skybox.frag
:: Filmic Tonemapping
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_FILMIC_TONEMAPPING -O -c -o skybox_hdr_filmic.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_FILMIC_TONEMAPPING -O -c -o skybox_hdr_filmic.frag.qsb skybox.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_FILMIC_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_filmic.vert.qsb skybox.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_FILMIC_TONEMAPPING --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o skybox_rgbe_filmic.frag.qsb skybox.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o ssaaresolve.vert.qsb ssaaresolve.vert
qsb -p --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o ssaaresolve.frag.qsb ssaaresolve.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o progressiveaa.vert.qsb progressiveaa.vert
qsb -p --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o progressiveaa.frag.qsb progressiveaa.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o texturedquad.vert.qsb texturedquad.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o texturedquad.frag.qsb texturedquad.frag

qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o environmentmap.vert.qsb environmentmap.vert
qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -O -c -o environmentmap.frag.qsb environmentmap.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o environmentmapprefilter.vert.qsb environmentmapprefilter.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o environmentmapprefilter.frag.qsb environmentmapprefilter.frag
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o environmentmapprefilter_rgbe.vert.qsb environmentmapprefilter.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o environmentmapprefilter_rgbe.frag.qsb environmentmapprefilter.frag

qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o environmentmapirradiance.vert.qsb environmentmapirradiance.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 -O -c -o environmentmapirradiance.frag.qsb environmentmapirradiance.frag
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o environmentmapirradiance_rgbe.vert.qsb environmentmapirradiance.vert
qsb --glsl "300 es,150" --hlsl 50 --msl 12 --define QSSG_ENABLE_RGBE_LIGHT_PROBE -O -c -o environmentmapirradiance_rgbe.frag.qsb environmentmapirradiance.frag
