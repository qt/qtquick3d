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

#include <QtTest>

#include <QtQuick3DUtils/private/qssginvasivelinkedlist_p.h>

struct NodeItem
{
    NodeItem() = default;
    NodeItem(int v, NodeItem *p, NodeItem *n) : value(v), prev(p), next(n) {}
    int value = -1;
    NodeItem *prev = nullptr;
    NodeItem *next = nullptr;
};

using SingleLinkedList = QSSGInvasiveSingleLinkedList<NodeItem, &NodeItem::next>;
using LinkedList = QSSGInvasiveLinkedList<NodeItem, &NodeItem::prev, &NodeItem::next>;

class invasivelist : public QObject
{
    Q_OBJECT

public:
    invasivelist() = default;
    ~invasivelist() = default;

private slots:
    void test_empty();
    void test_push_backSll();
    void test_push_frontSll();
    void test_push_backLl();
    void test_push_frontLl();
    void test_removeSll();
    void test_removeLl();
    void test_insert_unsafe();
    void test_insert_before();
    void test_insert_after();
    void test_copy_slList();
    void test_copy_lList();
    void test_iteratorSll();
    void test_iteratorLl();
};

void invasivelist::test_empty()
{
    SingleLinkedList sll;
    QVERIFY(sll.empty());
    LinkedList ll;
    QVERIFY(ll.empty());
    NodeItem node;
    QCOMPARE(node.next, nullptr);
    QCOMPARE(node.prev, nullptr);

    sll.push_back(node);
    QVERIFY(!sll.empty());
    sll.remove(node);
    // list should be empty and the node "links" reset
    QVERIFY(sll.empty());
    QCOMPARE(node.next, nullptr);
    QCOMPARE(node.prev, nullptr);

    ll.push_back(node);
    QVERIFY(!ll.empty());
    ll.remove(node);
    // list should be empty and the node "links" reset
    QVERIFY(ll.empty());
    QCOMPARE(node.next, nullptr);
    QCOMPARE(node.prev, nullptr);
}

void invasivelist::test_push_backSll()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    SingleLinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_back(items[i]);
    }

    int lastValue = -1;
    int count = 0;
    for (const auto &item : qAsConst(list)) {
        QVERIFY(item.value > lastValue);
        lastValue = item.value;
        ++count;
    }

    QCOMPARE(lastValue, itemCount - 1);
    QCOMPARE(count, itemCount);
}

void invasivelist::test_push_frontSll()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    SingleLinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_front(items[i]);
    }

    int lastValue = 100;
    int count = 0;
    for (const auto &item : qAsConst(list)) {
        QVERIFY(item.value < lastValue);
        lastValue = item.value;
        ++count;
    }

    QCOMPARE(lastValue, 0);
    QCOMPARE(count, itemCount);
}

void invasivelist::test_push_backLl()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    LinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_back(items[i]);
    }

    int lastValue = -1;
    int count = 0;
    for (const auto &item : qAsConst(list)) {
        QVERIFY(item.value > lastValue);
        lastValue = item.value;
        ++count;
    }

    QCOMPARE(lastValue, itemCount - 1);
    QCOMPARE(count, itemCount);
}

void invasivelist::test_push_frontLl()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    LinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_front(items[i]);
    }

    int lastValue = 100;
    int count = 0;
    for (const auto &item : qAsConst(list)) {
        QVERIFY(item.value < lastValue);
        lastValue = item.value;
        ++count;
    }

    QCOMPARE(lastValue, 0);
    QCOMPARE(count, itemCount);
}

void invasivelist::test_removeSll()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    SingleLinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_back(items[i]);
    }

    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &items[1]);
    list.remove(items[0]);
    QCOMPARE(items[0].next, nullptr);
    QCOMPARE(items[0].prev, nullptr);

    list.push_front(items[0]);
    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &items[1]);

    // Remove all
    for (int i = 0; i != itemCount; ++i)
        list.remove(items[i]);

    QVERIFY(list.empty());

    for (int i = 0; i != itemCount; ++i) {
        QCOMPARE(items[i].next, nullptr);
        QCOMPARE(items[i].prev, nullptr);
    }
}

void invasivelist::test_removeLl()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    LinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_back(items[i]);
    }

    // Remove first
    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &items[1]);
    QCOMPARE(items[1].prev, &items[0]);
    QCOMPARE(items[1].next, &items[2]);
    list.remove(items[0]);
    QCOMPARE(items[0].next, nullptr);
    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[1].next, &items[2]);
    QCOMPARE(items[1].prev, nullptr);

    list.push_front(items[0]);
    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &items[1]);

    // Remove !first
    QCOMPARE(items[49].next, &items[50]);
    QCOMPARE(items[51].prev, &items[50]);
    list.remove(items[50]);
    QCOMPARE(items[50].next, nullptr);
    QCOMPARE(items[50].prev, nullptr);
    QCOMPARE(items[49].next, &items[51]);
    QCOMPARE(items[51].prev, &items[49]);

    list.insert_after(items[49], items[50]);

    // Remove all
    for (int i = 0; i != itemCount; ++i)
        list.remove(items[i]);

    QVERIFY(list.empty());

    for (int i = 0; i != itemCount; ++i) {
        QCOMPARE(items[i].next, nullptr);
        QCOMPARE(items[i].prev, nullptr);
    }
}

// Shared between the two, so we don't need to duplcate these
void invasivelist::test_insert_unsafe()
{
    {
        const int itemCount = 2;
        NodeItem items[itemCount] { NodeItem { 0, nullptr, nullptr },
                                    NodeItem { 2, nullptr, nullptr } };
        LinkedList list;
        list.push_back(items[0]);
        list.push_back(items[1]);

        QCOMPARE(items[0].prev, nullptr);
        QCOMPARE(items[0].next, &items[1]);
        QCOMPARE(items[1].prev, &items[0]);
        QCOMPARE(items[1].next, nullptr);

        NodeItem middle { 1, nullptr, nullptr };
        list.insert_unsafe(&items[0], &items[1], middle);
        QCOMPARE(items[0].prev, nullptr);
        QCOMPARE(items[0].next, &middle);
        QCOMPARE(middle.prev, &items[0]);
        QCOMPARE(middle.next, &items[1]);
        QCOMPARE(items[1].prev, &middle);
        QCOMPARE(items[1].next, nullptr);
    }
    {
        const int itemCount = 3;
        NodeItem items[itemCount] { NodeItem { 0, nullptr, nullptr },
                                    NodeItem { -1, nullptr, nullptr },
                                    NodeItem { 2, nullptr, nullptr } };
        LinkedList list;
        list.push_back(items[0]);
        list.push_back(items[1]);
        list.push_back(items[2]);

        QCOMPARE(items[0].prev, nullptr);
        QCOMPARE(items[0].next, &items[1]);
        QCOMPARE(items[1].prev, &items[0]);
        QCOMPARE(items[1].next, &items[2]);
        QCOMPARE(items[2].prev, &items[1]);
        QCOMPARE(items[2].next, nullptr);

        NodeItem middle { 1, nullptr, nullptr };
        list.insert_unsafe(&items[0], &items[2], middle);
        QCOMPARE(items[0].prev, nullptr);
        QCOMPARE(items[0].next, &middle);
        QCOMPARE(middle.prev, &items[0]);
        QCOMPARE(middle.next, &items[2]);
        QCOMPARE(items[2].prev, &middle);
        QCOMPARE(items[2].next, nullptr);
        // item[1] is now danling, but the tail and head is
        // still pointing to the old items
        // (this is inteded, so test that it still works)
        QCOMPARE(items[1].prev, &items[0]);
        QCOMPARE(items[1].next, &items[2]);
    }
}

void invasivelist::test_insert_before()
{
    const int itemCount = 2;
    NodeItem items[itemCount] { NodeItem { 1, nullptr, nullptr },
                                NodeItem { 4, nullptr, nullptr } };
    LinkedList list;
    list.push_back(items[0]);
    list.push_back(items[1]);

    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &items[1]);
    QCOMPARE(items[1].prev, &items[0]);
    QCOMPARE(items[1].next, nullptr);

    NodeItem newItems[] { NodeItem { 0, nullptr, nullptr },
                          NodeItem { 2, nullptr, nullptr },
                          NodeItem { 3, nullptr, nullptr }};

    list.insert_before(items[0], newItems[0]);
    QCOMPARE(newItems[0].prev, nullptr);
    QCOMPARE(newItems[0].next, &items[0]);
    QCOMPARE(items[0].prev, &newItems[0]);
    QCOMPARE(items[0].next, &items[1]);
    QCOMPARE(items[1].prev, &items[0]);
    QCOMPARE(items[1].next, nullptr);

    list.insert_before(items[1], newItems[1]);
    QCOMPARE(newItems[0].prev, nullptr);
    QCOMPARE(newItems[0].next, &items[0]);
    QCOMPARE(items[0].prev, &newItems[0]);
    QCOMPARE(items[0].next, &newItems[1]);
    QCOMPARE(newItems[1].prev, &items[0]);
    QCOMPARE(newItems[1].next, &items[1]);
    QCOMPARE(items[1].prev, &newItems[1]);
    QCOMPARE(items[1].next, nullptr);

    list.insert_before(items[1], newItems[2]);
    QCOMPARE(newItems[0].prev, nullptr);
    QCOMPARE(newItems[0].next, &items[0]);
    QCOMPARE(items[0].prev, &newItems[0]);
    QCOMPARE(items[0].next, &newItems[1]);
    QCOMPARE(newItems[1].prev, &items[0]);
    QCOMPARE(newItems[1].next, &newItems[2]);
    QCOMPARE(newItems[2].prev, &newItems[1]);
    QCOMPARE(newItems[2].next, &items[1]);
    QCOMPARE(items[1].prev, &newItems[2]);
    QCOMPARE(items[1].next, nullptr);
}

void invasivelist::test_insert_after()
{
    const int itemCount = 2;
    NodeItem items[itemCount] { NodeItem { 0, nullptr, nullptr },
                                NodeItem { 3, nullptr, nullptr } };
    LinkedList list;
    list.push_back(items[0]);
    list.push_back(items[1]);

    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &items[1]);
    QCOMPARE(items[1].prev, &items[0]);
    QCOMPARE(items[1].next, nullptr);

    NodeItem newItems[] { NodeItem { 1, nullptr, nullptr },
                          NodeItem { 2, nullptr, nullptr },
                          NodeItem { 4, nullptr, nullptr }};

    list.insert_after(items[0], newItems[0]);
    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &newItems[0]);
    QCOMPARE(newItems[0].prev, &items[0]);
    QCOMPARE(newItems[0].next, &items[1]);
    QCOMPARE(items[1].prev, &newItems[0]);
    QCOMPARE(items[1].next, nullptr);

    list.insert_after(newItems[0], newItems[1]);
    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &newItems[0]);
    QCOMPARE(newItems[0].prev, &items[0]);
    QCOMPARE(newItems[0].next, &newItems[1]);
    QCOMPARE(newItems[1].prev, &newItems[0]);
    QCOMPARE(newItems[1].next, &items[1]);
    QCOMPARE(items[1].prev, &newItems[1]);
    QCOMPARE(items[1].next, nullptr);

    list.insert_after(items[1], newItems[2]);
    QCOMPARE(items[0].prev, nullptr);
    QCOMPARE(items[0].next, &newItems[0]);
    QCOMPARE(newItems[0].prev, &items[0]);
    QCOMPARE(newItems[0].next, &newItems[1]);
    QCOMPARE(newItems[1].prev, &newItems[0]);
    QCOMPARE(newItems[1].next, &items[1]);
    QCOMPARE(items[1].prev, &newItems[1]);
    QCOMPARE(items[1].next, &newItems[2]);
    QCOMPARE(newItems[2].prev, &items[1]);
    QCOMPARE(newItems[2].next, nullptr);
}

void invasivelist::test_copy_slList()
{
    const int itemCount = 2;
    NodeItem items[itemCount] { NodeItem { 0, nullptr, nullptr },
                                NodeItem { 1, nullptr, nullptr } };

    SingleLinkedList l1, l2;
    QVERIFY(l1.empty());
    QVERIFY(l2.empty());
    l1.push_back(items[0]);
    l1.push_back(items[1]);
    QVERIFY(!l1.empty());
    QVERIFY(l2.empty());
    l2 = l1;
    QVERIFY(!l1.empty());
    QVERIFY(!l2.empty());

    for (auto it1 = l1.begin(), it2 = l2.begin(); it1 != l1.end(); ++it1, ++it2)
        QCOMPARE(it2, it1);
}

void invasivelist::test_copy_lList()
{
    const int itemCount = 2;
    NodeItem items[itemCount] { NodeItem { 0, nullptr, nullptr },
                                NodeItem { 1, nullptr, nullptr } };

    LinkedList l1, l2;
    QVERIFY(l1.empty());
    QVERIFY(l2.empty());
    l1.push_back(items[0]);
    l1.push_back(items[1]);
    QVERIFY(!l1.empty());
    QVERIFY(l2.empty());
    l2 = l1;
    QVERIFY(!l1.empty());
    QVERIFY(!l2.empty());

    for (auto it1 = l1.begin(), it2 = l2.begin(); it1 != l1.end(); ++it1, ++it2)
        QCOMPARE(it2, it1);
}

void invasivelist::test_iteratorSll()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    SingleLinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_back(items[i]);
    }

    {
        auto it = list.begin();
        const auto end = list.end();
        int lastValue = -1;
        for (; it != end; ++it) {
            QVERIFY(it->value > lastValue);
            lastValue = it->value;
        }
        QCOMPARE(lastValue, itemCount - 1);
    }
}

void invasivelist::test_iteratorLl()
{
    const int itemCount = 100;
    NodeItem items[itemCount];

    LinkedList list;
    for (int i = 0; i != itemCount; ++i) {
        items[i].value = i;
        list.push_back(items[i]);
    }

    {
        auto it = list.begin();
        const auto end = list.end();
        int lastValue = -1;
        for (; it != end; ++it) {
            QVERIFY(it->value > lastValue);
            lastValue = it->value;
        }
        QCOMPARE(lastValue, itemCount - 1);
    }

    {
        auto rit = list.rbegin();
        const auto rend = list.rend();
        int lastValue = itemCount;
        for (; rit != rend; ++rit) {
            QVERIFY(rit->value < lastValue);
            lastValue = rit->value;
        }
        QCOMPARE(lastValue, 0);
    }
}

QTEST_APPLESS_MAIN(invasivelist)

#include "tst_invasivelist.moc"
