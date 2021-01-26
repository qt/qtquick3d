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

#ifndef QSSG_RENDER_INPUT_STREAM_FACTORY_H
#define QSSG_RENDER_INPUT_STREAM_FACTORY_H

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

#include <QtQuick3DUtils/private/qssgdataref_p.h>
#include <QtCore/QSharedPointer>
#include <QtCore/QIODevice>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE
// This class is threadsafe.
class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGInputStreamFactory
{
public:
    QAtomicInt ref;
    QMutex m_mutex;

    QSSGInputStreamFactory();

    ~QSSGInputStreamFactory();
    // These directories must have a '/' on them
    void addSearchDirectory(const QString &inDirectory);
    QSharedPointer<QIODevice> getStreamForFile(const QString &inFilename, bool inQuiet = false);
    // Return a path for this file.  Returns true if GetStreamForFile would return a valid
    // stream.
    // else returns false
    bool getPathForFile(const QString &inFilename, QString &outFile, bool inQuiet = false);
};
QT_END_NAMESPACE

#endif
