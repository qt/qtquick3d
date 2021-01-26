/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSSG_RENDER_THREAD_POOL_H
#define QSSG_RENDER_THREAD_POOL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick3DRuntimeRender/private/qtquick3druntimerenderglobal_p.h>

#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE

using QSSGTaskCallback = void (*)(void *);

enum class TaskStates
{
    UnknownTask = 0,
    Queued,
    Running,
};

enum class CancelReturnValues
{
    TaskCanceled = 0,
    TaskRunning,
    TaskNotFound,
};

class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGAbstractThreadPool
{
public:
    QAtomicInt ref;
    virtual ~QSSGAbstractThreadPool();
    // Add a task to be run at some point in the future.
    // Tasks will be run roughly in order they are given.
    // The returned value is a handle that can be used to query
    // details about the task
    // Cancel function will be called if the thread pool is destroyed or
    // of the task gets canceled.
    virtual quint64 addTask(void *inUserData, QSSGTaskCallback inFunction, QSSGTaskCallback inCancelFunction) = 0;
    virtual TaskStates getTaskState(quint64 inTaskId) = 0;
    virtual CancelReturnValues cancelTask(quint64 inTaskId) = 0;

    static QSSGRef<QSSGAbstractThreadPool> createThreadPool(qint32 inNumThreads = 4);
};
QT_END_NAMESPACE
#endif
