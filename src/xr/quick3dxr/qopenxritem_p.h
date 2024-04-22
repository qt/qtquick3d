// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QOPENXRITEM_H
#define QOPENXRITEM_H

#include <QQuickItem>
#include <QtQuick3D/private/qquick3dnode_p.h>

#include <QtQuick/private/qquicktranslate_p.h>

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

QT_BEGIN_NAMESPACE

class QOpenXRItemPrivate;
class QOpenXRView;

class QOpenXRItem : public QQuick3DNode
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOpenXRItem)

    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged FINAL)
    Q_PROPERTY(qreal pixelsPerUnit READ pixelsPerUnit WRITE setPixelsPerUnit NOTIFY pixelsPerUnitChanged FINAL)
    Q_PROPERTY(bool manualPixelsPerUnit READ manualPixelsPerUnit WRITE setManualPixelsPerUnit NOTIFY manualPixelsPerUnitChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)

    QML_NAMED_ELEMENT(XrItem)
public:
    struct TouchState
    {
        int pointId = -1;
        QOpenXRItem *target = nullptr;
        bool grabbed = false;
        bool pressed = false;
        qreal touchDistance = 1e6;
        QPointF cursorPos;
    };

    explicit QOpenXRItem(QQuick3DNode *parent = nullptr);
    ~QOpenXRItem() override;

    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *newContentItem);

    qreal pixelsPerUnit() const;
    void setPixelsPerUnit(qreal newPixelsPerUnit);

    QQuickScale *uiScale() const;
    void setUiScale(QQuickScale *newUiScale);

    bool manualPixelsPerUnit() const;
    void setManualPixelsPerUnit(bool newManualPixelsPerUnit);

    qreal width() const;
    void setWidth(qreal newWidth);

    qreal height() const;
    void setHeight(qreal newHeight);

    QColor color() const;
    void setColor(const QColor &newColor);

    void componentComplete() override;

    bool handleVirtualTouch(QOpenXRView *view, const QVector3D &pos, TouchState *touchState, QVector3D *offset);

signals:
    void contentItemChanged();
    void pixelsPerUnitChanged();
    void uiScaleChanged();
    void flagsChanged();
    void manualPixelsPerUnitChanged();
    void widthChanged();
    void heightChanged();
    void colorChanged();
};

QT_END_NAMESPACE

#endif // QOPENXRITEM_H
