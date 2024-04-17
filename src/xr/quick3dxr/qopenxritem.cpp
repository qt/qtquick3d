// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxritem_p.h"
#include <QtQuick3D/private/qquick3dnode_p_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QColor>

QT_BEGIN_NAMESPACE


class QOpenXRItemPrivate : public QQuick3DNodePrivate
{
    Q_DECLARE_PUBLIC(QOpenXRItem)
public:
    QOpenXRItemPrivate() : QQuick3DNodePrivate(QOpenXRItemPrivate::Type::Node)
    {
    }

    void setContentItem(QQuickItem *newContentItem)
    {
        Q_Q(QOpenXRItem);
        m_contentItem = newContentItem;

        initParentItem();

        if (m_contentItemDestroyedConnection) {
            QObject::disconnect(m_contentItemDestroyedConnection);
            m_contentItemDestroyedConnection = {};
        }
        if (m_contentItem) {
            m_contentItem->setParentItem(m_parentItem);
            m_contentItem->setParent(m_parentItem);
            m_contentItemDestroyedConnection = QObject::connect(m_contentItem, &QObject::destroyed, q, [q]() {
                q->setContentItem(nullptr);
            });
        }

        updateContent();
    }

    void initParentItem()
    {
        Q_Q(QOpenXRItem);
        if (!m_parentItem) {
            m_parentItem = new QQuickRectangle;
            m_parentItem->setTransformOrigin(QQuickItem::TopLeft);
            m_parentItem->setParent(q);

            auto dataProp = data();
            dataProp.append(&dataProp, m_parentItem);
        }
    }

    void updateContent();

    QQuickItem *m_contentItem = nullptr;
    QQuickRectangle *m_parentItem = nullptr;
    QMetaObject::Connection m_contentItemDestroyedConnection;
    QColor m_color = Qt::white;
    qreal m_pixelsPerUnit { 1.0 };
    qreal m_width = 1;
    qreal m_height = 1;
    bool m_manualPixelsPerUnit = false;
};

static inline qreal calculatePPU(qreal pxWidth, qreal pxHeight, qreal diagonal)
{
    return (diagonal > 0) ? std::sqrt((pxWidth * pxWidth) + (pxHeight * pxHeight)) / diagonal : 1.0;
}

void QOpenXRItemPrivate::updateContent()
{
    if (!componentComplete)
        return;
    Q_Q(QOpenXRItem);
    initParentItem();
    m_parentItem->setColor(m_color);
    if (m_contentItem) {
        if (Q_UNLIKELY(m_manualPixelsPerUnit && m_pixelsPerUnit < 0)) {
            qWarning() << "XrItem invalid pixelPerUnit" << m_pixelsPerUnit;
            return;
        }
        qreal newScale;
        if (m_manualPixelsPerUnit) {
            newScale = 1.0 / m_pixelsPerUnit;

        } else {
            qreal diagonal = std::sqrt((m_width * m_width) + (m_height * m_height));
            qreal ppu = calculatePPU(m_contentItem->width(), m_contentItem->height(), diagonal);
            if (ppu <= 0)
                ppu = 1.0;
            q->setPixelsPerUnit(ppu);
            newScale = 1.0 / ppu;
        }
        QSizeF newSize(m_width / newScale, m_height / newScale);
        m_parentItem->setSize(newSize);
        m_parentItem->setScale(newScale);
    }
}

/*!
    \qmltype XrItem
    \inqmlmodule QtQuick3D.Xr
    \inherits Node
    \brief A virtual surface in 3D space that can hold 2D user interface content
*/
QOpenXRItem::QOpenXRItem(QQuick3DNode *parent)
    : QQuick3DNode(*(new QOpenXRItemPrivate()), parent)
{
}

/*!
    \qmlproperty Item XrItem::contentItem

    This property holds the content item that will be displayed on the virtual surface.
    The content item's size will be used to calculate the pixels per unit value and scale based on this items size.

    This item is a convenient way to take traditional 2D user interfaces and display them on a virtual surface that has
    a real world size.

    For example the following code will create a virtual surface that's 1 meter by 1 meter and with a content item
    that's 600 pixels by 600 pixels. Note that the effect here is achieved by scaling the content item and not
    by rendering the content item at a higher resolution.

    \code
    XrItem {
        width: 100
        height: 100
        contentItem: Rectangle {
            width: 600
            height: 600
            color: "red"
        }
    }
    \endcode

    \sa pixelsPerUnit
 */
QQuickItem *QOpenXRItem::contentItem() const
{
    Q_D(const QOpenXRItem);
    return d->m_contentItem;
}

void QOpenXRItem::setContentItem(QQuickItem *newContentItem)
{
    Q_D(QOpenXRItem);
    if (d->m_contentItem == newContentItem)
        return;

    d->setContentItem(newContentItem);
    emit contentItemChanged();
}

/*!
    \qmlproperty real XrItem::pixelsPerUnit

    This property determines how many pixels in the contentItems's 2D coordinate system will fit
    in one unit of the XrItem's 3D coordinate system. By default, this is calculated based on the
    content item's size and the size of the XrItem.

    Set \l manualPixelsPerUnit to disable the default behavior and set the pixels per unit value manually.

    \sa manualPixelsPerUnit
 */
qreal QOpenXRItem::pixelsPerUnit() const
{
    Q_D(const QOpenXRItem);
    return d->m_pixelsPerUnit;
}

void QOpenXRItem::setPixelsPerUnit(qreal newPixelsPerUnit)
{
    Q_D(QOpenXRItem);
    if (qFuzzyCompare(d->m_pixelsPerUnit, newPixelsPerUnit))
        return;

    d->m_pixelsPerUnit = newPixelsPerUnit;

    if (d->m_manualPixelsPerUnit)
        d->updateContent();

    emit pixelsPerUnitChanged();
}

/*!
    \qmlproperty bool XrItem::manualPixelsPerUnit

    If this property is true, the ratio between the contentItems's 2D coordinate system and this
    XrItem's 3D coordinate system is defined by the value of \l pixelsPerUnit. If this property is false,
    the ratio is calculated based on the content item's size and the size of the XrItem.

    The default value is false.
    \sa pixelsPerUnit
*/

bool QOpenXRItem::manualPixelsPerUnit() const
{
    Q_D(const QOpenXRItem);
    return d->m_manualPixelsPerUnit;
}

void QOpenXRItem::setManualPixelsPerUnit(bool newManualPixelsPerUnit)
{
    Q_D(QOpenXRItem);
    if (d->m_manualPixelsPerUnit == newManualPixelsPerUnit)
        return;
    d->m_manualPixelsPerUnit = newManualPixelsPerUnit;
    d->updateContent();
    emit manualPixelsPerUnitChanged();
}

/*!
    \qmlproperty real XrItem::width

    This property defines the width of the XrItem in the 3D coordinate system.
    \sa height
*/

qreal QOpenXRItem::width() const
{
    Q_D(const QOpenXRItem);
    return d->m_width;
}

void QOpenXRItem::setWidth(qreal newWidth)
{
    Q_D(QOpenXRItem);
    if (d->m_width == newWidth)
        return;
    d->m_width = newWidth;
    emit widthChanged();

    d->updateContent();
}

/*!
    \qmlproperty real XrItem::height

    This property defines the height of the XrItem in the 3D coordinate system.
    \sa width
*/

qreal QOpenXRItem::height() const
{
    Q_D(const QOpenXRItem);
    return d->m_height;
}

void QOpenXRItem::setHeight(qreal newHeight)
{
    Q_D(QOpenXRItem);
    if (d->m_height == newHeight)
        return;
    d->m_height = newHeight;
    emit widthChanged();

    d->updateContent();
}

void QOpenXRItem::componentComplete()
{
    Q_D(QOpenXRItem);
    QQuick3DNode::componentComplete();
    d->updateContent();
}

/*!
   \qmlproperty color XrItem::color

    This property defines the background color of the XrItem.

    The default value is \c white
 */

QColor QOpenXRItem::color() const
{
    Q_D(const QOpenXRItem);
    return d->m_color;
}

void QOpenXRItem::setColor(const QColor &newColor)
{
    Q_D(QOpenXRItem);
    if (d->m_color == newColor)
        return;
    d->m_color = newColor;
    emit colorChanged();
    d->updateContent();
}

QT_END_NAMESPACE
