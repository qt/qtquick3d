// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IESTEXTUREDATA_H
#define IESTEXTUREDATA_H
#include <QQuick3DTextureData>
#include <QUrl>
#include <QVector>
#include <QList>
#include <QTextStream>

class IESTextureData : public QQuick3DTextureData
{
    Q_OBJECT
    Q_PROPERTY(QList<QUrl> sources READ sources WRITE setSources NOTIFY sourcesChanged)
    QML_ELEMENT
public:
    explicit IESTextureData(QQuick3DObject *parent = nullptr);
    QList<QUrl> sources() const;
    void setSources(const QList<QUrl> &sources);
signals:
    void sourcesChanged();
private:
    struct IESData {
        int numVerticalAngles = 0;
        int numHorizontalAngles = 0;
        float candelaMultiplier = 1.0f;
        float maxCandela = 0.0f;
        QVector<float> verticalAngles;
        QVector<float> horizontalAngles;
        QVector<QVector<float>> candelaValues;
    };
    void parseIESFiles();
    IESData parseIESFile(const QUrl &source);
    QVector<float> readFloats(QTextStream &in, int count);
    void generateTexture(const QVector<IESData> &dataList);
    float getIntensity(const IESData &data, float theta, float phi) const;
    QList<QUrl> m_sources;
};
#endif // IESTEXTUREDATA_H
