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

#ifndef QSSG_GLSLSYMBOLS_H
#define QSSG_GLSLSYMBOLS_H

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

#include <QtQuick3DGlslParser/private/glsltype_p.h>
#include <QtQuick3DGlslParser/private/glslsymbol_p.h>
#include <QVector>
#include <QString>
#include <QHash>

QT_BEGIN_NAMESPACE

namespace GLSL {

class Q_QUICK3DGLSLPARSER_EXPORT Argument: public Symbol
{
public:
    Argument(Function *scope);

    const Type *type() const override;
    void setType(const Type *type);

    Argument *asArgument() override { return this; }

private:
    const Type *_type;
};

class Q_QUICK3DGLSLPARSER_EXPORT Variable: public Symbol
{
public:
    Variable(Scope *scope);

    const Type *type() const override;
    void setType(const Type *type);

    int qualifiers() const { return _qualifiers; }
    void setQualifiers(int qualifiers) { _qualifiers = qualifiers; }

    Variable *asVariable() override { return this; }

private:
    const Type *_type;
    int _qualifiers;
};

class Q_QUICK3DGLSLPARSER_EXPORT Block: public Scope
{
public:
    Block(Scope *enclosingScope = nullptr);

    QList<Symbol *> members() const override;
    void add(Symbol *symbol) override;

    Block *asBlock() override { return this; }

    const Type *type() const override;
    Symbol *find(const QString &name) const override;

private:
    QHash<QString, Symbol *> _members;
};

class Q_QUICK3DGLSLPARSER_EXPORT Namespace: public Scope
{
public:
    Namespace();
    ~Namespace() override;

    void add(Symbol *symbol) override;

    Namespace *asNamespace() override { return this; }

    QList<Symbol *> members() const override;
    const Type *type() const override;
    Symbol *find(const QString &name) const override;

private:
    QHash<QString, Symbol *> _members;
    QVector<OverloadSet *> _overloadSets;
};

} // namespace GLSL

QT_END_NAMESPACE

#endif // QSSG_GLSLSYMBOLS_H
