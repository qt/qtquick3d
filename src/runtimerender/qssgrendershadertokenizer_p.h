// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSGRENDERSHADERTOKENIZER_P_H
#define QSSGRENDERSHADERTOKENIZER_P_H

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

#include "qtquick3druntimerenderexports.h"

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

// A minimal tokenizer for the GLSL-ish shader snippets used by the custom
// material, effect, quad texture provider and user render pass APIs. It does not
// try to be a full GLSL lexer; it only classifies the tokens these code paths
// need (identifiers, comments, braces, parens, brackets and semicolons) so that
// keyword detection/replacement can be done without re-implementing comment
// skipping and identifier boundary handling in each caller.
//
// This is based on the Qt Quick shader rewriter (with fixes).
class Q_QUICK3DRUNTIMERENDER_EXPORT QSSGShaderTokenizer
{
public:
    enum Token {
        Token_Comment,
        Token_OpenBrace,
        Token_CloseBrace,
        Token_OpenParen,
        Token_CloseParen,
        Token_SemiColon,
        Token_Identifier,
        Token_OpenBraket,
        Token_CloseBraket,
        Token_Unspecified,

        Token_EOF
    };

    void initialize(const char *input);
    Token next();

    const char *stream = nullptr;
    const char *pos = nullptr;
    const char *identifier = nullptr;
};

QT_END_NAMESPACE

#endif // QSSGRENDERSHADERTOKENIZER_P_H
