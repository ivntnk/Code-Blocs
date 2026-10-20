// Export : noms nettoyés et écriture d'un projet .NET (exporter.h), sans lancer dotnet.
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "exporter.h"

namespace {
QString readAll(const QString& path) {
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}
}  // namespace

class TestExporter : public QObject {
    Q_OBJECT
private slots:
    void sanitizeName_data() {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");
        auto row = [](const char* label, const QString& in, const QString& out) {
            QTest::newRow(label) << in << out;
        };
        row("simple", QStringLiteral("MonJeu"), QStringLiteral("MonJeu"));
        row("espace", QStringLiteral("Mon Jeu"), QStringLiteral("Mon_Jeu"));
        row("chiffre en tete", QStringLiteral("1abc"), QStringLiteral("Jeu_1abc"));
        row("vide", QString(), QStringLiteral("Jeu_"));
        row("accents", QString::fromUtf8("été"), QStringLiteral("_t_"));
        row("underscore", QStringLiteral("mon_jeu"), QStringLiteral("mon_jeu"));
        row("espaces autour", QStringLiteral("  jeu  "), QStringLiteral("jeu"));
    }
    void sanitizeName() {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QCOMPARE(Exporter::sanitizeName(input), expected);
    }

    void writesConsoleProject() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString error;
        QVERIFY2(Exporter::writeProject(tmp.path(), "class P {}", {}, false, "Demo", 8, &error), qUtf8Printable(error));

        QCOMPARE(readAll(tmp.filePath("Program.cs")), QStringLiteral("class P {}"));
        const QString csproj = readAll(tmp.filePath("Demo.csproj"));
        QVERIFY(csproj.contains("<OutputType>Exe</OutputType>"));
        QVERIFY(csproj.contains("<TargetFramework>net8.0</TargetFramework>"));
        QVERIFY(!csproj.contains("UseWindowsForms"));
    }

    void writesGraphicalProject() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(Exporter::writeProject(tmp.path(), "class P {}", {}, true, "Demo", 9));
        const QString csproj = readAll(tmp.filePath("Demo.csproj"));
        QVERIFY(csproj.contains("<OutputType>WinExe</OutputType>"));
        QVERIFY(csproj.contains("net9.0-windows"));
        QVERIFY(csproj.contains("<UseWindowsForms>true</UseWindowsForms>"));
    }

    void unknownDotnetVersionFallsBackToDefault() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(Exporter::writeProject(tmp.path(), "x", {}, false, "Demo", 0));
        QVERIFY(readAll(tmp.filePath("Demo.csproj")).contains("net8.0"));
    }

    void writesAssets() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QMap<QString, Asset> assets;
        assets.insert(QStringLiteral("son clic"), Asset{QStringLiteral("wav"), QByteArray("RIFF")});
        QString error;
        QVERIFY2(Exporter::writeProject(tmp.path(), "x", assets, false, "Demo", 8, &error), qUtf8Printable(error));
        QVERIFY(QFile::exists(tmp.filePath("Assets/son_clic.wav")));
    }
};

QTEST_MAIN(TestExporter)
#include "test_exporter.moc"
