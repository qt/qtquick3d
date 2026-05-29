// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FOG3DVOLUME_H
#define FOG3DVOLUME_H

#include <QtQuick3D/private/qquick3dnode_p.h>
#include <QtGui/QVector3D>
#include <QtGui/QColor>

class Fog3DVolume : public QQuick3DNode
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(FogType type READ type WRITE setType NOTIFY typeChanged FINAL)
    Q_PROPERTY(QVector3D extents READ extents WRITE setExtents NOTIFY extentsChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(float density READ density WRITE setDensity NOTIFY densityChanged FINAL)
    Q_PROPERTY(bool heightEnabled READ heightEnabled WRITE setHeightEnabled NOTIFY heightEnabledChanged FINAL)
    Q_PROPERTY(float leastIntenseY READ leastIntenseY WRITE setLeastIntenseY NOTIFY leastIntenseYChanged FINAL)
    Q_PROPERTY(float mostIntenseY READ mostIntenseY WRITE setMostIntenseY NOTIFY mostIntenseYChanged FINAL)
    Q_PROPERTY(float heightCurve READ heightCurve WRITE setHeightCurve NOTIFY heightCurveChanged FINAL)
    Q_PROPERTY(QVector3D noiseOffset READ noiseOffset WRITE setNoiseOffset NOTIFY noiseOffsetChanged FINAL)
    Q_PROPERTY(float noiseScale READ noiseScale WRITE setNoiseScale NOTIFY noiseScaleChanged FINAL)

public:
    enum FogType { Box = 0, Sphere = 1 };
    Q_ENUM(FogType)

    explicit Fog3DVolume(QQuick3DNode *parent = nullptr);

    FogType type() const;
    QVector3D extents() const;
    QColor color() const;
    float density() const;
    bool heightEnabled() const;
    float leastIntenseY() const;
    float mostIntenseY() const;
    float heightCurve() const;
    QVector3D noiseOffset() const;
    float noiseScale() const;

    void setType(FogType v);
    void setExtents(const QVector3D &v);
    void setColor(const QColor &v);
    void setDensity(float v);
    void setHeightEnabled(bool v);
    void setLeastIntenseY(float v);
    void setMostIntenseY(float v);
    void setHeightCurve(float v);
    void setNoiseOffset(const QVector3D &v);
    void setNoiseScale(float v);

signals:
    void typeChanged();
    void extentsChanged();
    void colorChanged();
    void densityChanged();
    void heightEnabledChanged();
    void leastIntenseYChanged();
    void mostIntenseYChanged();
    void heightCurveChanged();
    void noiseOffsetChanged();
    void noiseScaleChanged();

private:
    FogType m_type = Sphere;
    QVector3D m_extents = { 100.f, 100.f, 100.f };
    QColor m_color = Qt::white;
    float m_density = 1.0f;
    bool m_heightEnabled = false;
    float m_leastIntenseY = 10.0f;
    float m_mostIntenseY = 0.0f;
    float m_heightCurve = 1.0f;
    QVector3D m_noiseOffset;
    float m_noiseScale = 0.5f;
};

#endif // FOG3DVOLUME_H
