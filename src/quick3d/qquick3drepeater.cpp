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

#include "qquick3drepeater_p.h"

#include <private/qqmlglobal_p.h>
#include <private/qqmllistaccessor_p.h>
#include <private/qqmlchangeset_p.h>
#include <private/qqmldelegatemodel_p.h>

#include <QtQml/QQmlInfo>

QT_BEGIN_NAMESPACE


/*!
    \qmltype Repeater3D
    \inqmlmodule QtQuick3D
    \inherits Node
    \brief Instantiates a number of Node-based components using a provided model.

    The Repeater3D type is used to create a large number of
    similar items. Like other view types, a Repeater3D has a \l model and a \l delegate:
    for each entry in the model, the delegate is instantiated
    in a context seeded with data from the model.

    A Repeater's \l model can be any of the supported \l {qml-data-models}{data models}.
    Additionally, like delegates for other views, a Repeater delegate can access
    its index within the repeater, as well as the model data relevant to the
    delegate. See the \l delegate property documentation for details.

    \note A Repeater3D item owns all items it instantiates. Removing or dynamically destroying
    an item created by a Repeater3D results in unpredictable behavior.

    \note Repeater3D is \l {Node}-based, and can only repeat \l {Node}-derived objects.
 */

/*!
    \qmlsignal QtQuick3D::Repeater3D::objectAdded(int index, Object3D object)

    This signal is emitted when an object is added to the repeater. The \a index
    parameter holds the index at which object has been inserted within the
    repeater, and the \a object parameter holds the \l Object3D that has been added.

    The corresponding handler is \c onObjectAdded.
*/

/*!
    \qmlsignal QtQuick3D::Repeater3D::objectRemoved(int index, Object3D object)

    This signal is emitted when an object is removed from the repeater. The \a index
    parameter holds the index at which the item was removed from the repeater,
    and the \a object parameter holds the \l Object3D that was removed.

    Do not keep a reference to \a object if it was created by this repeater, as
    in these cases it will be deleted shortly after the signal is handled.

    The corresponding handler is \c onObjectRemoved.
*/

QQuick3DRepeater::QQuick3DRepeater(QQuick3DNode *parent)
    : QQuick3DNode(parent)
    , m_model(nullptr)
    , m_itemCount(0)
    , m_ownModel(false)
    , m_dataSourceIsObject(false)
    , m_delegateValidated(false)
{
}

QQuick3DRepeater::~QQuick3DRepeater()
{
    if (m_ownModel)
        delete m_model;
}

/*!
    \qmlproperty any QtQuick3D::Repeater3D::model

    The model providing data for the repeater.

    This property can be set to any of the supported \l {qml-data-models}{data models}:

    \list
    \li A number that indicates the number of delegates to be created by the repeater
    \li A model (e.g. a ListModel item, or a QAbstractItemModel subclass)
    \li A string list
    \li An object list
    \endlist

    The type of model affects the properties that are exposed to the \l delegate.

    \sa {qml-data-models}{Data Models}
*/

QVariant QQuick3DRepeater::model() const
{
    if (m_dataSourceIsObject) {
        QObject *o = m_dataSourceAsObject;
        return QVariant::fromValue(o);
    }

    return m_dataSource;

}

void QQuick3DRepeater::setModel(const QVariant &m)
{
    QVariant model = m;
    if (model.userType() == qMetaTypeId<QJSValue>())
        model = model.value<QJSValue>().toVariant();

    if (m_dataSource == model)
        return;

    clear();
    if (m_model) {
        qmlobject_disconnect(m_model, QQmlInstanceModel, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                this, QQuick3DRepeater, SLOT(modelUpdated(QQmlChangeSet,bool)));
        qmlobject_disconnect(m_model, QQmlInstanceModel, SIGNAL(createdItem(int,QObject*)),
                this, QQuick3DRepeater, SLOT(createdObject(int,QObject*)));
        qmlobject_disconnect(m_model, QQmlInstanceModel, SIGNAL(initItem(int,QObject*)),
                this, QQuick3DRepeater, SLOT(initObject(int,QObject*)));
    }
    m_dataSource = model;
    QObject *object = qvariant_cast<QObject*>(model);
    m_dataSourceAsObject = object;
    m_dataSourceIsObject = object != nullptr;
    QQmlInstanceModel *vim = nullptr;
    if (object && (vim = qobject_cast<QQmlInstanceModel *>(object))) {
        if (m_ownModel) {
            delete m_model;
            m_ownModel = false;
        }
        m_model = vim;
    } else {
        if (!m_ownModel) {
            m_model = new QQmlDelegateModel(qmlContext(this));
            m_ownModel = true;
            if (isComponentComplete())
                static_cast<QQmlDelegateModel *>(m_model.data())->componentComplete();
        }
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(m_model))
            dataModel->setModel(model);
    }
    if (m_model) {
        qmlobject_connect(m_model, QQmlInstanceModel, SIGNAL(modelUpdated(QQmlChangeSet,bool)),
                this, QQuick3DRepeater, SLOT(modelUpdated(QQmlChangeSet,bool)));
        qmlobject_connect(m_model, QQmlInstanceModel, SIGNAL(createdItem(int,QObject*)),
                this, QQuick3DRepeater, SLOT(createdObject(int,QObject*)));
        qmlobject_connect(m_model, QQmlInstanceModel, SIGNAL(initItem(int,QObject*)),
                this, QQuick3DRepeater, SLOT(initObject(int,QObject*)));
        regenerate();
    }
    emit modelChanged();
    emit countChanged();
}

/*!
    \qmlproperty Component QtQuick3D::Repeater3D::delegate
    \default

    The delegate provides a template defining each object instantiated by the repeater.

    Delegates are exposed to a read-only \c index property that indicates the index
    of the delegate within the repeater.

    If the \l model is a model object (such as a \l ListModel) the delegate
    can access all model roles as named properties, in the same way that delegates
    do for view classes like ListView.

    \sa {QML Data Models}
 */

QQmlComponent *QQuick3DRepeater::delegate() const
{
    if (m_model) {
        if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(m_model))
            return dataModel->delegate();
    }

    return nullptr;
}

void QQuick3DRepeater::setDelegate(QQmlComponent *delegate)
{
    if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(m_model))
       if (delegate == dataModel->delegate())
           return;

    if (!m_ownModel) {
        m_model = new QQmlDelegateModel(qmlContext(this));
        m_ownModel = true;
    }

    if (QQmlDelegateModel *dataModel = qobject_cast<QQmlDelegateModel*>(m_model)) {
        dataModel->setDelegate(delegate);
        regenerate();
        emit delegateChanged();
        m_delegateValidated = false;
    }
}

/*!
    \qmlproperty int QtQuick3D::Repeater3D::count

    This property holds the number of items in the model.

    \note The number of items in the model as reported by count may differ from
    the number of created delegates if the Repeater3D is in the process of
    instantiating delegates or is incorrectly set up.
*/

int QQuick3DRepeater::count() const
{
    if (m_model)
        return m_model->count();
    return 0;
}

/*!
    \qmlmethod Object3D QtQuick3D::Repeater3D::objectAt(index)

    Returns the \l Object3D that has been created at the given \a index, or \c null
    if no item exists at \a index.
*/

QQuick3DObject *QQuick3DRepeater::objectAt(int index) const
{
    if (index >= 0 && index < m_deletables.count())
        return m_deletables[index];
    return nullptr;
}

void QQuick3DRepeater::clear()
{
    bool complete = isComponentComplete();

    if (m_model) {
        // We remove in reverse order deliberately; so that signals are emitted
        // with sensible indices.
        for (int i = m_deletables.count() - 1; i >= 0; --i) {
            if (QQuick3DObject *item = m_deletables.at(i)) {
                if (complete)
                    emit objectRemoved(i, item);
                m_model->release(item);
            }
        }
        for (QQuick3DObject *item : qAsConst(m_deletables)) {
            if (item)
                item->setParentItem(nullptr);
        }
    }
    m_deletables.clear();
    m_itemCount = 0;
}

void QQuick3DRepeater::regenerate()
{
    if (!isComponentComplete())
        return;

    clear();

    if (!m_model || !m_model->count() || !m_model->isValid() || !parentItem() || !isComponentComplete())
        return;

    m_itemCount = count();
    m_deletables.resize(m_itemCount);
    requestItems();
}

void QQuick3DRepeater::componentComplete()
{
    if (m_model && m_ownModel)
        static_cast<QQmlDelegateModel *>(m_model.data())->componentComplete();
    QQuick3DObject::componentComplete();
    regenerate();
    if (m_model && m_model->count())
        emit countChanged();
}

void QQuick3DRepeater::itemChange(QQuick3DObject::ItemChange change, const QQuick3DObject::ItemChangeData &value)
{
    QQuick3DObject::itemChange(change, value);
    if (change == ItemParentHasChanged) {
        regenerate();
    }
}

void QQuick3DRepeater::createdObject(int index, QObject *)
{
    QObject *object = m_model->object(index, QQmlIncubator::AsynchronousIfNested);
    QQuick3DObject *item = qmlobject_cast<QQuick3DObject*>(object);
    emit objectAdded(index, item);
}

void QQuick3DRepeater::initObject(int index, QObject *object)
{
    QQuick3DNode *item = qmlobject_cast<QQuick3DNode*>(object);

    if (!m_deletables.at(index)) {
        if (!item) {
            if (object) {
                m_model->release(object);
                if (!m_delegateValidated) {
                    m_delegateValidated = true;
                    QObject* delegate = this->delegate();
                    qmlWarning(delegate ? delegate : this) << QQuick3DRepeater::tr("Delegate must be of Node type");
                }
            }
            return;
        }
        m_deletables[index] = item;
        item->setParent(this);
        item->setParentItem(static_cast<QQuick3DNode*>(this));
    }
}

void QQuick3DRepeater::modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    if (!isComponentComplete())
        return;

    if (reset) {
        regenerate();
        if (changeSet.difference() != 0)
            emit countChanged();
        return;
    }

    int difference = 0;
    QHash<int, QVector<QPointer<QQuick3DNode> > > moved;
    for (const QQmlChangeSet::Change &remove : changeSet.removes()) {
        int index = qMin(remove.index, m_deletables.count());
        int count = qMin(remove.index + remove.count, m_deletables.count()) - index;
        if (remove.isMove()) {
            moved.insert(remove.moveId, m_deletables.mid(index, count));
            m_deletables.erase(
                    m_deletables.begin() + index,
                    m_deletables.begin() + index + count);
        } else while (count--) {
            QQuick3DNode *item = m_deletables.at(index);
            m_deletables.remove(index);
            emit objectRemoved(index, item);
            if (item) {
                m_model->release(item);
                item->setParentItem(nullptr);
            }
            --m_itemCount;
        }

        difference -= remove.count;
    }

    for (const QQmlChangeSet::Change &insert : changeSet.inserts()) {
        int index = qMin(insert.index, m_deletables.count());
        if (insert.isMove()) {
            QVector<QPointer<QQuick3DNode> > items = moved.value(insert.moveId);
            m_deletables = m_deletables.mid(0, index) + items + m_deletables.mid(index);
        } else for (int i = 0; i < insert.count; ++i) {
            int modelIndex = index + i;
            ++m_itemCount;
            m_deletables.insert(modelIndex, nullptr);
            QObject *object = m_model->object(modelIndex, QQmlIncubator::AsynchronousIfNested);
            if (object)
                m_model->release(object);
        }
        difference += insert.count;
    }

    if (difference != 0)
        emit countChanged();
}

void QQuick3DRepeater::requestItems()
{
    for (int i = 0; i < m_itemCount; i++) {
        QObject *object = m_model->object(i, QQmlIncubator::AsynchronousIfNested);
        if (object)
            m_model->release(object);
    }
}

QT_END_NAMESPACE

#include "moc_qquick3drepeater_p.cpp"
