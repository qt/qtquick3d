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

#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QtQml/qqmlregistration.h>

#include <QtGui/qsyntaxhighlighter.h>
#include <QtQuick/qquicktextdocument.h>

QT_BEGIN_NAMESPACE

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument * document READ document WRITE setDocument NOTIFY documentChanged)
    QML_ELEMENT
public:
    enum BlockState
    {
        None,
        Comment
    };

    explicit SyntaxHighlighter(QObject *p = nullptr);

    // Shadows
    QQuickTextDocument *document() const;
    void setDocument(QQuickTextDocument *newDocument);

signals:
    void documentChanged();

protected:
    void highlightBlock(const QString &text) final;
    QPointer<QQuickTextDocument> m_quickTextDocument;
    QSet<QByteArrayView> m_keywords;
    QSet<QByteArrayView> m_argumentKeywords;

private:
    void highlightLine(const QString &text, int position, int length, const QTextCharFormat &format);
};

QT_END_NAMESPACE

#endif // SYNTAXHIGHLIGHTER_H
