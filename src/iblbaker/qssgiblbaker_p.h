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

#ifndef QSSGIBLBAKERIMPORTER_P_H
#define QSSGIBLBAKERIMPORTER_P_H

#include "qtquick3diblbaker_p.h"
#include <QDir>

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

QT_BEGIN_NAMESPACE

class Q_QUICK3DIBLBAKER_EXPORT QSSGIblBaker
{
public:
    const QStringList inputExtensions() const;
    const QString outputExtension() const;
    const QString import(const QString &sourceFile, const QDir &savePath, QStringList *generatedFiles);

private:
    QByteArray createBsdfMipLevel(QByteArray &previousMipLevel, uint width, uint height);
    QStringList m_generatedFiles;
};

QT_END_NAMESPACE

#endif // QSSGIBLBAKERIMPORTER_P_H
