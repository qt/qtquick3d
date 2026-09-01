// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DTextureHelpers/private/qssgexrcodecs_p.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>

static QtMessageHandler mh = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &) { });

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    constexpr qint64 nx = 64;
    constexpr qint64 ny = 32;

    QList<QSSGExr::PizChannel> channels;
    channels.append({ 1, 0 }); // half
    channels.append({ 1, 2 }); // half
    channels.append({ 2, 4 }); // float, two 16-bit planes
    const qint64 bytesPerPixel = 8;

    QByteArray out(nx * ny * bytesPerPixel, '\0');
    QSSGExr::PizScratch scratch;
    QString error;
    QSSGExr::pizDecode(QByteArray::fromRawData(Data, qsizetype(Size)), QSpan<char>(out.data(), out.size()), channels, nx, ny, bytesPerPixel, &scratch, &error);
    return 0;
}
