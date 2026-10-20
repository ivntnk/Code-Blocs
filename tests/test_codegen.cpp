// Génération du code C# (codegen.h) pour tous les exemples et tous les modes.
#include <QtTest>

#include "codegen.h"
#include "editor.h"
#include "examples.h"

class TestCodegen : public QObject {
    Q_OBJECT
private slots:
    void everyExampleGeneratesCode_data() {
        QTest::addColumn<int>("id");
        QTest::addColumn<int>("mode");
        const struct {
            const char* name;
            CodeMode mode;
        } modes[] = {{"export", CodeMode::Export}, {"run", CodeMode::Run}, {"standalone", CodeMode::Standalone}};
        for (const ExampleInfo& e : examples())
            for (const auto& m : modes) {
                const QByteArray row = QString::fromUtf8(e.name).toUtf8() + " [" + m.name + "]";
                QTest::newRow(row.constData()) << e.id << int(m.mode);
            }
    }
    void everyExampleGeneratesCode() {
        QFETCH(int, id);
        QFETCH(int, mode);
        NodeScene scene;
        buildExample(scene, id);
        const CodeResult r = generateCSharp(scene, static_cast<CodeMode>(mode));
        QVERIFY2(!r.code.isEmpty(), "aucun code généré");
        QVERIFY2(r.code.contains(QStringLiteral("Main")), "le code généré n'a pas de point d'entrée Main");
        // Les exemples sont censés être corrects ; si ce test devient bavard, c'est un vrai signal.
        if (!r.problems.isEmpty()) QWARN(qUtf8Printable("problèmes signalés : " + r.problems.join(" | ")));
    }

    void generationIsDeterministic() {
        NodeScene scene;
        buildExample(scene, 0);
        const CodeResult a = generateCSharp(scene, CodeMode::Export);
        const CodeResult b = generateCSharp(scene, CodeMode::Export);
        QCOMPARE(a.code, b.code);
    }

    void legacyOverloadMatchesRunMode() {
        NodeScene scene;
        buildExample(scene, 0);
        QCOMPARE(generateCSharp(scene, true).code, generateCSharp(scene, CodeMode::Run).code);
        QCOMPARE(generateCSharp(scene, false).code, generateCSharp(scene, CodeMode::Export).code);
    }

    void emptySceneDoesNotCrash() {
        NodeScene scene;
        const CodeResult r = generateCSharp(scene, CodeMode::Export);
        Q_UNUSED(r)
        QVERIFY(true);
    }
};

QTEST_MAIN(TestCodegen)
#include "test_codegen.moc"
