// Tutoriels : contenu présent et exemples référencés existants (tutorials.h / examples.h).
#include <QtTest>

#include "examples.h"
#include "tutorials.h"

class TestTutorials : public QObject {
    Q_OBJECT
private slots:
    void tutorialsExist() { QVERIFY(!tutorials().isEmpty()); }

    void everyTutorialIsComplete() {
        for (const Tutorial& t : tutorials()) {
            QVERIFY2(!t.title.isEmpty(), "tutoriel sans titre");
            QVERIFY2(!t.steps.isEmpty(), qUtf8Printable("tutoriel sans étape : " + t.title));
            for (const TutorialStep& s : t.steps) {
                QVERIFY2(!s.title.isEmpty(), qUtf8Printable("étape sans titre dans : " + t.title));
                QVERIFY2(!s.html.isEmpty(), qUtf8Printable("étape vide : " + s.title));
            }
        }
    }

    void referencedExamplesExist() {
        QSet<int> ids;
        for (const ExampleInfo& e : examples()) ids.insert(e.id);
        for (const Tutorial& t : tutorials())
            for (const TutorialStep& s : t.steps)
                if (s.example != -1)
                    QVERIFY2(ids.contains(s.example),
                             qUtf8Printable(QString("exemple %1 inconnu (étape « %2 »)").arg(s.example).arg(s.title)));
    }

    void exampleNamesAreAvailable() {
        for (const ExampleInfo& e : examples()) QVERIFY(e.name != nullptr && *e.name != '\0');
    }
};

QTEST_MAIN(TestTutorials)
#include "test_tutorials.moc"
