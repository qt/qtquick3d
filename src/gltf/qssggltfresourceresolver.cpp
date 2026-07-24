// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qssggltfresourceresolver_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

/*!
    \namespace QSSGGltfResourceResolver
    \internal

    Resolves glTF URIs: data: URIs (base64 or percent-encoded), and relative
    or absolute file paths, including Qt resource (":/...") paths.
*/
namespace QSSGGltfResourceResolver {

/*!
    \internal

    Returns whether \a uri is a data: URI.
*/
bool isDataUri(QStringView uri)
{
    return uri.startsWith(QLatin1String("data:"), Qt::CaseInsensitive);
}

/*!
    \internal

    Decodes the payload of the data: URI \a uri. Returns an empty array and
    sets \a errorMessage on failure.
*/
QByteArray decodeDataUri(QStringView uri, QString *errorMessage)
{
    // data:[<mediatype>][;base64],<payload>
    const qsizetype comma = uri.indexOf(u',');
    if (!isDataUri(uri) || comma < 0) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Malformed data URI");
        return {};
    }

    const QStringView header = uri.mid(0, comma);
    const QStringView payload = uri.mid(comma + 1);

    if (header.endsWith(QLatin1String(";base64"), Qt::CaseInsensitive)) {
        auto result = QByteArray::fromBase64Encoding(payload.toLatin1());
        if (!result) {
            if (errorMessage)
                *errorMessage = QStringLiteral("Invalid base64 payload in data URI");
            return {};
        }
        return result.decoded;
    }

    // Non-base64 data URIs carry percent-encoded octets. Decoding has to stay
    // in QByteArray: going through QString would replace every octet sequence
    // that is not valid UTF-8 with U+FFFD, changing both the contents and the
    // length of what is meant to be arbitrary binary data.
    return QByteArray::fromPercentEncoding(payload.toLatin1());
}

/*!
    \internal

    Resolves the relative file URI \a uri against \a baseDir to a local or qrc
    path. The URI is percent-decoded as required by the glTF specification.

    A URI that would reach outside \a baseDir is rejected, which returns an
    empty string. The specification only allows relative references here, so
    an absolute path or one climbing out with ".." is malformed rather than
    merely unusual, and refusing it keeps a hostile asset from naming an
    arbitrary file for the application to load and read back.
*/
QString resolveFilePath(QStringView uri, const QString &baseDir)
{
    // glTF URIs are percent-encoded relative URI references
    const QString decoded = QUrl::fromPercentEncoding(uri.toUtf8());
    if (decoded.isEmpty())
        return {};
    if (QDir::isAbsolutePath(decoded) || decoded.startsWith(QLatin1String(":/")))
        return {};
    if (baseDir.isEmpty())
        return QDir::cleanPath(decoded).startsWith(QLatin1String("..")) ? QString() : decoded;

    const QString base = QDir::cleanPath(baseDir);
    const QString resolved = QDir::cleanPath(base + QLatin1Char('/') + decoded);
    if (resolved != base && !resolved.startsWith(base + QLatin1Char('/')))
        return {};
    return resolved;
}

/*!
    \internal

    Loads the contents referenced by the glTF URI \a uri: data: URIs are
    decoded, anything else is treated as a file path relative to \a baseDir.
    Sets \a errorMessage on failure.
*/
QByteArray loadUri(QStringView uri, const QString &baseDir, QString *errorMessage)
{
    if (isDataUri(uri))
        return decodeDataUri(uri, errorMessage);

    const QString path = resolveFilePath(uri, baseDir);
    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Refusing to resolve '%1' outside the asset directory").arg(uri);
        return {};
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to open '%1': %2").arg(path, file.errorString());
        return {};
    }
    return file.readAll();
}

} // namespace QSSGGltfResourceResolver

QT_END_NAMESPACE
