// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QImage>
#include <QQuickImageProvider>

class LightmapImageProvider : public QQuickImageProvider
{
public:
    LightmapImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QImage m_errorImage;
};
