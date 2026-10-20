// Cohérence du catalogue de blocs (graph.h / graph.cpp).
#include <QSet>
#include <QtTest>

#include "graph.h"

class TestGraph : public QObject {
    Q_OBJECT
private slots:
    void catalogIsNotEmpty() { QVERIFY(!nodeCatalog().isEmpty()); }

    void idsAreUniqueAndNotEmpty() {
        QSet<QString> seen;
        for (const NodeDef& d : nodeCatalog()) {
            QVERIFY2(!d.id.isEmpty(), "un bloc du catalogue n'a pas d'identifiant");
            QVERIFY2(!seen.contains(d.id), qUtf8Printable("identifiant en double : " + d.id));
            seen.insert(d.id);
        }
    }

    void everyBlockIsDescribed() {
        for (const NodeDef& d : nodeCatalog()) {
            QVERIFY2(!d.title.isEmpty(), qUtf8Printable("titre manquant : " + d.id));
            QVERIFY2(!d.category.isEmpty(), qUtf8Printable("catégorie manquante : " + d.id));
            QVERIFY2(d.color.isValid(), qUtf8Printable("couleur invalide : " + d.id));
        }
    }

    void findNodeDefFindsEveryBlock() {
        for (const NodeDef& d : nodeCatalog()) {
            const NodeDef* found = findNodeDef(d.id);
            QVERIFY2(found != nullptr, qUtf8Printable("introuvable : " + d.id));
            QCOMPARE(found->id, d.id);
        }
        QVERIFY(findNodeDef(QStringLiteral("ce-bloc-n-existe-pas")) == nullptr);
    }

    void inputsAndOutputsAreUniquePerBlock() {
        for (const NodeDef& d : nodeCatalog()) {
            QSet<QString> ins;
            for (const DataIn& in : d.dataIns) {
                QVERIFY2(!ins.contains(in.id), qUtf8Printable("entrée en double dans " + d.id + " : " + in.id));
                ins.insert(in.id);
                QVERIFY(d.findInput(in.id) != nullptr);
            }
            QSet<QString> outs;
            for (const auto& out : d.flowOuts) {
                QVERIFY2(!outs.contains(out.first), qUtf8Printable("sortie en double dans " + d.id));
                outs.insert(out.first);
            }
        }
    }

    void typeNamesRoundTrip_data() {
        QTest::addColumn<int>("type");
        QTest::newRow("nombre") << int(DataType::Number);
        QTest::newRow("texte") << int(DataType::Text);
        QTest::newRow("booleen") << int(DataType::Bool);
        QTest::newRow("liste de nombres") << int(DataType::NumList);
        QTest::newRow("liste de textes") << int(DataType::TextList);
    }
    void typeNamesRoundTrip() {
        QFETCH(int, type);
        const auto t = static_cast<DataType>(type);
        QCOMPARE(typeFromName(typeName(t)), t);
    }

    void listHelpers() {
        QVERIFY(isList(DataType::NumList));
        QVERIFY(isList(DataType::TextList));
        QVERIFY(!isList(DataType::Number));
        QCOMPARE(elemType(DataType::NumList), DataType::Number);
        QCOMPARE(elemType(DataType::TextList), DataType::Text);
    }
};

QTEST_MAIN(TestGraph)
#include "test_graph.moc"
