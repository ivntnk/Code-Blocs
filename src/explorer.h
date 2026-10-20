// explorer.h — explorateur de fichiers du dossier de projet (comme dans VS Code).
#pragma once

#include <QFileSystemModel>
#include <QStringList>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTreeView;

// Modèle de fichiers : met en évidence le programme actuellement ouvert
class ExplorerModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;
    void setCurrent(const QString& path);
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;   // pas de glisser/renommer sur les dossiers système

private:
    QString current_;
};

class ProjectExplorer : public QWidget {
    Q_OBJECT
public:
    explicit ProjectExplorer(QWidget* parent = nullptr);

    void setRootPath(const QString& dir);         // dossier vide = pas de dossier de projet : on parcourt tout l'ordinateur (C:, D:…)
    bool isBrowsingComputer() const { return root_.isEmpty(); }
    QString rootPath() const { return root_; }
    void setCurrentFile(const QString& path);     // programme ouvert dans l'éditeur (mis en gras)
    void refresh();
    QTreeView* tree() const { return tree_; }
    ExplorerModel* model() const { return model_; }

    // ---- opérations sur les fichiers : sans interface, donc réutilisables et testables ----
    static bool isValidName(const QString& name, QString* why = nullptr);
    static QString uniquePath(const QString& dir, const QString& baseName, const QString& ext = QString());
    static bool samePath(const QString& a, const QString& b);
    static bool isUnder(const QString& path, const QString& folder);   // path est dans folder (ou égal)
    static bool isProtectedPath(const QString& path);   // lecteur, dossier personnel (et parents), Windows, Program Files
    static bool createFolder(const QString& path, QString* err = nullptr);
    static bool createProgram(const QString& path, QString* err = nullptr);    // projet .blocs avec un bloc Début
    static bool createTextFile(const QString& path, QString* err = nullptr);
    static bool copyRecursively(const QString& src, const QString& dst, QString* err = nullptr);
    static QString duplicatePath(const QString& path, QString* err = nullptr);  // « nom - copie.ext »
    static bool trashPath(const QString& path, QString* trashed = nullptr);     // corbeille, sinon false

signals:
    void openProgramRequested(const QString& path);
    void importAssetRequested(const QString& path);
    void pathRenamed(const QString& oldPath, const QString& newPath);
    void pathRemoved(const QString& path);
    void openFolderRequested();
    void openFolderPathRequested(const QString& dir);   // « Ouvrir comme dossier de projet » (clic droit)
    void browseComputerRequested();                      // revenir à la vue « tout l'ordinateur »
    void newProjectRequested();
    void message(const QString& text);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void buildUi();
    QString targetDir() const;
    QStringList selectedPaths() const;
    void showContextMenu(const QPoint& pos);
    void activatePath(const QString& path);
    void previewImage(const QString& path);
    void selectLater(const QString& path);
    void revealHome();

    void newProgram();
    void newFolder();
    void newTextFile();
    void importFiles();
    void duplicateSelected();
    void deleteSelected();
    void renameSelected();
    void copySelected(bool cut);
    void paste();

    ExplorerModel* model_;
    QTreeView* tree_;
    QStackedWidget* stack_;
    QLabel* title_;
    QString root_;
    QStringList clip_;
    bool clipCut_ = false;
};
