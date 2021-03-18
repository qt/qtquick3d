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

#include "qssgperftimer_p.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QHash>
#include <QtCore/QVector>

#include <private/qssgutils_p.h>

QT_BEGIN_NAMESPACE

static uint qHash(const QSSGPerfTimer::Key &key)
{
    const uchar *s = reinterpret_cast<const uchar *>(key.id);
    uint h = 0;
    while (*s) {
        h = 31 * h + *s;
        ++s;
    }
    return h;
}
static bool operator==(const QSSGPerfTimer::Key &a, const QSSGPerfTimer::Key &b) { return !strcmp(a.id, b.id); }

static bool operator<(const QSSGPerfTimer::Entry &a, const QSSGPerfTimer::Entry &b) { return a.tag < b.tag; }

void QSSGPerfTimer::Entry::update(qint64 elapsed)
{
    totalTime += elapsed;
    maxTime = qMax(maxTime, elapsed);
    ++count;
}

void QSSGPerfTimer::Entry::reset()
{
    totalTime = 0;
    maxTime = 0;
    count = 0;
}

QString QSSGPerfTimer::Entry::toString(quint32 inFramesPassed) const
{
    if (!count)
        return QString();


    const double milliseconds = totalTime / 1000000.0;
    const double maxMilliseconds = maxTime / 1000000.0;
    if (inFramesPassed == 0)
        return QString::fromLatin1("%1 - %2ms").arg(tag).arg(milliseconds);

    return QString::fromLatin1("%1 - %2ms/frame; %3ms max; %4 hits").arg(tag).arg(milliseconds/inFramesPassed).arg(maxMilliseconds).arg(count);
}


QSSGPerfTimer::QSSGPerfTimer() = default;

QSSGPerfTimer::~QSSGPerfTimer() = default;

void QSSGPerfTimer::update(const char *inId, qint64 elapsed)
{
    QMutexLocker locker(&mutex);
    auto it = entries.find(Key{inId});
    if (it == entries.end())
        it = entries.insert(Key{inId}, Entry(QString::fromUtf8(inId)));
    it.value().update(elapsed);
}

void QSSGPerfTimer::dump()
{
    QMutexLocker locker(&mutex);
    QVector<QSSGPerfTimer::Entry> allEntries;
    for (auto iter = entries.begin(), end = entries.end(); iter != end; ++iter) {
        allEntries.push_back(iter.value());
        iter.value().reset();
    }

    std::sort(allEntries.begin(), allEntries.end());

    qDebug() << "performance data:";
    for (const auto &e: qAsConst(allEntries))
        qDebug() << "    " << e.toString(frameCount).toUtf8().constData();
    qDebug() << "";

    frameCount = 0;
}

void QSSGPerfTimer::reset()
{
    QMutexLocker locker(&mutex);
    auto iter = entries.begin();
    const auto end = entries.end();
    while (iter != end) {
        iter.value().reset();
        ++iter;
    }

    frameCount = 0;
}

QT_END_NAMESPACE
