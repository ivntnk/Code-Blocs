// Explorateur de fichiers (explorer.h) : vue « ordinateur » par défaut, règles de nommage, protection système.
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTreeView>
#include <QtTest>

#include "explorer.h"

class TestExplorer : public QObject {
    Q_OBJECT
private slots:
    void startsOnTheWholeComputer() {
        ProjectExplorer e;
        QVERIFY(e.rootPath().isEmpty());
        QVERIFY(e.isBrowsingComputer());
        QVERIFY(e.tree()->model() == e.model());
        QVERIFY(!e.tree()->rootIndex().isValid());   // racine invisible = les lecteurs (C:, D:…)
    }

    void openingAFolderThenLeavingIt() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        ProjectExplorer e;
        e.setRootPath(tmp.path());
        QVERIFY(ProjectExplorer::samePath(e.rootPath(), tmp.path()));
        QVERIFY(!e.isBrowsingComputer());
        e.setRootPath(QString());
        QVERIFY(e.isBrowsingComputer());
    }

    void nonexistentFolderFallsBackToComputer() {
        ProjectExplorer e;
        e.setRootPath(QStringLiteral("/ce/dossier/n/existe/pas"));
        QVERIFY(e.isBrowsingComputer());
    }

    void systemLocationsAreProtected() {
        QVERIFY(ProjectExplorer::isProtectedPath(QDir::rootPath()));
        QVERIFY(ProjectExplorer::isProtectedPath(QDir::homePath()));
        QVERIFY(ProjectExplorer::isProtectedPath(QString()));
    }

    void projectFoldersAreNotProtected() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(!ProjectExplorer::isProtectedPath(QDir(tmp.path()).filePath("MonProjet")));
    }

    void nameValidation_data() {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("valid");
        QTest::newRow("simple") << QStringLiteral("Programme") << true;
        QTest::newRow("avec espace") << QStringLiteral("Mon jeu") << true;
        QTest::newRow("vide") << QString() << false;
        QTest::newRow("caractere interdit") << QStringLiteral("a:b") << false;
        QTest::newRow("point final") << QStringLiteral("jeu.") << false;
        QTest::newRow("reserve windows") << QStringLiteral("CON") << false;
    }
    void nameValidation() {
        QFETCH(QString, name);
        QFETCH(bool, valid);
        QCOMPARE(ProjectExplorer::isValidName(name), valid);
    }

    void uniquePathAvoidsExistingFiles() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString first = ProjectExplorer::uniquePath(tmp.path(), "Programme", ".blocs");
        QVERIFY(ProjectExplorer::createProgram(first));
        const QString second = ProjectExplorer::uniquePath(tmp.path(), "Programme", ".blocs");
        QVERIFY(!ProjectExplorer::samePath(first, second));
        QVERIFY(!QFile::exists(second));
    }

    void isUnderChecksFolderMembership() {
        QVERIFY(ProjectExplorer::isUnder("/a/b/c", "/a/b"));
        QVERIFY(ProjectExplorer::isUnder("/a/b", "/a/b"));
        QVERIFY(!ProjectExplorer::isUnder("/a/bc", "/a/b"));
    }
};

QTEST_MAIN(TestExplorer)
#include "test_explorer.moc"
