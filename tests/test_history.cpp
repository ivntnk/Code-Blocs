// Tests de l'historique d'annulation (history.h).
#include <QtTest>

#include "history.h"

class TestHistory : public QObject {
    Q_OBJECT
private slots:
    void startsEmpty() {
        History h;
        QVERIFY(!h.canUndo());
        QVERIFY(!h.canRedo());
    }

    void resetKeepsSingleState() {
        History h;
        h.reset("a");
        QVERIFY(!h.canUndo());
        QVERIFY(!h.canRedo());
    }

    void identicalStateIsIgnored() {
        History h;
        h.reset("a");
        QVERIFY(!h.push("a"));
        QVERIFY(!h.canUndo());
    }

    void undoThenRedo() {
        History h;
        h.reset("a");
        QVERIFY(h.push("b"));
        QVERIFY(h.canUndo());
        QCOMPARE(h.undo(), QByteArray("a"));
        QVERIFY(h.canRedo());
        QCOMPARE(h.redo(), QByteArray("b"));
        QVERIFY(!h.canRedo());
    }

    void pushAfterUndoDropsTheFuture() {
        History h;
        h.reset("a");
        h.push("b");
        h.push("c");
        QCOMPARE(h.undo(), QByteArray("b"));
        QVERIFY(h.push("d"));
        QVERIFY(!h.canRedo());
        QCOMPARE(h.undo(), QByteArray("b"));
        QCOMPARE(h.undo(), QByteArray("a"));
        QVERIFY(!h.canUndo());
    }

    void historyIsBounded() {
        History h;
        h.reset("0");
        for (int i = 1; i <= 500; ++i) h.push(QByteArray::number(i));
        int steps = 0;
        while (h.canUndo()) {
            h.undo();
            ++steps;
        }
        QVERIFY2(steps < 500, "l'historique doit être borné");
        QVERIFY(steps > 0);
    }
};

QTEST_APPLESS_MAIN(TestHistory)
#include "test_history.moc"
