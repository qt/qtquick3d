// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenxritem_p.h"
#include "qopenxrview_p.h"
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
            m_contentItem->setParentItem(m_containerItem);
            m_contentItem->setParent(m_containerItem);
            m_contentItemDestroyedConnection = QObject::connect(m_contentItem, &QObject::destroyed, q, [q]() {
                q->setContentItem(nullptr);
            });
        }

        updateContent();
    }

    void initParentItem()
    {
        Q_Q(QOpenXRItem);
        if (!m_containerItem) {
            m_containerItem = new QQuickRectangle;
            m_containerItem->setTransformOrigin(QQuickItem::TopLeft);
            m_containerItem->setParent(q);

            auto dataProp = data();
            dataProp.append(&dataProp, m_containerItem);
        }
    }

    void updateContent();

    QQuickItem *m_contentItem = nullptr;
    QQuickRectangle *m_containerItem = nullptr;
    QPointer<QOpenXRView> m_XrView;
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
    m_containerItem->setColor(m_color);
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
        m_containerItem->setSize(newSize);
        m_containerItem->setScale(newScale);
    }
}

/*!
    \qmltype XrItem
    \inqmlmodule QtQuick3D.Xr
    \inherits Node
    \brief A virtual surface in 3D space that can hold 2D user interface content

    The XrItem type is a Qt Quick 3D \l Node that represents a rectangle with \l width and \l height.
    It holds one Qt Quick \l Item, specified by \l contentItem, and scales it to fit.
    This gives a convenient way to take traditional 2D user interfaces and display them on a virtual surface that has
    a real world size.

    Any other children of the XrItem will be treated as normal children of a Node, and will not be scaled.

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
*/
QOpenXRItem::QOpenXRItem(QQuick3DNode *parent)
    : QQuick3DNode(*(new QOpenXRItemPrivate()), parent)
{
}

QOpenXRItem::~QOpenXRItem()
{
    Q_D(QOpenXRItem);
    if (d->m_XrView)
        d->m_XrView->unregisterXrItem(this);
}

void QOpenXRItem::componentComplete()
{
    Q_D(QOpenXRItem);
    QQuick3DNode::componentComplete(); // Sets d->componentComplete, so must be called first

    auto findView = [this]() -> QOpenXRView * {
        QQuick3DNode *parent = parentNode();
        while (parent) {
            if (auto *xrView = qobject_cast<QOpenXRView*>(parent))
                return xrView;
            parent = parent->parentNode();
        }
        return nullptr;
    };
    d->m_XrView = findView();
    if (d->m_XrView)
        d->m_XrView->registerXrItem(this);
    else
        qWarning("Could not find XrView for XrItem");
    d->updateContent();
}

/*!
    \qmlproperty Item XrItem::contentItem

    This property holds the content item that will be displayed on the virtual surface.
    The content item's size will be used to calculate the pixels per unit value and scale based on this item's size.

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

    If this property is \c true, the ratio between the contentItems's 2D coordinate system and this
    XrItem's 3D coordinate system is defined by the value of \l pixelsPerUnit. If this property is \c false,
    the ratio is calculated based on the content item's size and the size of the XrItem.

    \default false
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

// Sends appropriate touch events.
// Updates the touchState and returns true if this item grabs the touch point.
// touchState is input/output, and input contains the previous state if touchState->grabbed is true

bool QOpenXRItem::handleVirtualTouch(QOpenXRView *view, const QVector3D &pos, TouchState *touchState, QVector3D *offset)
{
    Q_D(QOpenXRItem);

    auto mappedPos = mapPositionFromScene(pos);

    QPointF point = {mappedPos.x(), -mappedPos.y()};

    constexpr qreal sideMargin = 20; // How far outside the rect do you have to go to cancel the grab (cm)
    constexpr qreal cancelDepth = 50; // How far through the rect do you have to go to cancel the grab (cm)
    constexpr qreal hoverHeight = 10; // How far above does the hover state begin (cm). NOTE: no hover events actually sent

    const float z = mappedPos.z();

    const bool wayOutside = point.x() < -sideMargin || point.x() > width() + sideMargin
            || point.y() < -sideMargin || point.y() > height() + sideMargin || z < -cancelDepth;
    const bool inside = point.x() >= 0 && point.x() <= width() && point.y() >= 0 && point.y() <= height() && !wayOutside;

    const bool wasGrabbed = touchState->grabbed;

    bool hover = z > 0 && z < hoverHeight;

    bool pressed = false;
    bool grab;
    if (wasGrabbed) {
        // We maintain a grab as long as we don't move too far away while pressed, or if we hover inside
        pressed = z <= 0; // TODO: we should release when the finger is moved upwards, even if it's below the surface
        grab = (pressed && !wayOutside) || (hover && inside);
    } else {
        // We don't want movement behind the surface to register as pressed, so we need at least one hover before a press
        grab = hover && inside;
    }

    if (grab) {
        float zOffset = qMin(z, 0.0);
        if (offset)
            *offset = sceneRotation() * QVector3D{ 0, 0, -zOffset };
        touchState->grabbed = true;
        touchState->target = this;
        touchState->touchDistance = z;
        touchState->pressed = pressed;
        touchState->cursorPos = point;
        view->setTouchpoint(d->m_containerItem, point, touchState->pointId, pressed);
        return true;
    }

    if (wasGrabbed) {
        touchState->grabbed = false;
        touchState->target = nullptr;
        view->setTouchpoint(d->m_containerItem, point, touchState->pointId, false);
    }

    return false;
}

/*!
   \qmlproperty color XrItem::color

    This property defines the background color of the XrItem.

    \default "white"
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
