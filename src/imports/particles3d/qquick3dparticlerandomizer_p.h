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

#ifndef QQUICK3DPARTICLERANDOMIZER_H
#define QQUICK3DPARTICLERANDOMIZER_H

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

#include <QRandomGenerator>
#include <QList>

QT_BEGIN_NAMESPACE

/*

Simple helper to get pseudo-random numbers which remain the same per particle & user.
Particles don't need strong randomization and the ability to seed can be useful.

Based on brief testing on my laptop, getting 1.000.000 random numbers:

1) Using QRandomGenerator::global()->generateDouble()
-> ~120ms

2) Using QPRand::get(), with s_size 4096
-> ~8ms

3) Using QPRand::get(i, j), with s_size 4096
-> ~10ms

4) Using QPRand::get(i, j), with s_size 100000
-> ~10ms

So QPRand seems fast and increasing keys amount doesn't notably affect the performance,
just the memory usage. With more particles s_size should be relatively big to make sure
particles appear random enough.

Bounded usage tips:
- qrg->bounded(4.0) == QPRand::get() * 4.0
- qrg->bounded(4) == int(QPRand::get() * 4)

*/

class QPRand
{
public:

    // Add here new enums for diferent "users".
    // This way e.g. particle can vary little on colors but more on sizes.
    enum UserType {
        Default = 0,
        WanderXPS, // PaceStart
        WanderYPS,
        WanderZPS,
        WanderXPV, // PaceVariation
        WanderYPV,
        WanderZPV,
        WanderXAV, // AmountVariation
        WanderYAV,
        WanderZAV,
        AttractorDurationV,
        AttractorPosVX,
        AttractorPosVY,
        AttractorPosVZ
    };

    static void init(quint32 seed, int size = 65536) {
        s_size = size;
        s_generator.seed(seed);
        s_randomList.clear();
        s_randomList.reserve(s_size);
        for (int i = 0; i < s_size; i++) {
            s_randomList << float(s_generator.generateDouble());
        }
    }

    // Return float 0.0 - 1.0
    // With the same input values, returns always the same output.
    inline static float get(int particleIndex, UserType user = Default) {
        int i = (particleIndex + user) % s_size;
        return s_randomList.at(i);
    }

    // Return float 0.0 - 1.0 from random list
    inline static float get() {
        s_index = (s_index < s_size - 1) ? s_index + 1 : 0;
        return s_randomList.at(s_index);
    }

private:
    static QRandomGenerator s_generator;
    static int s_size;
    static int s_index;
    static QList<float> s_randomList;

};

QT_END_NAMESPACE

#endif // QQUICK3DPARTICLERANDOMIZER_H
