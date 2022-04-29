/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "converterthread.h"

#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>

#include <QFuture>
#include <QtConcurrent/QtConcurrent>

ConverterThread::ConverterThread(QObject *parent) : QThread(parent) { }

ConverterThread::~ConverterThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();
    wait();
}

void ConverterThread::convert(QStringList filenames, QDir outputPath, QVariantMap options)
{
    QMutexLocker locker(&mutex);

    // Write settings
    m_filenames = filenames;
    m_outputPath = outputPath;
    m_options = options;

    if (!isRunning()) {
        start(LowPriority);
    } else {
        condition.wakeOne();
    }
}

void ConverterThread::run()
{
    forever {
        if (abort)
            return;

        // Copy settings
        mutex.lock();
        auto filenames = m_filenames;
        auto outputPath = m_outputPath;
        auto options = m_options;
        mutex.unlock();

        emit convertStart(QString("Converting %1 files...").arg(filenames.size()));

        std::atomic<int> failCounter = 0;
        std::atomic<int> fileCounter = 0;
        const int numFiles = filenames.size();

        auto convertFile = [&](const QString &filename) {
            if (abort)
                return;
            QString error;
            QSSGAssetImportManager assetImporter;
            assetImporter.importFile(filename, outputPath, options, &error);
            const size_t ctr = ++fileCounter;
            failCounter += error.isEmpty() ? 0 : 1;

            if (!error.isEmpty())
                emit convertUpdate(
                        QString("[%1/%2] Failed to convert '%3': %4").arg(QString::number(ctr), QString::number(numFiles), filename, error));
            else
                emit convertUpdate(
                        QString("[%1/%2] Successfully converted '%3'").arg(QString::number(ctr), QString::number(numFiles), filename));
        };

        // Convert in parallel
        QtConcurrent::blockingMap(filenames, convertFile);

        if (failCounter > 0)
            emit convertDone(QString("\nConversion done, failed to convert %1 of %2 files")
                                     .arg(QString::number(failCounter), QString::number(numFiles)));
        else
            emit convertDone(QString("\nSuccessfully converted all files!"));

        // Abort or wait for signal
        mutex.lock();
        if (!abort)
            condition.wait(&mutex);
        mutex.unlock();
    }
}
