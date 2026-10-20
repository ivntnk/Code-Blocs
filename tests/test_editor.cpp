// Scène de blocs : ajout, connexion, sauvegarde JSON (editor.h) sur les exemples intégrés.
#include <QtTest>

#include "editor.h"
#include "examples.h"

namespace {
int connectionCount(const NodeScene& scene) {
    int n = 0;
    for (const QGraphicsItem* it : scene.items())
        if (qgraphicsitem_cast<const ConnectionItem*>(it)) ++n;
    return n;
}
}  // namespace

class TestEditor : public QObject {
    Q_OBJECT
private slots:
    void unknownBlockIsRejected() {
        NodeScene scene;
        QVERIFY(scene.addNode(QStringLiteral("ce-bloc-n-existe-pas"), {0, 0}) == nullptr);
        QCOMPARE(scene.nodeItems().size(), 0);
    }

    void addConnectAndClear() {
        NodeScene scene;
        NodeItem* start = scene.addNode(QStringLiteral("start"), {0, 0}, false);
        NodeItem* print = scene.addNode(QStringLiteral("print"), {200, 0}, false);
        QVERIFY(start != nullptr);
        QVERIFY(print != nullptr);
        QCOMPARE(scene.nodeItems().size(), 2);
        QVERIFY(scene.connectNodes(start, QStringLiteral("next"), print, QStringLiteral("in")));
        QCOMPARE(connectionCount(scene), 1);
        scene.clearAll();
        QCOMPARE(scene.nodeItems().size(), 0);
    }

    void jsonRoundTripOnEveryExample_data() {
        QTest::addColumn<int>("id");
        for (const ExampleInfo& e : examples())
            QTest::newRow(QString::fromUtf8(e.name).toUtf8().constData()) << e.id;
    }
    void jsonRoundTripOnEveryExample() {
        QFETCH(int, id);
        NodeScene original;
        buildExample(original, id);
        QVERIFY(!original.nodeItems().isEmpty());

        const QJsonObject json = original.toJson();
        NodeScene copy;
        QVERIFY(copy.fromJson(json));

        QCOMPARE(copy.nodeItems().size(), original.nodeItems().size());
        QCOMPARE(connectionCount(copy), connectionCount(original));
        QCOMPARE(copy.assets().size(), original.assets().size());
    }
};

QTEST_MAIN(TestEditor)
#include "test_editor.moc"
