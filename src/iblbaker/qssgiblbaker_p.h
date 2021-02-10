/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
