// runner.h — compile et exécute le code C# avec le SDK .NET (« dotnet »), en direct.
#pragma once

#include <QObject>
#include <QProcess>
#include <QStringDecoder>

// Outils partagés avec l'export
QString findDotnetExecutable();               // chemin de « dotnet », ou vide
int dotnetSdkMajor(const QString& dotnet);     // 8 pour .NET 8… (0 si aucun SDK)

class Runner : public QObject {
    Q_OBJECT
public:
    enum Kind { Normal = 0, Error = 1, Info = 2, UserInput = 3 };

    explicit Runner(QObject* parent = nullptr);
    ~Runner() override;

    bool isRunning() const { return phase_ != Phase::Idle; }
    void start(const QString& csharpCode);       // compile puis lance
    void stop();
    void sendInput(const QString& line);         // répond à un Console.ReadLine()

signals:
    void output(const QString& text, int kind);
    void gameData(const QString& payload);      // une image complète du jeu graphique
    void runningChanged(bool running);

private:
    enum class Phase { Idle, Build, Run };

    QString findDotnet() const;
    int sdkMajor(const QString& dotnet) const;
    void launch(const QStringList& args);
    void onFinished(int code, QProcess::ExitStatus status);
    void finishAll();
    void feedOutput(const QString& text);       // sépare le texte normal des images du jeu
    QString cleanBuildOutput(const QString& raw) const;

    Phase phase_ = Phase::Idle;
    QProcess* proc_ = nullptr;
    QString dotnet_, dir_, buildOutput_;
    bool stopping_ = false;
    bool inGame_ = false;                       // on est au milieu d'un message \x01 … \x02
    QString gameBuf_;
    QStringDecoder outDec_{QStringConverter::Utf8};
    QStringDecoder errDec_{QStringConverter::Utf8};
};
