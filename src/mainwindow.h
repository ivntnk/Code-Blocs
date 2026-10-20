// mainwindow.h — fenêtre principale : palette, éditeur, propriétés, code C#, console, fichiers.
#pragma once

#include <QJsonObject>
#include <QMainWindow>

#include "history.h"

class NodeScene;
class NodeEditorView;
class Runner;
class QAction;
class QDockWidget;
class QLineEdit;
class QPlainTextEdit;
class QListWidget;
class QScrollArea;
class GameWindow;
class ProjectExplorer;
class QMenu;
class Exporter;
class TutorialDialog;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    void openPath(const QString& path);
    void showWelcomeIfFirstRun();            // ouvre le tutoriel au tout premier lancement      // ouvre un projet .blocs (ex. double-clic dans l'Explorateur)

protected:
    void closeEvent(QCloseEvent* e) override;

private:
    void buildPalette();
    void buildDocks();
    void buildConsole();
    void buildMenus();

    void scheduleRefresh(bool rebuildPanel);   // regroupe les mises à jour de l'interface
    void rebuildProps();
    void refreshCode();

    // exécution
    void runProgram();
    void appendConsole(const QString& text, int kind);
    void sendConsoleInput();

    // menu contextuel + presse-papiers de blocs
    void showContextMenu(const QPoint& globalPos, const QPointF& scenePos);
    void copySelection();
    void cutSelection();
    void pasteAt(const QPointF& scenePos);
    QJsonObject clipboardBlocs() const;
    QPointF cursorScenePos() const;

    // historique (annuler / rétablir)
    void scheduleSnapshot();
    void commitSnapshot();
    void restoreState(const QByteArray& state);
    void resetHistory();
    void undo();
    void redo();
    void loadExample(int index);
    void openHelp(int tutorial = 0);
    void exportApplication();

    // dossier de projet + explorateur de fichiers
    void buildExplorer();
    void setWorkspace(const QString& dir);
    void openFolder();
    void newProjectFolder();
    void updateTitle();
    void rebuildRecentMenu();
    QStringList missingAssetWarnings() const;

    // fichiers
    bool maybeSave();
    bool saveFile(bool askName);
    void openFile();
    void newProject();
    void exportCSharp();
    void setCurrentFile(const QString& path);
    void loadDemo();

    NodeScene* scene_;
    NodeEditorView* view_;
    Runner* runner_;
    QScrollArea* propsScroll_;
    QPlainTextEdit* code_;
    QPlainTextEdit* console_;
    QLineEdit* consoleInput_;
    QDockWidget* consoleDock_;
    QListWidget* problems_;
    QAction* runAct_ = nullptr;
    QAction* stopAct_ = nullptr;
    QAction* undoAct_ = nullptr;
    QAction* redoAct_ = nullptr;
    QTimer* snapTimer_ = nullptr;
    TutorialDialog* help_ = nullptr;
    GameWindow* game_ = nullptr;
    Exporter* exporter_ = nullptr;
    ProjectExplorer* explorer_ = nullptr;
    QDockWidget* explorerDock_ = nullptr;
    QDockWidget* paletteDock_ = nullptr;
    QMenu* recentMenu_ = nullptr;
    QString workspace_;
    QAction* exportAct_ = nullptr;
    History history_;
    bool restoring_ = false;
    QString file_;
    QString lastCode_;
    int spawn_ = 0;
    bool refreshPending_ = false;
    bool propsPending_ = false;
};
