// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRANIMATIONDRIVER_H
#define QOPENXRANIMATIONDRIVER_H

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


#include <QtCore/QAnimationDriver>

QT_BEGIN_NAMESPACE

class QOpenXRAnimationDriver : public QAnimationDriver
{
public:
    QOpenXRAnimationDriver();
    void advance() override;
    qint64 elapsed() const override;

    void setStep(int stepSize);

private:
    int m_step = 16;
    qint64 m_elapsed = 0;
};

QT_END_NAMESPACE

#endif // QOPENXRANIMATIONDRIVER_H
