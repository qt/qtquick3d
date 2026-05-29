// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IESLIGHTPROFILEINDEX_H
#define IESLIGHTPROFILEINDEX_H

#include <QtCore/QObject>
#include <QtQml/qqml.h>

class IESLightProfileIndex : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject *light READ light WRITE setLight NOTIFY lightChanged FINAL)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged FINAL)
    Q_PROPERTY(float intensity READ intensity WRITE setIntensity NOTIFY intensityChanged FINAL)

public:
    explicit IESLightProfileIndex(QObject *parent = nullptr);

    QObject *light() const;
    int index() const;
    float intensity() const;

    void setLight(QObject *light);
    void setIndex(int index);
    void setIntensity(float intensity);

signals:
    void lightChanged();
    void indexChanged();
    void intensityChanged();

private:
    QObject *m_light = nullptr;
    int m_index = -1;
    float m_intensity = 1.0f;
};

#endif // IESLIGHTPROFILEINDEX_H
