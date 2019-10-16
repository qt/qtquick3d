/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef KEYFRAMEGROUPGENERATOR_H
#define KEYFRAMEGROUPGENERATOR_H

#include <QString>
#include <QHash>
#include "uippresentation.h"
#include "uipparser.h"

QT_BEGIN_NAMESPACE

class KeyframeGroupGenerator
{
public:
    struct KeyframeGroup {
        enum AnimationType {
            NoAnimation = 0,
            Linear,
            EaseInOut,
            Bezier
        };
        struct KeyFrame {
            enum ValueType {
                Unhandled = -1,
                Float = 0,
                Vector2D,
                Vector3D,
                Vector4D,
                Color
            };

            KeyFrame() = default;
            KeyFrame(const AnimationTrack::KeyFrame &keyframe, ValueType type,
                     const QString &field = QStringLiteral("x"), float fps = 60.f);
            void setValue(float newValue, const QString &field = QStringLiteral("x"));
            int frame = 0;
            QVector4D value;
            ValueType valueType = Float;
            union {
                float easeIn;
                float c2time;
            };
            union {
                float easeOut;
                float c2value;
            };
            float c1time;
            float c1value;
            QString valueToString() const;
        };
        using KeyFrameList = QVector<KeyFrame*>;
        KeyframeGroup() = default;
        KeyframeGroup(const AnimationTrack &animation, const QString &p,
                      const QString &field = QStringLiteral("x"), float fps = 60.f);
        ~KeyframeGroup();

        void generateKeyframeGroupQml(QTextStream &output, int tabLevel) const;

        static KeyFrame::ValueType getPropertyValueType(GraphObject::Type type,
                                                        const QString &propertyName);
        QString getQmlPropertyName(const QString &propertyName);

        AnimationType type = NoAnimation;
        GraphObject *target;
        QString property;
        bool isDynamic = false;
        KeyFrameList keyframes;
    };

    KeyframeGroupGenerator(float fps);
    ~KeyframeGroupGenerator();

    void addAnimation(const AnimationTrack &animation);

    void generateKeyframeGroups(QTextStream &output, int tabLevel);

private:
    using KeyframeGroupMap = QHash<QString, KeyframeGroup *>;
    QHash<GraphObject *, KeyframeGroupMap> m_targetKeyframeMap;
    float m_fps = 60.f;
};

QT_END_NAMESPACE

#endif // KEYFRAMEGROUPGENERATOR_H
