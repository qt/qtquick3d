// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick3DTextureHelpers/private/qssgexrreader_p.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>

// At fuzzing rates the warnings alone would dominate the run time.
static QtMessageHandler mh = qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &) { });

extern "C" int LLVMFuzzerTestOneInput(const char *Data, size_t Size)
{
    QSSGExrReader reader;
    if (!reader.parseHeader(QByteArray::fromRawData(Data, qsizetype(Size))))
        return 0;

    const QSize size = reader.size();
    if (qint64(size.width()) * size.height() > (1 << 20))
        return 0;

    QList<float> pixels(qsizetype(size.width()) * size.height() * 4, 0.0f);
    reader.decodeToRgba32F(qssgExrRgbaChannelMap(reader), QSSGExrReader::FlipVertically, pixels.data(), pixels.size());
    return 0;
}
