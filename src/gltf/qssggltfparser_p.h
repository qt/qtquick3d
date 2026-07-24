// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSGGLTFPARSER_P_H
#define QSSGGLTFPARSER_P_H

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

#include <QtQuick3DGltf/private/qssggltfdocument_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQuick3DGltf, Q_QUICK3DGLTF_EXPORT)

class Q_QUICK3DGLTF_EXPORT QSSGGltfParser
{
public:
    static QStringList supportedExtensions();

    bool parse(const QByteArray &data, const QString &baseDir, QSSGGltfDocument *document);
    bool parseFile(const QString &filePath, QSSGGltfDocument *document);

    QString errorMessage() const { return m_errorMessage; }

private:
    bool setError(const QString &message);

    QString m_errorMessage;
};

QT_END_NAMESPACE

#endif // QSSGGLTFPARSER_P_H
