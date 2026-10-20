#include "exporter.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>

#include "runner.h"

QString Exporter::sanitizeName(const QString& s) {
    QString r;
    for (const QChar c : s.trimmed()) r += (c.isLetterOrNumber() && c.unicode() < 128) || c == '_' ? c : QChar('_');
    if (r.isEmpty() || r[0].isDigit()) r.prepend("Jeu_");
    return r;
}

bool Exporter::writeProject(const QString& dir, const QString& code, const QMap<QString, Asset>& assets,
                            bool graphical, const QString& name, int dotnetMajor, QString* error) {
    auto fail = [&](const QString& m) { if (error) *error = m; return false; };
    QDir d(dir);
    if (!d.mkpath(".")) return fail("Impossible de créer le dossier : " + dir);

    QFile prog(d.filePath("Program.cs"));
    if (!prog.open(QIODevice::WriteOnly)) return fail("Impossible d'écrire Program.cs");
    prog.write(code.toUtf8());
    prog.close();

    // ressources : intégrées dans l'exécutable (un seul fichier à partager)
    QDir(d.filePath("Assets")).removeRecursively();
    if (!assets.isEmpty()) {
        d.mkpath("Assets");
        for (auto it = assets.begin(); it != assets.end(); ++it) {
            QFile f(d.filePath("Assets/" + sanitizeName(it.key()) + "." + it.value().ext));
            if (!f.open(QIODevice::WriteOnly)) return fail("Impossible d'écrire une ressource : " + it.key());
            f.write(it.value().data);
        }
    }

    const int major = dotnetMajor >= 6 ? dotnetMajor : 8;
    const QString tfm = QString("net%1.0").arg(major) + (graphical ? "-windows" : "");
    const QString csproj = QString(
        "<Project Sdk=\"Microsoft.NET.Sdk\">\n"
        "  <PropertyGroup>\n"
        "    <OutputType>%1</OutputType>\n"
        "    <TargetFramework>%2</TargetFramework>\n"
        "%3"
        "    <Nullable>disable</Nullable>\n"
        "    <ImplicitUsings>disable</ImplicitUsings>\n"
        "    <AssemblyName>%4</AssemblyName>\n"
        "    <RootNamespace>%4</RootNamespace>\n"
        "    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>\n"
        "  </PropertyGroup>\n"
        "  <ItemGroup>\n"
        "    <Compile Include=\"Program.cs\" />\n"
        "    <EmbeddedResource Include=\"Assets\\*\" LogicalName=\"%(Filename)%(Extension)\" />\n"
        "  </ItemGroup>\n"
        "</Project>\n")
        .arg(graphical ? "WinExe" : "Exe", tfm,
             graphical ? "    <UseWindowsForms>true</UseWindowsForms>\n" : "", name);
    QFile pj(d.filePath(name + ".csproj"));
    if (!pj.open(QIODevice::WriteOnly)) return fail("Impossible d'écrire le .csproj");
    pj.write(csproj.toUtf8());
    return true;
}

Exporter::Exporter(QObject* parent) : QObject(parent) {}

Exporter::~Exporter() {
    if (proc_) {
        proc_->disconnect(this);
        proc_->kill();
        proc_->waitForFinished(1000);
    }
}

void Exporter::finish(bool ok, const QString& path) {
    if (proc_) {
        proc_->deleteLater();
        proc_ = nullptr;
    }
    emit finished(ok, path);
}

void Exporter::start(const QString& code, const QMap<QString, Asset>& assets, bool graphical, const ExportOptions& opt) {
    if (isRunning()) return;
    const QString name = sanitizeName(opt.name);
    const QString finalDir = QDir(opt.outDir).filePath(name);

    // ---- projet Visual Studio : on écrit simplement le dossier
    if (opt.kind == ExportOptions::VisualStudioProject) {
        const QString dotnet = findDotnetExecutable();
        QString err;
        if (!writeProject(finalDir, code, assets, graphical, name, dotnet.isEmpty() ? 0 : dotnetSdkMajor(dotnet), &err)) {
            emit output("✖ " + err + "\n", 1);
            emit finished(false, QString());
            return;
        }
        emit output(QString("✔ Projet créé : %1\nOuvre %2.csproj avec Visual Studio, ou lance « dotnet run » dans ce dossier.\n")
                        .arg(QDir::toNativeSeparators(finalDir), name), 2);
        emit finished(true, finalDir);
        return;
    }

    // ---- application (.exe) : dotnet publish
    const QString dotnet = findDotnetExecutable();
    if (dotnet.isEmpty()) {
        emit output("✖ .NET est introuvable.\nInstalle le SDK .NET : https://dotnet.microsoft.com/download\n", 1);
        emit finished(false, QString());
        return;
    }
    const int major = dotnetSdkMajor(dotnet);
    if (major < 6) {
        emit output("✖ Le SDK .NET 6 ou plus récent est nécessaire (pas seulement le « runtime »).\n", 1);
        emit finished(false, QString());
        return;
    }

    buildDir_ = QDir::tempPath() + "/BlocsExport/" + name;
    QDir(buildDir_).removeRecursively();
    QString err;
    if (!writeProject(buildDir_, code, assets, graphical, name, major, &err)) {
        emit output("✖ " + err + "\n", 1);
        emit finished(false, QString());
        return;
    }
    QDir(finalDir).removeRecursively();
    QDir().mkpath(finalDir);
    exePath_ = QDir(finalDir).filePath(name + ".exe");

    emit output(opt.kind == ExportOptions::PublishSelfContained
                    ? "⚙ Fabrication de l'application autonome… (la première fois, .NET est téléchargé : une à deux minutes)\n"
                    : "⚙ Fabrication de l'application…\n", 2);

    QStringList args = {"publish", QDir(buildDir_).filePath(name + ".csproj"), "-c", "Release", "-r", "win-x64",
                        "--self-contained", opt.kind == ExportOptions::PublishSelfContained ? "true" : "false",
                        "-p:PublishSingleFile=true", "-p:IncludeNativeLibrariesForSelfExtract=true",
                        "-p:DebugType=none", "-p:UseSharedCompilation=false", "-nologo", "-v", "q",
                        "-nodeReuse:false", "-o", finalDir};

    raw_.clear();
    cancelled_ = false;
    dec_ = QStringDecoder(QStringConverter::Utf8);
    QProcess* p = proc_ = new QProcess(this);
    p->setWorkingDirectory(buildDir_);
    p->setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DOTNET_CLI_TELEMETRY_OPTOUT", "1");
    env.insert("DOTNET_NOLOGO", "1");
    env.insert("MSBUILDDISABLENODEREUSE", "1");
    p->setProcessEnvironment(env);
    connect(p, &QProcess::readyRead, this, [this, p] { raw_ += dec_(p->readAll()); });
    connect(p, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            emit output("✖ Impossible de lancer « dotnet ».\n", 1);
            finish(false, QString());
        }
    });
    connect(p, &QProcess::finished, this, [this, p, finalDir](int code, QProcess::ExitStatus) {
        raw_ += dec_(p->readAll());
        if (cancelled_) {
            emit output("■ Export annulé.\n", 2);
            finish(false, QString());
            return;
        }
        if (code != 0 || !QFile::exists(exePath_)) {
            static const QRegularExpression suffix(R"(\s*\[[^\]]*\.csproj\]\s*$)");
            QSet<QString> seen;
            QStringList lines;
            for (QString l : raw_.split('\n')) {
                l.remove('\r');
                l.remove(suffix);
                if (!l.trimmed().isEmpty() && !seen.contains(l)) { seen.insert(l); lines << l; }
            }
            emit output(lines.join('\n') + "\n✖ L'export a échoué.\n", 1);
            if (raw_.contains("NU1101") || raw_.contains("NU1100") || raw_.contains("nuget.org"))
                emit output("Cette fabrication a besoin d'Internet (téléchargement de composants .NET). "
                            "Vérifie ta connexion, ou choisis « .exe léger ».\n", 1);
            finish(false, QString());
            return;
        }
        QFile::remove(QDir(finalDir).filePath(QFileInfo(exePath_).completeBaseName() + ".pdb"));
        emit output("✔ Application créée : " + QDir::toNativeSeparators(exePath_) + "\n", 2);
        finish(true, finalDir);
    });
    p->start(dotnet, args);
}

void Exporter::cancel() {
    if (proc_) {
        cancelled_ = true;
        proc_->kill();
    }
}
