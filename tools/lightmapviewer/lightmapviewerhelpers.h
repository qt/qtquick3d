// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef LIGHTMAPVIEWERHELPERS_H
#define LIGHTMAPVIEWERHELPERS_H

#include <QByteArray>
#include <QString>

struct LightmapViewerHelpers
{
    static void maskToBBGRColor(QByteArray &array, bool useAlpha = true);
    static bool processLightmap(const QString &filename, bool print, bool extract);
};

#endif // LIGHTMAPVIEWERHELPERS_H
