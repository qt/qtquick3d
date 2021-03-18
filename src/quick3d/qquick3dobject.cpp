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

#include "qquick3dobject.h"
#include "qquick3dobject_p.h"
#include "qquick3dscenemanager_p.h"
#include "qquick3ditem2d_p.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergraphobject_p.h>

#include <QtQml/private/qqmlglobal_p.h>
#include <QtQuick/private/qquickstategroup_p.h>
#include <QtQuick/private/qquickstate_p.h>
#include <QtQuick/private/qquickitem_p.h>

#include <private/qv4object_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE


/*!
    \qmltype Object3D
    \inqmlmodule QtQuick3D
    \brief Abstract base type of all 3D nodes and resources.
*/

/*!
    \class QQuick3DObject
    \inmodule QtQuick3D
    \since 5.15
    \brief Base class of all 3D nodes and resources.
*/

QQuick3DObject::QQuick3DObject(QQuick3DObject *parent)
    : QObject(*(new QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type::Unknown)), parent)
{
    Q_D(QQuick3DObject);
    d->init(parent);
}

QQuick3DObject::~QQuick3DObject()
{
    Q_D(QQuick3DObject);
    if (d->windowRefCount > 1)
        d->windowRefCount = 1; // Make sure window is set to null in next call to derefWindow().
    if (d->parentItem)
        setParentItem(nullptr);
    else if (d->sceneManager)
        QQuick3DObjectPrivate::derefSceneManager(this);

    // XXX todo - optimize
    while (!d->childItems.isEmpty())
        d->childItems.constFirst()->setParentItem(nullptr);

    delete d->_stateGroup;
    d->_stateGroup = nullptr;
}

void QQuick3DObject::update()
{
    Q_D(QQuick3DObject);
    d->dirty(QQuick3DObjectPrivate::Content);
}

void QQuick3DObject::setParentItem(QQuick3DObject *parentItem)
{
    Q_D(QQuick3DObject);
    if (parentItem == d->parentItem)
        return;

    if (parentItem) {
        QQuick3DObject *itemAncestor = parentItem;
        while (itemAncestor != nullptr) {
            if (Q_UNLIKELY(itemAncestor == this)) {
                qWarning() << "QSSGObject::setParentItem: Parent" << parentItem << "is already part of the subtree of" << this;
                return;
            }
            itemAncestor = itemAncestor->parentItem();
        }
    }

    d->removeFromDirtyList();

    QQuick3DObject *oldParentItem = d->parentItem;

    if (oldParentItem) {
        QQuick3DObjectPrivate *op = QQuick3DObjectPrivate::get(oldParentItem);

        op->removeChild(this);
    } else if (d->sceneManager) {
        d->sceneManager->parentlessItems.remove(this);
    }

    const auto parentSceneManager = parentItem ? QQuick3DObjectPrivate::get(parentItem)->sceneManager : nullptr;
    if (d->sceneManager == parentSceneManager) {
        // Avoid freeing and reallocating resources if the window stays the same.
        d->parentItem = parentItem;
    } else {
        if (d->sceneManager)
            QQuick3DObjectPrivate::derefSceneManager(this);
        d->parentItem = parentItem;
        if (parentSceneManager)
            QQuick3DObjectPrivate::refSceneManager(this, parentSceneManager);
    }

    d->dirty(QQuick3DObjectPrivate::ParentChanged);

    if (d->parentItem)
        QQuick3DObjectPrivate::get(d->parentItem)->addChild(this);
    else if (d->sceneManager)
        d->sceneManager->parentlessItems.insert(this);

    d->itemChange(ItemParentHasChanged, d->parentItem);

    emit parentChanged();
}

QString QQuick3DObject::state() const
{
    Q_D(const QQuick3DObject);
    return d->state();
}

void QQuick3DObject::setState(const QString &state)
{
    Q_D(QQuick3DObject);
    d->setState(state);
}

QList<QQuick3DObject *> QQuick3DObject::childItems() const
{
    Q_D(const QQuick3DObject);
    return d->childItems;
}

QQuick3DObject *QQuick3DObject::parentItem() const
{
    Q_D(const QQuick3DObject);
    return d->parentItem;
}

void QQuick3DObject::markAllDirty()
{
}

void QQuick3DObject::itemChange(QQuick3DObject::ItemChange, const QQuick3DObject::ItemChangeData &)
{
}

QQuick3DObject::QQuick3DObject(QQuick3DObjectPrivate &dd, QQuick3DObject *parent)
    : QObject(dd, parent)
{
    Q_D(QQuick3DObject);
    d->init(parent);
}

void QQuick3DObject::classBegin()
{
    Q_D(QQuick3DObject);
    d->componentComplete = false;
    if (d->_stateGroup)
        d->_stateGroup->classBegin();
}

void QQuick3DObject::componentComplete()
{
    Q_D(QQuick3DObject);
    d->componentComplete = true;
    if (d->_stateGroup)
        d->_stateGroup->componentComplete();

    if (d->sceneManager && d->dirtyAttributes) {
        d->addToDirtyList();
        d->sceneManager->dirtyItem(this);
    }
}

bool QQuick3DObject::isComponentComplete() const
{
    Q_D(const QQuick3DObject);
    return d->componentComplete;
}

void QQuick3DObject::updatePropertyListener(QQuick3DObject *newO,
                                            QQuick3DObject *oldO,
                                            const QSharedPointer<QQuick3DSceneManager> &window,
                                            const QByteArray &propertyKey,
                                            QQuick3DObject::ConnectionMap &connections,
                                            const std::function<void(QQuick3DObject *)> &callFn)
{
    // disconnect previous destruction listern
    if (oldO) {
        if (window)
            QQuick3DObjectPrivate::derefSceneManager(oldO);

        auto connection = connections.find(propertyKey);
        if (connection != connections.end()) {
            QObject::disconnect(connection.value());
            connections.erase(connection);
        }
    }

    // listen for new map's destruction
    if (newO) {
        if (window)
            QQuick3DObjectPrivate::refSceneManager(newO, window);
        auto connection = QObject::connect(newO, &QObject::destroyed, [callFn](){
            callFn(nullptr);
        });
        connections.insert(propertyKey, connection);
    }
}

QQuick3DObjectPrivate::QQuick3DObjectPrivate(QQuick3DObjectPrivate::Type t)
    : _stateGroup(nullptr)
    , dirtyAttributes(0)
    , nextDirtyItem(nullptr)
    , prevDirtyItem(nullptr)
    , sceneManager(nullptr)
    , windowRefCount(0)
    , parentItem(nullptr)
    , sortedChildItems(&childItems)
    , subFocusItem(nullptr)
    , type(t)
{
}

QQuick3DObjectPrivate::~QQuick3DObjectPrivate()
{
    if (sortedChildItems != &childItems)
        delete sortedChildItems;
}

void QQuick3DObjectPrivate::init(QQuick3DObject *parent)
{
    Q_Q(QQuick3DObject);

    if (parent)
        q->setParentItem(parent);
}

QQmlListProperty<QObject> QQuick3DObjectPrivate::data()
{
    return QQmlListProperty<QObject>(q_func(),
                                     nullptr,
                                     QQuick3DObjectPrivate::data_append,
                                     QQuick3DObjectPrivate::data_count,
                                     QQuick3DObjectPrivate::data_at,
                                     QQuick3DObjectPrivate::data_clear);
}

QQmlListProperty<QObject> QQuick3DObjectPrivate::resources()
{
    return QQmlListProperty<QObject>(q_func(),
                                     nullptr,
                                     QQuick3DObjectPrivate::resources_append,
                                     QQuick3DObjectPrivate::resources_count,
                                     QQuick3DObjectPrivate::resources_at,
                                     QQuick3DObjectPrivate::resources_clear);
}

QQmlListProperty<QQuick3DObject> QQuick3DObjectPrivate::children()
{
    return QQmlListProperty<QQuick3DObject>(q_func(),
                                          nullptr,
                                          QQuick3DObjectPrivate::children_append,
                                          QQuick3DObjectPrivate::children_count,
                                          QQuick3DObjectPrivate::children_at,
                                          QQuick3DObjectPrivate::children_clear);
}

QQmlListProperty<QQuickState> QQuick3DObjectPrivate::states()
{
    return _states()->statesProperty();
}

QQmlListProperty<QQuickTransition> QQuick3DObjectPrivate::transitions()
{
    return _states()->transitionsProperty();
}

QString QQuick3DObjectPrivate::state() const
{
    return _stateGroup ? _stateGroup->state() : QString();
}

void QQuick3DObjectPrivate::setState(const QString &state)
{
    _states()->setState(state);
}

void QQuick3DObjectPrivate::data_append(QQmlListProperty<QObject> *prop, QObject *o)
{
    if (!o)
        return;

    QQuick3DObject *that = static_cast<QQuick3DObject *>(prop->object);

    if (QQuick3DObject *item = qmlobject_cast<QQuick3DObject *>(o)) {
        item->setParentItem(that);

    } else {
//        QSSGSceneRenderer *thisSceneRenderer = qmlobject_cast<QSSGSceneRenderer *>(o);
//        item = that;
//        QSSGSceneRenderer *itemSceneRenderer = that->sceneRenderer();
//        while (!itemSceneRenderer && item && item->parentItem()) {
//            item = item->parentItem();
//            itemSceneRenderer = item->sceneRenderer();
//        }

//        if (thisSceneRenderer) {
//            if (itemSceneRenderer) {
//                // qCDebug(lcTransient) << thisWindow << "is transient for" << itemWindow;
//                thisSceneRenderer->setTransientParent(itemSceneRenderer);
//            } else {
//                QObject::connect(item, SIGNAL(sceneRendererChanged(QSSGSceneRenderer *)), thisSceneRenderer, SLOT(setTransientParent_helper(QSSGSceneRenderer *)));
//            }
//        }

        QQuickItem *quickItem = qobject_cast<QQuickItem *>(o);
        if (quickItem) {
            QQuick3DItem2D *item2d = new QQuick3DItem2D(quickItem);
            item2d->setParent(that);
            item2d->setParentItem(that);
        } else {
            o->setParent(that);
        }
    }
    resources_append(prop, o);
}

int QQuick3DObjectPrivate::data_count(QQmlListProperty<QObject> *property)
{
    QQuick3DObject *item = static_cast<QQuick3DObject *>(property->object);
    QQuick3DObjectPrivate *privateItem = QQuick3DObjectPrivate::get(item);
    QQmlListProperty<QObject> resourcesProperty = privateItem->resources();
    QQmlListProperty<QQuick3DObject> childrenProperty = privateItem->children();

    return resources_count(&resourcesProperty) + children_count(&childrenProperty);
}

QObject *QQuick3DObjectPrivate::data_at(QQmlListProperty<QObject> *property, int i)
{
    QQuick3DObject *item = static_cast<QQuick3DObject *>(property->object);
    QQuick3DObjectPrivate *privateItem = QQuick3DObjectPrivate::get(item);
    QQmlListProperty<QObject> resourcesProperty = privateItem->resources();
    QQmlListProperty<QQuick3DObject> childrenProperty = privateItem->children();

    int resourcesCount = resources_count(&resourcesProperty);
    if (i < resourcesCount)
        return resources_at(&resourcesProperty, i);
    const int j = i - resourcesCount;
    if (j < children_count(&childrenProperty))
        return children_at(&childrenProperty, j);
    return nullptr;
}

void QQuick3DObjectPrivate::data_clear(QQmlListProperty<QObject> *property)
{
    QQuick3DObject *item = static_cast<QQuick3DObject *>(property->object);
    QQuick3DObjectPrivate *privateItem = QQuick3DObjectPrivate::get(item);
    QQmlListProperty<QObject> resourcesProperty = privateItem->resources();
    QQmlListProperty<QQuick3DObject> childrenProperty = privateItem->children();

    resources_clear(&resourcesProperty);
    children_clear(&childrenProperty);
}

QObject *QQuick3DObjectPrivate::resources_at(QQmlListProperty<QObject> *prop, int index)
{
    QQuick3DObjectPrivate *quickItemPrivate = QQuick3DObjectPrivate::get(static_cast<QQuick3DObject *>(prop->object));
    return quickItemPrivate->extra.isAllocated() ? quickItemPrivate->extra->resourcesList.value(index) : 0;
}

void QQuick3DObjectPrivate::resources_append(QQmlListProperty<QObject> *prop, QObject *object)
{
    QQuick3DObject *quickItem = static_cast<QQuick3DObject *>(prop->object);
    QQuick3DObjectPrivate *quickItemPrivate = QQuick3DObjectPrivate::get(quickItem);
    if (!quickItemPrivate->extra.value().resourcesList.contains(object)) {
        quickItemPrivate->extra.value().resourcesList.append(object);
        // clang-format off
        qmlobject_connect(object, QObject, SIGNAL(destroyed(QObject*)),
                          quickItem, QQuick3DObject, SLOT(_q_resourceObjectDeleted(QObject*)));
        // clang-format on
    }
}

int QQuick3DObjectPrivate::resources_count(QQmlListProperty<QObject> *prop)
{
    QQuick3DObjectPrivate *quickItemPrivate = QQuick3DObjectPrivate::get(static_cast<QQuick3DObject *>(prop->object));
    return quickItemPrivate->extra.isAllocated() ? quickItemPrivate->extra->resourcesList.count() : 0;
}

void QQuick3DObjectPrivate::resources_clear(QQmlListProperty<QObject> *prop)
{
    QQuick3DObject *quickItem = static_cast<QQuick3DObject *>(prop->object);
    QQuick3DObjectPrivate *quickItemPrivate = QQuick3DObjectPrivate::get(quickItem);
    if (quickItemPrivate->extra.isAllocated()) { // If extra is not allocated resources is empty.
        for (QObject *object : qAsConst(quickItemPrivate->extra->resourcesList)) {
            // clang-format off
            qmlobject_disconnect(object, QObject, SIGNAL(destroyed(QObject*)),
                                 quickItem, QQuick3DObject, SLOT(_q_resourceObjectDeleted(QObject*)));
            // clang-format on
        }
        quickItemPrivate->extra->resourcesList.clear();
    }
}

void QQuick3DObjectPrivate::children_append(QQmlListProperty<QQuick3DObject> *prop, QQuick3DObject *o)
{
    if (!o)
        return;

    QQuick3DObject *that = static_cast<QQuick3DObject *>(prop->object);
    if (o->parentItem() == that)
        o->setParentItem(nullptr);

    o->setParentItem(that);
}

int QQuick3DObjectPrivate::children_count(QQmlListProperty<QQuick3DObject> *prop)
{
    QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(static_cast<QQuick3DObject *>(prop->object));
    return p->childItems.count();
}

QQuick3DObject *QQuick3DObjectPrivate::children_at(QQmlListProperty<QQuick3DObject> *prop, int index)
{
    QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(static_cast<QQuick3DObject *>(prop->object));
    if (index >= p->childItems.count() || index < 0)
        return nullptr;

    return p->childItems.at(index);
}

void QQuick3DObjectPrivate::children_clear(QQmlListProperty<QQuick3DObject> *prop)
{
    QQuick3DObject *that = static_cast<QQuick3DObject *>(prop->object);
    QQuick3DObjectPrivate *p = QQuick3DObjectPrivate::get(that);
    while (!p->childItems.isEmpty())
        p->childItems.at(0)->setParentItem(nullptr);
}

void QQuick3DObjectPrivate::_q_resourceObjectDeleted(QObject *object)
{
    if (extra.isAllocated() && extra->resourcesList.contains(object))
        extra->resourcesList.removeAll(object);
}

void QQuick3DObjectPrivate::addItemChangeListener(QQuick3DObjectChangeListener *listener, ChangeTypes types)
{
    changeListeners.append(ChangeListener(listener, types));
}

void QQuick3DObjectPrivate::updateOrAddItemChangeListener(QQuick3DObjectChangeListener *listener, ChangeTypes types)
{
    const ChangeListener changeListener(listener, types);
    const int index = changeListeners.indexOf(changeListener);
    if (index > -1)
        changeListeners[index].types = changeListener.types;
    else
        changeListeners.append(changeListener);
}

void QQuick3DObjectPrivate::removeItemChangeListener(QQuick3DObjectChangeListener *listener, ChangeTypes types)
{
    ChangeListener change(listener, types);
    changeListeners.removeOne(change);
}

QQuickStateGroup *QQuick3DObjectPrivate::_states()
{
    Q_Q(QQuick3DObject);
    if (!_stateGroup) {
        _stateGroup = new QQuickStateGroup;
        if (!componentComplete)
            _stateGroup->classBegin();
        // clang-format off
        qmlobject_connect(_stateGroup, QQuickStateGroup, SIGNAL(stateChanged(QString)),
                          q, QQuick3DObject, SIGNAL(stateChanged()));
        // clang-format on
    }

    return _stateGroup;
}

QString QQuick3DObjectPrivate::dirtyToString() const
{
#define DIRTY_TO_STRING(value)                                                                                         \
    if (dirtyAttributes & value) {                                                                                     \
        if (!rv.isEmpty())                                                                                             \
            rv.append(QLatin1Char('|'));                                                                               \
        rv.append(QLatin1String(#value));                                                                              \
    }

    //    QString rv = QLatin1String("0x") + QString::number(dirtyAttributes, 16);
    QString rv;

    DIRTY_TO_STRING(TransformOrigin);
    DIRTY_TO_STRING(Transform);
    DIRTY_TO_STRING(BasicTransform);
    DIRTY_TO_STRING(Position);
    DIRTY_TO_STRING(Size);
    DIRTY_TO_STRING(ZValue);
    DIRTY_TO_STRING(Content);
    DIRTY_TO_STRING(Smooth);
    DIRTY_TO_STRING(OpacityValue);
    DIRTY_TO_STRING(ChildrenChanged);
    DIRTY_TO_STRING(ChildrenStackingChanged);
    DIRTY_TO_STRING(ParentChanged);
    DIRTY_TO_STRING(Clip);
    DIRTY_TO_STRING(Window);
    DIRTY_TO_STRING(EffectReference);
    DIRTY_TO_STRING(Visible);
    DIRTY_TO_STRING(HideReference);
    DIRTY_TO_STRING(Antialiasing);

    return rv;
}

void QQuick3DObjectPrivate::dirty(QQuick3DObjectPrivate::DirtyType type)
{
    Q_Q(QQuick3DObject);
    if (!(dirtyAttributes & type) || (sceneManager && !prevDirtyItem)) {
        dirtyAttributes |= type;
        if (sceneManager && componentComplete) {
            addToDirtyList();
            sceneManager->dirtyItem(q);
        }
    }
}

void QQuick3DObjectPrivate::addToDirtyList()
{
    Q_Q(QQuick3DObject);

    Q_ASSERT(sceneManager);
    if (!prevDirtyItem) {
        Q_ASSERT(!nextDirtyItem);

        if (isResourceNode()) {
            if (type == Type::Image) {
                // Will likely need to refactor this, but images need to come before other
                // resources
                nextDirtyItem = sceneManager->dirtyImageList;
                if (nextDirtyItem)
                    QQuick3DObjectPrivate::get(nextDirtyItem)->prevDirtyItem = &nextDirtyItem;
                prevDirtyItem = &sceneManager->dirtyImageList;
                sceneManager->dirtyImageList = q;
            } else {
                nextDirtyItem = sceneManager->dirtyResourceList;
                if (nextDirtyItem)
                    QQuick3DObjectPrivate::get(nextDirtyItem)->prevDirtyItem = &nextDirtyItem;
                prevDirtyItem = &sceneManager->dirtyResourceList;
                sceneManager->dirtyResourceList = q;
            }
        } else {
            if (type == Type::Light) {
                // needed for scoped lights second pass
               sceneManager->dirtyLightList.append(q);
            }

            nextDirtyItem = sceneManager->dirtySpatialNodeList;
            if (nextDirtyItem)
                QQuick3DObjectPrivate::get(nextDirtyItem)->prevDirtyItem = &nextDirtyItem;
            prevDirtyItem = &sceneManager->dirtySpatialNodeList;
            sceneManager->dirtySpatialNodeList = q;
        }

        sceneManager->dirtyItem(q);
    }
    Q_ASSERT(prevDirtyItem);
}

void QQuick3DObjectPrivate::removeFromDirtyList()
{
    if (prevDirtyItem) {
        if (nextDirtyItem)
            QQuick3DObjectPrivate::get(nextDirtyItem)->prevDirtyItem = prevDirtyItem;
        *prevDirtyItem = nextDirtyItem;
        prevDirtyItem = nullptr;
        nextDirtyItem = nullptr;
    }
    Q_ASSERT(!prevDirtyItem);
    Q_ASSERT(!nextDirtyItem);
}

bool QQuick3DObjectPrivate::isResourceNode() const
{
    switch (type) {
    case Type::Layer:
    case Type::Node:
    case Type::Light:
    case Type::Camera:
    case Type::Model:
    case Type::Text:
    case Type::Item2D:
        return false;
    case Type::SceneEnvironment:
    case Type::DefaultMaterial:
    case Type::PrincipledMaterial:
    case Type::Image:
    case Type::CustomMaterial:
    case Type::Lightmaps:
    case Type::Geometry:
        return true;
    default:
        return false;
    }
}

bool QQuick3DObjectPrivate::isSpatialNode() const
{
    switch (type) {
    case Type::Layer:
    case Type::Node:
    case Type::Light:
    case Type::Camera:
    case Type::Model:
    case Type::Text:
        return true;
    case Type::SceneEnvironment:
    case Type::DefaultMaterial:
    case Type::PrincipledMaterial:
    case Type::Image:
    case Type::CustomMaterial:
    case Type::Lightmaps:
    case Type::Geometry:
    default:
        return false;
    }
}

void QQuick3DObjectPrivate::setCulled(bool cull)
{
    if (cull == culled)
        return;

    culled = cull;
    if ((cull && ++extra.value().hideRefCount == 1) || (!cull && --extra.value().hideRefCount == 0))
        dirty(HideReference);
}

QList<QQuick3DObject *> QQuick3DObjectPrivate::paintOrderChildItems() const
{
    if (sortedChildItems)
        return *sortedChildItems;

    sortedChildItems = const_cast<QList<QQuick3DObject *> *>(&childItems);

    return childItems;
}

void QQuick3DObjectPrivate::addChild(QQuick3DObject *child)
{
    Q_Q(QQuick3DObject);

    Q_ASSERT(!childItems.contains(child));

    childItems.append(child);

    markSortedChildrenDirty(child);
    dirty(QQuick3DObjectPrivate::ChildrenChanged);

    itemChange(QQuick3DObject::ItemChildAddedChange, child);

    emit q->childrenChanged();
}

void QQuick3DObjectPrivate::removeChild(QQuick3DObject *child)
{
    Q_Q(QQuick3DObject);

    Q_ASSERT(child);
    Q_ASSERT(childItems.contains(child));
    childItems.removeOne(child);
    Q_ASSERT(!childItems.contains(child));

    markSortedChildrenDirty(child);
    dirty(QQuick3DObjectPrivate::ChildrenChanged);

    itemChange(QQuick3DObject::ItemChildRemovedChange, child);

    emit q->childrenChanged();
}

void QQuick3DObjectPrivate::siblingOrderChanged()
{
    Q_Q(QQuick3DObject);
    if (!changeListeners.isEmpty()) {
        const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
        for (const QQuick3DObjectPrivate::ChangeListener &change : listeners) {
            if (change.types & QQuick3DObjectPrivate::SiblingOrder) {
                change.listener->itemSiblingOrderChanged(q);
            }
        }
    }
}

void QQuick3DObjectPrivate::markSortedChildrenDirty(QQuick3DObject *child)
{
    Q_UNUSED(child);
}

void QQuick3DObjectPrivate::refSceneManager(const QSharedPointer<QQuick3DSceneManager> &c)
{
    // An item needs a window if it is referenced by another item which has a window.
    // Typically the item is referenced by a parent, but can also be referenced by a
    // ShaderEffect or ShaderEffectSource. 'windowRefCount' counts how many items with
    // a window is referencing this item. When the reference count goes from zero to one,
    // or one to zero, the window of this item is updated and propagated to the children.
    // As long as the reference count stays above zero, the window is unchanged.
    // refWindow() increments the reference count.
    // derefWindow() decrements the reference count.

    Q_Q(QQuick3DObject);
    Q_ASSERT((sceneManager != nullptr) == (windowRefCount > 0));
    Q_ASSERT(c);
    if (++windowRefCount > 1) {
        if (c != sceneManager)
            qWarning("QSSGObject: Cannot use same item on different windows at the same time.");
        return; // Window already set.
    }

    Q_ASSERT(sceneManager == nullptr);
    sceneManager = c;

    //    if (polishScheduled)
    //        QSSGWindowPrivate::get(window)->itemsToPolish.append(q);

    if (!parentItem)
        sceneManager->parentlessItems.insert(q);

    for (int ii = 0; ii < childItems.count(); ++ii) {
        QQuick3DObject *child = childItems.at(ii);
        QQuick3DObjectPrivate::refSceneManager(child, c);
    }

    dirty(Window);

    itemChange(QQuick3DObject::ItemSceneChange, c);
}

void QQuick3DObjectPrivate::derefSceneManager()
{
    Q_Q(QQuick3DObject);

    if (!sceneManager)
        return; // This can happen when destroying recursive shader effect sources.

    if (--windowRefCount > 0)
        return; // There are still other references, so don't set window to null yet.

    removeFromDirtyList();
    if (sceneManager) {
        sceneManager->dirtyBoundingBoxList.removeAll(q);
        sceneManager->dirtyLightList.removeAll(q);
    }

    if (spatialNode)
        sceneManager->cleanup(spatialNode);
    if (!parentItem)
        sceneManager->parentlessItems.remove(q);

    sceneManager.reset();

    spatialNode = nullptr;

    for (int ii = 0; ii < childItems.count(); ++ii) {
        QQuick3DObject *child = childItems.at(ii);
        QQuick3DObjectPrivate::derefSceneManager(child);
    }

    dirty(Window);

    itemChange(QQuick3DObject::ItemSceneChange, sceneManager);
}

void QQuick3DObjectPrivate::updateSubFocusItem(QQuick3DObject *scope, bool focus)
{
    Q_Q(QQuick3DObject);
    Q_ASSERT(scope);

    QQuick3DObjectPrivate *scopePrivate = QQuick3DObjectPrivate::get(scope);

    QQuick3DObject *oldSubFocusItem = scopePrivate->subFocusItem;
    // Correct focus chain in scope
    if (oldSubFocusItem) {
        QQuick3DObject *sfi = scopePrivate->subFocusItem->parentItem();
        while (sfi && sfi != scope) {
            QQuick3DObjectPrivate::get(sfi)->subFocusItem = nullptr;
            sfi = sfi->parentItem();
        }
    }

    if (focus) {
        scopePrivate->subFocusItem = q;
        QQuick3DObject *sfi = scopePrivate->subFocusItem->parentItem();
        while (sfi && sfi != scope) {
            QQuick3DObjectPrivate::get(sfi)->subFocusItem = q;
            sfi = sfi->parentItem();
        }
    } else {
        scopePrivate->subFocusItem = nullptr;
    }
}

void QQuick3DObjectPrivate::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &data)
{
    Q_Q(QQuick3DObject);
    switch (change) {
    case QQuick3DObject::ItemRotationHasChanged:
        // TODO:
        qWarning("ItemRoationHasChange is unhandled!!!!");
        break;
    case QQuick3DObject::ItemChildAddedChange: {
        q->itemChange(change, data);
        if (!changeListeners.isEmpty()) {
            const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
            for (const QQuick3DObjectPrivate::ChangeListener &change : listeners) {
                if (change.types & QQuick3DObjectPrivate::Children) {
                    change.listener->itemChildAdded(q, data.item);
                }
            }
        }
        break;
    }
    case QQuick3DObject::ItemChildRemovedChange: {
        q->itemChange(change, data);
        if (!changeListeners.isEmpty()) {
            const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
            for (const QQuick3DObjectPrivate::ChangeListener &change : listeners) {
                if (change.types & QQuick3DObjectPrivate::Children) {
                    change.listener->itemChildRemoved(q, data.item);
                }
            }
        }
        break;
    }
    case QQuick3DObject::ItemSceneChange:
        q->itemChange(change, data);
        break;
    case QQuick3DObject::ItemVisibleHasChanged: {
        q->itemChange(change, data);
        if (!changeListeners.isEmpty()) {
            const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
            for (const QQuick3DObjectPrivate::ChangeListener &change : listeners) {
                if (change.types & QQuick3DObjectPrivate::Visibility) {
                    change.listener->itemVisibilityChanged(q);
                }
            }
        }
        break;
    }
    case QQuick3DObject::ItemEnabledHasChanged: {
        q->itemChange(change, data);
        if (!changeListeners.isEmpty()) {
            const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
            for (const QQuick3DObjectPrivate::ChangeListener &change : listeners) {
                if (change.types & QQuick3DObjectPrivate::Enabled) {
                    change.listener->itemEnabledChanged(q);
                }
            }
        }
        break;
    }
    case QQuick3DObject::ItemParentHasChanged: {
        q->itemChange(change, data);
        if (!changeListeners.isEmpty()) {
            const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
            for (const QQuick3DObjectPrivate::ChangeListener &change : listeners) {
                if (change.types & QQuick3DObjectPrivate::Parent) {
                    change.listener->itemParentChanged(q, data.item);
                }
            }
        }
        break;
    }
    case QQuick3DObject::ItemOpacityHasChanged: {
        q->itemChange(change, data);
        if (!changeListeners.isEmpty()) {
            const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
            for (const QQuick3DObjectPrivate::ChangeListener &change : listeners) {
                if (change.types & QQuick3DObjectPrivate::Opacity) {
                    change.listener->itemOpacityChanged(q);
                }
            }
        }
        break;
    }
    case QQuick3DObject::ItemActiveFocusHasChanged:
        q->itemChange(change, data);
        break;
    case QQuick3DObject::ItemAntialiasingHasChanged:
        // fall through
    case QQuick3DObject::ItemDevicePixelRatioHasChanged:
        q->itemChange(change, data);
        break;
    }
}

namespace QV4 {
namespace Heap {
struct QSSGItemWrapper : public QObjectWrapper
{
    static void markObjects(QV4::Heap::Base *that, QV4::MarkStack *markStack);
};
}
}

struct QSSGItemWrapper : public QV4::QObjectWrapper
{
    V4_OBJECT2(QSSGItemWrapper, QV4::QObjectWrapper)
};

DEFINE_OBJECT_VTABLE(QSSGItemWrapper);

void QV4::Heap::QSSGItemWrapper::markObjects(QV4::Heap::Base *that, QV4::MarkStack *markStack)
{
    QObjectWrapper *This = static_cast<QObjectWrapper *>(that);
    if (QQuick3DObject *item = static_cast<QQuick3DObject *>(This->object())) {
        for (QQuick3DObject *child : qAsConst(QQuick3DObjectPrivate::get(item)->childItems))
            QV4::QObjectWrapper::markWrapper(child, markStack);
    }
    QObjectWrapper::markObjects(that, markStack);
}

quint64 QQuick3DObjectPrivate::_q_createJSWrapper(QV4::ExecutionEngine *engine)
{
    return (engine->memoryManager->allocate<QSSGItemWrapper>(q_func()))->asReturnedValue();
}

QQuick3DObjectPrivate::ExtraData::ExtraData() : hideRefCount(0) {}

QT_END_NAMESPACE

#include "moc_qquick3dobject.cpp"
