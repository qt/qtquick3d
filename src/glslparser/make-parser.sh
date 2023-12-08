#!/bin/sh
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial

me=$(dirname $0)
qlalr --qt --no-debug $me/glsl.g

