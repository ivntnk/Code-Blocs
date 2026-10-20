#include "runner.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>

namespace {
void writeIfChanged(const QString& path, const QByteArray& data) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly) && f.readAll() == data) return;   // inchangé : on garde le cache de build
    f.close();
    QFile w(path);
    if (w.open(QIODevice::WriteOnly)) w.write(data);
}
}  // namespace

Runner::Runner(QObject* parent) : QObject(parent) {}

Runner::~Runner() {
    if (proc_) {
        proc_->disconnect(this);
        proc_->kill();
        proc_->waitForFinished(1000);
    }
}

QString findDotnetExecutable() {
    QString p = QStandardPaths::findExecutable("dotnet");
    if (!p.isEmpty()) return p;
    const QStringList guesses = {"C:/Program Files/dotnet/dotnet.exe", "C:/Program Files (x86)/dotnet/dotnet.exe",
                                 "/usr/local/share/dotnet/dotnet", "/usr/share/dotnet/dotnet", "/usr/lib/dotnet/dotnet"};
    for (const QString& g : guesses)
        if (QFile::exists(g)) return g;
    return QString();
}

int dotnetSdkMajor(const QString& dotnet) {
    QProcess p;
    p.setWorkingDirectory(QDir::tempPath());
    p.start(dotnet, {"--version"});
    if (!p.waitForFinished(15000) || p.exitCode() != 0) return 0;
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed().section('.', 0, 0).toInt();
}

QString Runner::findDotnet() const { return findDotnetExecutable(); }
int Runner::sdkMajor(const QString& dotnet) const { return dotnetSdkMajor(dotnet); }

void Runner::start(const QString& code) {
    if (isRunning()) return;

    dotnet_ = findDotnet();
    if (dotnet_.isEmpty()) {
        emit output("✖ .NET est introuvable.\nInstalle le SDK .NET : https://dotnet.microsoft.com/download\n"
                    "(puis relance Blocs, pour qu'il retrouve « dotnet »).\n", Error);
        return;
    }
    const int major = sdkMajor(dotnet_);
    if (major < 6) {
        emit output("✖ Le SDK .NET 6 ou plus récent est nécessaire (pas seulement le « runtime »).\n"
                    "Télécharge-le ici : https://dotnet.microsoft.com/download\n", Error);
        return;
    }

    dir_ = QDir::tempPath() + "/BlocsRun";
    QDir().mkpath(dir_);
    const QString csproj = QString(
        "<Project Sdk=\"Microsoft.NET.Sdk\">\n"
        "  <PropertyGroup>\n"
        "    <OutputType>Exe</OutputType>\n"
        "    <TargetFramework>net%1.0</TargetFramework>\n"
        "    <Nullable>disable</Nullable>\n"
        "    <ImplicitUsings>disable</ImplicitUsings>\n"
        "    <UseAppHost>false</UseAppHost>\n"
        "    <AssemblyName>Blocs</AssemblyName>\n"
        "    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>\n"
        "  </PropertyGroup>\n"
        "  <ItemGroup><Compile Include=\"Program.cs\" /></ItemGroup>\n"
        "</Project>\n").arg(major);
    writeIfChanged(dir_ + "/Blocs.csproj", csproj.toUtf8());
    writeIfChanged(dir_ + "/Program.cs", code.toUtf8());

    stopping_ = false;
    phase_ = Phase::Build;
    buildOutput_.clear();
    emit runningChanged(true);
    emit output("⚙ Compilation…\n", Info);
    launch({"build", "-c", "Release", "-nologo", "-v", "q", "-nodeReuse:false",
            "-p:UseSharedCompilation=false", "-o", dir_ + "/out"});
}

void Runner::launch(const QStringList& args) {
    outDec_ = QStringDecoder(QStringConverter::Utf8);
    errDec_ = QStringDecoder(QStringConverter::Utf8);

    QProcess* p = proc_ = new QProcess(this);
    p->setWorkingDirectory(dir_);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("DOTNET_CLI_TELEMETRY_OPTOUT", "1");
    env.insert("DOTNET_NOLOGO", "1");
    env.insert("DOTNET_SKIP_FIRST_TIME_EXPERIENCE", "1");
    env.insert("MSBUILDDISABLENODEREUSE", "1");
    p->setProcessEnvironment(env);

    connect(p, &QProcess::readyReadStandardOutput, this, [this, p] {
        const QString t = outDec_(p->readAllStandardOutput());
        if (phase_ == Phase::Build) buildOutput_ += t;     // affiché seulement en cas d'échec
        else feedOutput(t);
    });
    connect(p, &QProcess::readyReadStandardError, this, [this, p] {
        const QString t = errDec_(p->readAllStandardError());
        if (phase_ == Phase::Build) buildOutput_ += t;
        else emit output(t, Error);
    });
    connect(p, &QProcess::finished, this, [this](int code, QProcess::ExitStatus st) { onFinished(code, st); });
    connect(p, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            emit output("✖ Impossible de lancer « dotnet ».\n", Error);
            finishAll();
        }
    });
    p->start(dotnet_, args);
}

QString Runner::cleanBuildOutput(const QString& raw) const {
    static const QRegularExpression projSuffix(R"(\s*\[[^\]]*\.csproj\]\s*$)");
    QString dirFwd = dir_, dirBack = dir_;
    dirBack.replace('/', '\\');
    QStringList lines;
    QSet<QString> seen;
    for (QString line : raw.split('\n')) {
        line.remove('\r');
        line.remove(projSuffix);
        line.replace(dirFwd + "/", "").replace(dirBack + "\\", "");
        if (line.trimmed().isEmpty() || seen.contains(line)) continue;   // MSBuild répète les erreurs
        seen.insert(line);
        lines << line;
    }
    return lines.join('\n') + "\n";
}

void Runner::onFinished(int code, QProcess::ExitStatus) {
    QProcess* p = proc_;
    if (p) {                                   // dernières données éventuelles
        const QString rest = outDec_(p->readAllStandardOutput());
        if (phase_ == Phase::Build) buildOutput_ += rest;
        else if (!rest.isEmpty()) feedOutput(rest);
        const QString restErr = errDec_(p->readAllStandardError());
        if (phase_ == Phase::Build) buildOutput_ += restErr;
        else if (!restErr.isEmpty()) emit output(restErr, Error);
    }

    if (stopping_) {
        emit output("\n■ Arrêté.\n", Info);
        finishAll();
        return;
    }
    if (phase_ == Phase::Build) {
        if (code != 0) {
            emit output(cleanBuildOutput(buildOutput_), Error);
            emit output("✖ La compilation a échoué.\n", Error);
            finishAll();
            return;
        }
        p->deleteLater();
        proc_ = nullptr;
        phase_ = Phase::Run;
        inGame_ = false;
        gameBuf_.clear();
        emit output("▶ Exécution\n─────────────\n", Info);
        launch({dir_ + "/out/Blocs.dll"});
        return;
    }
    emit output(QString("\n─────────────\n■ Programme terminé (code %1)\n").arg(code), Info);
    finishAll();
}

// Le programme parle sur la sortie standard : texte normal, mais aussi des images de jeu
// encadrées par les caractères \x01 (début) et \x02 (fin), qu'on ne montre pas dans la console.
void Runner::feedOutput(const QString& t) {
    QString normal;
    for (const QChar ch : t) {
        if (inGame_) {
            if (ch == QChar(2)) {
                emit gameData(gameBuf_);
                gameBuf_.clear();
                inGame_ = false;
            } else {
                gameBuf_ += ch;
            }
        } else if (ch == QChar(1)) {
            if (!normal.isEmpty()) { emit output(normal, Normal); normal.clear(); }
            inGame_ = true;
        } else {
            normal += ch;
        }
    }
    if (!normal.isEmpty()) emit output(normal, Normal);
}

void Runner::finishAll() {
    if (proc_) {
        proc_->deleteLater();
        proc_ = nullptr;
    }
    phase_ = Phase::Idle;
    stopping_ = false;
    emit runningChanged(false);
}

void Runner::stop() {
    if (!isRunning() || !proc_) return;
    stopping_ = true;
    proc_->kill();
}

void Runner::sendInput(const QString& line) {
    if (phase_ == Phase::Run && proc_) proc_->write(line.toUtf8() + "\n");
}
