// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquick3dprofiler_p.h"

#include <QtQml/private/qqmlabstractprofileradapter_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

// instance will be set, unset in constructor. Allows static methods to be inlined.
QQuick3DProfiler *QQuick3DProfiler::s_instance = nullptr;
quint64 QQuick3DProfiler::featuresEnabled = 0;

void QQuick3DProfiler::initialize(QObject *parent)
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = new QQuick3DProfiler(parent);
}

QQuick3DProfiler::QQuick3DProfiler(QObject *parent)
    : QObject(parent)
{
    m_timer.start();
}

QQuick3DProfiler::~QQuick3DProfiler()
{
    QMutexLocker lock(&m_dataMutex);
    featuresEnabled = 0;
    s_instance = nullptr;
}

void QQuick3DProfiler::startProfilingImpl(quint64 features)
{
    QMutexLocker lock(&m_dataMutex);
    featuresEnabled = features;
}

void QQuick3DProfiler::stopProfilingImpl()
{
    QMutexLocker lock(&m_dataMutex);
    featuresEnabled = 0;
    emit dataReady(m_data);
    m_data.clear();
}

void QQuick3DProfiler::reportDataImpl()
{
    QMutexLocker lock(&m_dataMutex);
    emit dataReady(m_data);
    m_data.clear();
}

void QQuick3DProfiler::setTimer(const QElapsedTimer &t)
{
    QMutexLocker lock(&m_dataMutex);
    m_timer = t;
}

QT_END_NAMESPACE
