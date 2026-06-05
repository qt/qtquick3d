// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtQuick3DRuntimeRender/private/qssgrendershadertokenizer_p.h>

using Token = QSSGShaderTokenizer::Token;

// Short aliases for the enumerators (an unscoped enum nested in the class, so the
// values live in the QSSGShaderTokenizer scope, not under Token).
static constexpr Token Comment = QSSGShaderTokenizer::Token_Comment;
static constexpr Token OpenBrace = QSSGShaderTokenizer::Token_OpenBrace;
static constexpr Token CloseBrace = QSSGShaderTokenizer::Token_CloseBrace;
static constexpr Token OpenParen = QSSGShaderTokenizer::Token_OpenParen;
static constexpr Token CloseParen = QSSGShaderTokenizer::Token_CloseParen;
static constexpr Token SemiColon = QSSGShaderTokenizer::Token_SemiColon;
static constexpr Token Identifier = QSSGShaderTokenizer::Token_Identifier;
static constexpr Token OpenBraket = QSSGShaderTokenizer::Token_OpenBraket;
static constexpr Token CloseBraket = QSSGShaderTokenizer::Token_CloseBraket;
static constexpr Token Unspecified = QSSGShaderTokenizer::Token_Unspecified;

class tst_ShaderTokenizer : public QObject
{
    Q_OBJECT

private:
    struct Lexed {
        Token token;
        QByteArray identifier; // The matched text, for Token_Identifier only.
    };

    static QList<Lexed> lex(const char *src)
    {
        QSSGShaderTokenizer tok;
        tok.initialize(src);
        QList<Lexed> out;
        for (Token t = tok.next(); t != QSSGShaderTokenizer::Token_EOF; t = tok.next()) {
            QByteArray id;
            if (t == QSSGShaderTokenizer::Token_Identifier)
                id = QByteArray(tok.identifier, tok.pos - tok.identifier);
            out.push_back({ t, id });
        }
        return out;
    }

    static QList<Token> kinds(const char *src)
    {
        QList<Token> out;
        const QList<Lexed> lexed = lex(src);
        out.reserve(lexed.size());
        for (const Lexed &l : lexed)
            out.push_back(l.token);
        return out;
    }

private slots:
    void empty();
    void punctuation();
    void identifiers();
    void identifierBoundaries();
    void identifierText();
    void lineComment();
    void blockComment();
    void braceInsideComment();
    void unterminatedBlockComment();
    void numbersAndOperatorsAreUnspecified();
    void preprocessorLinesAreNotSkipped();
};

void tst_ShaderTokenizer::empty()
{
    QCOMPARE(kinds(""), QList<Token>{});
}

void tst_ShaderTokenizer::punctuation()
{
    QCOMPARE(kinds("(){}[];"),
             (QList<Token>{ OpenParen, CloseParen, OpenBrace, CloseBrace, OpenBraket, CloseBraket, SemiColon }));
}

void tst_ShaderTokenizer::identifiers()
{
    QCOMPARE(kinds("foo bar_baz qux123"),
             (QList<Token>{ Identifier, Identifier, Identifier }));
}

void tst_ShaderTokenizer::identifierBoundaries()
{
    // Underscores and digits are part of an identifier; a dot is not.
    QCOMPARE(kinds("foo_bar"), (QList<Token>{ Identifier }));
    QCOMPARE(kinds("foo.bar"), (QList<Token>{ Identifier, Unspecified, Identifier }));

    // Adjacent punctuation terminates the identifier.
    QCOMPARE(kinds("MAIN()"), (QList<Token>{ Identifier, OpenParen, CloseParen }));
}

void tst_ShaderTokenizer::identifierText()
{
    const QList<Lexed> lexed = lex("foo + barBaz123");
    QCOMPARE(lexed.size(), 3);
    QCOMPARE(lexed.at(0).token, Identifier);
    QCOMPARE(lexed.at(0).identifier, QByteArray("foo"));
    QCOMPARE(lexed.at(1).token, Unspecified); // '+'
    QCOMPARE(lexed.at(2).token, Identifier);
    QCOMPARE(lexed.at(2).identifier, QByteArray("barBaz123"));
}

void tst_ShaderTokenizer::lineComment()
{
    QCOMPARE(kinds("a // comment with ; and }\nb"),
             (QList<Token>{ Identifier, Comment, Identifier }));
}

void tst_ShaderTokenizer::blockComment()
{
    QCOMPARE(kinds("a /* multi\nline */ b"),
             (QList<Token>{ Identifier, Comment, Identifier }));
}

void tst_ShaderTokenizer::braceInsideComment()
{
    // Braces, parens and semicolons inside comments must not be tokenized as such.
    const QList<Token> result = kinds("{ /* } ; ) */ }");
    QCOMPARE(result, (QList<Token>{ OpenBrace, Comment, CloseBrace }));
}

void tst_ShaderTokenizer::unterminatedBlockComment()
{
    // An unterminated block comment consumes the rest of the input.
    QCOMPARE(kinds("a /* never closed"),
             (QList<Token>{ Identifier, Comment }));
}

void tst_ShaderTokenizer::numbersAndOperatorsAreUnspecified()
{
    // The tokenizer only classifies identifiers, comments and a handful of punctuation;
    // everything else (numbers, operators, ...) is reported as Token_Unspecified.
    QCOMPARE(kinds("1 + 2"),
             (QList<Token>{ Unspecified, Unspecified, Unspecified }));
}

void tst_ShaderTokenizer::preprocessorLinesAreNotSkipped()
{
    // Unlike a real GLSL lexer, this tokenizer does not special-case '#': preprocessor
    // lines are tokenized like any other text, so the identifiers on them are still seen.
    QCOMPARE(kinds("#define FOO bar"),
             (QList<Token>{ Unspecified, Identifier, Identifier, Identifier }));
}

QTEST_APPLESS_MAIN(tst_ShaderTokenizer)

#include "tst_shadertokenizer.moc"
