// exporter.h — fabrique une application Windows autonome (.exe) ou un projet Visual Studio.
#pragma once

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QStringDecoder>

#include "editor.h"

struct ExportOptions {
    enum Kind {
        PublishLight,         // .exe léger : nécessite le « runtime .NET » sur l'autre PC
        PublishSelfContained, // .exe autonome : tout inclus (gros, nécessite Internet pour être fabriqué)
        VisualStudioProject   // dossier de projet à ouvrir dans Visual Studio / dotnet
    };
    Kind kind = PublishLight;
    QString name = "MonJeu";  // nom de l'application (lettres, chiffres, _)
    QString outDir;           // dossier de destination (on y crée un sous-dossier « name »)
};

class Exporter : public QObject {
    Q_OBJECT
public:
    explicit Exporter(QObject* parent = nullptr);
    ~Exporter() override;

    bool isRunning() const { return proc_ != nullptr; }
    void start(const QString& code, const QMap<QString, Asset>& assets, bool graphical, const ExportOptions& opt);
    void cancel();

    // Écrit Program.cs, <nom>.csproj et Assets/… dans « dir ». Utilisable seul (tests, projet Visual Studio).
    static bool writeProject(const QString& dir, const QString& code, const QMap<QString, Asset>& assets,
                             bool graphical, const QString& name, int dotnetMajor, QString* error = nullptr);
    static QString sanitizeName(const QString& s);

signals:
    void output(const QString& text, int kind);      // kind : 0 normal, 1 erreur, 2 info (comme Runner)
    void finished(bool ok, const QString& path);

private:
    void finish(bool ok, const QString& path);

    QProcess* proc_ = nullptr;
    QString raw_, exePath_, buildDir_;
    bool cancelled_ = false;
    QStringDecoder dec_{QStringConverter::Utf8};
};
