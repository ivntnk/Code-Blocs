#include "explorer.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

// ===========================================================================
//  Modèle
// ===========================================================================
void ExplorerModel::setCurrent(const QString& path) {
    const QString old = current_;
    current_ = path;
    for (const QString& p : {old, path}) {
        const QModelIndex i = index(p);
        if (i.isValid()) emit dataChanged(i, i, {Qt::FontRole, Qt::ForegroundRole});
    }
}

QVariant ExplorerModel::data(const QModelIndex& idx, int role) const {
    if (!current_.isEmpty() && idx.column() == 0 && (role == Qt::FontRole || role == Qt::ForegroundRole) &&
        ProjectExplorer::samePath(filePath(idx), current_)) {
        if (role == Qt::ForegroundRole) return QColor("#4fc3f7");
        QFont f = QFileSystemModel::data(idx, role).value<QFont>();
        f.setBold(true);
        return f;
    }
    return QFileSystemModel::data(idx, role);
}

Qt::ItemFlags ExplorerModel::flags(const QModelIndex& idx) const {
    Qt::ItemFlags f = QFileSystemModel::flags(idx);
    if (idx.isValid() && ProjectExplorer::isProtectedPath(filePath(idx))) f &= ~(Qt::ItemIsDragEnabled | Qt::ItemIsEditable);
    return f;
}

// ===========================================================================
//  Opérations sur les fichiers
// ===========================================================================
bool ProjectExplorer::isValidName(const QString& name, QString* why) {
    auto bad = [&](const QString& m) { if (why) *why = m; return false; };
    const QString n = name.trimmed();
    if (n.isEmpty()) return bad("Le nom est vide.");
    if (n == "." || n == "..") return bad("Nom non autorisé.");
    if (n.size() > 120) return bad("Le nom est trop long.");
    static const QString forbidden = "\\/:*?\"<>|";
    for (const QChar c : n)
        if (forbidden.contains(c) || c.unicode() < 32) return bad("Le nom ne peut pas contenir  \\ / : * ? \" < > |");
    if (n.endsWith('.') || name.endsWith(' ')) return bad("Le nom ne peut pas finir par un point ou une espace.");
    static const QStringList reserved = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6",
                                         "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7",
                                         "LPT8", "LPT9"};
    if (reserved.contains(n.section('.', 0, 0).toUpper())) return bad("Ce nom est réservé par Windows.");
    return true;
}

QString ProjectExplorer::uniquePath(const QString& dir, const QString& baseName, const QString& ext) {
    QDir d(dir);
    QString p = d.filePath(baseName + ext);
    for (int i = 2; QFileInfo::exists(p); ++i) p = d.filePath(QString("%1 %2%3").arg(baseName).arg(i).arg(ext));
    return p;
}

bool ProjectExplorer::samePath(const QString& a, const QString& b) {
    if (a.isEmpty() || b.isEmpty()) return false;
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    return QDir::cleanPath(a).compare(QDir::cleanPath(b), cs) == 0;
}

bool ProjectExplorer::isUnder(const QString& path, const QString& folder) {
    if (samePath(path, folder)) return true;
    QString f = QDir::cleanPath(folder);
    if (!f.endsWith('/')) f += '/';
#ifdef Q_OS_WIN
    return QDir::cleanPath(path).startsWith(f, Qt::CaseInsensitive);
#else
    return QDir::cleanPath(path).startsWith(f);
#endif
}

bool ProjectExplorer::isProtectedPath(const QString& path) {
    if (path.isEmpty()) return true;
    if (QFileInfo(path).isRoot()) return true;                               // C:/ , D:/ , /
    if (isUnder(QDir::homePath(), path)) return true;                       // dossier personnel et ses parents (C:/Users)
    for (const char* var : {"SystemRoot", "ProgramFiles", "ProgramFiles(x86)", "ProgramData"}) {
        const QString dir = qEnvironmentVariable(var);
        if (!dir.isEmpty() && isUnder(path, dir)) return true;               // Windows, Program Files…
    }
#ifndef Q_OS_WIN
    for (const char* dir : {"/bin", "/boot", "/dev", "/etc", "/lib", "/proc", "/sbin", "/sys", "/usr", "/var"})
        if (isUnder(path, QString::fromLatin1(dir))) return true;
#endif
    return false;
}

bool ProjectExplorer::createFolder(const QString& path, QString* err) {
    if (QFileInfo::exists(path)) { if (err) *err = "Ce nom existe déjà."; return false; }
    if (!QDir().mkpath(path)) { if (err) *err = "Impossible de créer le dossier."; return false; }
    return true;
}

bool ProjectExplorer::createProgram(const QString& path, QString* err) {
    if (QFileInfo::exists(path)) { if (err) *err = "Ce nom existe déjà."; return false; }
    const QJsonObject root{{"app", "Blocs"}, {"version", 2},
                           {"nodes", QJsonArray{QJsonObject{{"type", "start"}, {"x", 0}, {"y", 0}, {"params", QJsonObject()}}}},
                           {"links", QJsonArray()}};
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { if (err) *err = "Impossible d'écrire le fichier."; return false; }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ProjectExplorer::createTextFile(const QString& path, QString* err) {
    if (QFileInfo::exists(path)) { if (err) *err = "Ce nom existe déjà."; return false; }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { if (err) *err = "Impossible d'écrire le fichier."; return false; }
    return true;
}

bool ProjectExplorer::copyRecursively(const QString& src, const QString& dst, QString* err) {
    const QFileInfo si(src);
    if (si.isDir()) {
        if (isUnder(dst, src)) { if (err) *err = "On ne peut pas copier un dossier dans lui-même."; return false; }
        if (!QDir().mkpath(dst)) { if (err) *err = "Impossible de créer " + dst; return false; }
        for (const QFileInfo& e : QDir(src).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden))
            if (!copyRecursively(e.filePath(), QDir(dst).filePath(e.fileName()), err)) return false;
        return true;
    }
    if (!QFile::copy(src, dst)) { if (err) *err = "Copie impossible : " + src; return false; }
    return true;
}

QString ProjectExplorer::duplicatePath(const QString& path, QString* err) {
    const QFileInfo fi(path);
    const QString ext = fi.isDir() ? QString() : (fi.suffix().isEmpty() ? QString() : "." + fi.suffix());
    const QString base = fi.isDir() ? fi.fileName() : fi.completeBaseName();
    const QString dst = uniquePath(fi.absolutePath(), base + " - copie", ext);
    return copyRecursively(path, dst, err) ? dst : QString();
}

bool ProjectExplorer::trashPath(const QString& path, QString* trashed) {
    return QFile::moveToTrash(path, trashed);
}

// ===========================================================================
//  Interface
// ===========================================================================
ProjectExplorer::ProjectExplorer(QWidget* parent) : QWidget(parent) { buildUi(); }

void ProjectExplorer::buildUi() {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // --- barre d'en-tête (nom du dossier + boutons), comme dans VS Code
    auto* header = new QWidget;
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(8, 4, 4, 4);
    title_ = new QLabel;
    title_->setStyleSheet("font-weight:bold; color:#b4b9cc;");
    hl->addWidget(title_, 1);

    auto mkBtn = [&](const QString& text, const QString& tip) {
        auto* b = new QToolButton;
        b->setText(text);
        b->setToolTip(tip);
        b->setAutoRaise(true);
        hl->addWidget(b);
        return b;
    };
    auto* add = mkBtn("＋", "Nouveau…");
    add->setPopupMode(QToolButton::InstantPopup);
    auto* addMenu = new QMenu(add);
    addMenu->addAction("Nouveau programme Blocs…", this, [this] { newProgram(); });
    addMenu->addAction("Nouveau dossier…", this, [this] { newFolder(); });
    addMenu->addAction("Nouveau fichier texte…", this, [this] { newTextFile(); });
    addMenu->addSeparator();
    addMenu->addAction("Importer des fichiers…", this, [this] { importFiles(); });
    add->setMenu(addMenu);
    connect(mkBtn("📁", "Ouvrir un dossier de projet…"), &QToolButton::clicked, this, &ProjectExplorer::openFolderRequested);
    connect(mkBtn("🖥", "Parcourir tout l'ordinateur (C:, D:…)"), &QToolButton::clicked, this,
            &ProjectExplorer::browseComputerRequested);
    connect(mkBtn("↻", "Actualiser"), &QToolButton::clicked, this, [this] { refresh(); });
    connect(mkBtn("⊟", "Réduire tous les dossiers"), &QToolButton::clicked, this, [this] { tree_->collapseAll(); });
    lay->addWidget(header);

    stack_ = new QStackedWidget;
    lay->addWidget(stack_, 1);

    // --- page « aucun dossier ouvert »
    auto* empty = new QWidget;
    auto* el = new QVBoxLayout(empty);
    el->setContentsMargins(12, 16, 12, 12);
    auto* msg = new QLabel("Aucun dossier ouvert.\n\nOuvre le dossier de ton projet pour voir et organiser tes programmes, images et sons.");
    msg->setWordWrap(true);
    msg->setStyleSheet("color:#9aa0b4;");
    auto* bOpen = new QPushButton("Ouvrir un dossier…");
    auto* bNew = new QPushButton("Nouveau projet…");
    el->addWidget(msg);
    el->addWidget(bOpen);
    el->addWidget(bNew);
    el->addStretch(1);
    connect(bOpen, &QPushButton::clicked, this, &ProjectExplorer::openFolderRequested);
    connect(bNew, &QPushButton::clicked, this, &ProjectExplorer::newProjectRequested);
    stack_->addWidget(empty);

    // --- l'arborescence
    model_ = new ExplorerModel(this);
    model_->setReadOnly(false);
    model_->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    tree_ = new QTreeView;
    tree_->setModel(model_);
    tree_->setHeaderHidden(true);
    for (int c = 1; c < 4; ++c) tree_->hideColumn(c);
    tree_->setUniformRowHeights(true);
    tree_->setEditTriggers(QAbstractItemView::EditKeyPressed);        // F2 : renommer sur place
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setDragEnabled(true);
    tree_->setAcceptDrops(true);
    tree_->setDropIndicatorShown(true);
    tree_->setDragDropMode(QAbstractItemView::DragDrop);
    tree_->setDefaultDropAction(Qt::MoveAction);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setIndentation(14);
    tree_->setAnimated(true);
    tree_->sortByColumn(0, Qt::AscendingOrder);
    tree_->installEventFilter(this);
    setMinimumWidth(200);
    stack_->addWidget(tree_);
    stack_->setCurrentIndex(0);

    connect(tree_, &QTreeView::customContextMenuRequested, this, [this](const QPoint& p) { showContextMenu(p); });
    connect(tree_, &QTreeView::activated, this, [this](const QModelIndex& i) { activatePath(model_->filePath(i)); });
    connect(model_, &QFileSystemModel::fileRenamed, this, [this](const QString& dir, const QString& o, const QString& n) {
        emit pathRenamed(QDir(dir).filePath(o), QDir(dir).filePath(n));
    });
    setRootPath(QString());
}

void ProjectExplorer::setRootPath(const QString& dir) {
    if (dir.isEmpty() || !QDir(dir).exists()) {
        // Pas de dossier de projet : vrai explorateur de l'ordinateur (lecteurs C:, D:… sous Windows, « / » ailleurs).
        root_.clear();
        model_->setRootPath(QString());
        tree_->setRootIndex(QModelIndex());
        title_->setText("CE PC");
        title_->setToolTip("Tous les fichiers de l'ordinateur. Clic droit sur un dossier : « Ouvrir comme dossier de projet ».");
        stack_->setCurrentIndex(1);
        revealHome();
        return;
    }
    root_ = dir;
    const QModelIndex idx = model_->setRootPath(dir);
    tree_->setRootIndex(idx);
    title_->setText(QFileInfo(dir).fileName().isEmpty() ? dir : QFileInfo(dir).fileName().toUpper());
    title_->setToolTip(QDir::toNativeSeparators(dir));
    stack_->setCurrentIndex(1);
}

// Déplie l'arborescence jusqu'au dossier personnel de l'utilisateur (C:\Users\nom), pour ne pas démarrer « à vide ».
void ProjectExplorer::revealHome() {
    QTimer::singleShot(300, this, [this] {
        if (!root_.isEmpty()) return;
        const QModelIndex home = model_->index(QDir::homePath());
        if (!home.isValid()) return;
        for (QModelIndex p = home; p.isValid(); p = p.parent()) tree_->expand(p);
        tree_->setCurrentIndex(home);
        tree_->scrollTo(home, QAbstractItemView::PositionAtCenter);
    });
}

void ProjectExplorer::setCurrentFile(const QString& path) { model_->setCurrent(path); }

void ProjectExplorer::refresh() {
    const QString r = root_;
    model_->setRootPath(QString());   // force la relecture du dossier
    setRootPath(r);
}

QStringList ProjectExplorer::selectedPaths() const {
    QStringList res;
    for (const QModelIndex& i : tree_->selectionModel()->selectedRows(0)) res << model_->filePath(i);
    return res;
}

QString ProjectExplorer::targetDir() const {
    const QStringList sel = selectedPaths();
    if (sel.isEmpty()) return root_;
    const QFileInfo fi(sel.first());
    return fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
}

void ProjectExplorer::selectLater(const QString& path) {
    for (int ms : {150, 600}) {
        QTimer::singleShot(ms, this, [this, path] {
            const QModelIndex i = model_->index(path);
            if (!i.isValid()) return;
            tree_->setCurrentIndex(i);
            tree_->scrollTo(i);
        });
    }
}

// ---------------------------------------------------------------- ouverture
void ProjectExplorer::activatePath(const QString& path) {
    const QFileInfo fi(path);
    if (fi.isDir()) return;                                   // le double-clic déplie/replie
    const QString ext = fi.suffix().toLower();
    if (ext == "blocs") emit openProgramRequested(path);
    else if (QStringList{"png", "jpg", "jpeg", "gif", "bmp"}.contains(ext)) previewImage(path);
    else QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void ProjectExplorer::previewImage(const QString& path) {
    QPixmap pm(path);
    auto* dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(QFileInfo(path).fileName());
    auto* lay = new QVBoxLayout(dlg);
    auto* img = new QLabel;
    img->setAlignment(Qt::AlignCenter);
    img->setStyleSheet("background:#1c1d24;");
    img->setPixmap(pm.isNull() ? QPixmap() : pm.scaled(560, 420, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    if (pm.isNull()) img->setText("Image illisible");
    lay->addWidget(img);
    lay->addWidget(new QLabel(QString("%1 × %2 pixels").arg(pm.width()).arg(pm.height())));
    auto* row = new QHBoxLayout;
    auto* imp = new QPushButton("Importer comme ressource du projet");
    auto* close = new QPushButton("Fermer");
    row->addWidget(imp);
    row->addStretch(1);
    row->addWidget(close);
    lay->addLayout(row);
    connect(imp, &QPushButton::clicked, dlg, [this, dlg, path] { emit importAssetRequested(path); dlg->close(); });
    connect(close, &QPushButton::clicked, dlg, &QDialog::close);
    dlg->show();
}

// ---------------------------------------------------------------- créations
void ProjectExplorer::newProgram() {
    const QString dir = targetDir();
    if (dir.isEmpty()) { emit message("Sélectionne d'abord un dossier."); return; }
    bool ok = false;
    QString name = QInputDialog::getText(this, "Nouveau programme", "Nom du programme :", QLineEdit::Normal,
                                         QFileInfo(uniquePath(dir, "Programme", ".blocs")).completeBaseName(), &ok).trimmed();
    if (!ok) return;
    if (name.endsWith(".blocs", Qt::CaseInsensitive)) name.chop(6);
    QString why;
    if (!isValidName(name, &why)) { QMessageBox::warning(this, "Nom invalide", why); return; }
    const QString path = QDir(dir).filePath(name + ".blocs");
    QString err;
    if (!createProgram(path, &err)) { QMessageBox::warning(this, "Impossible de créer", err); return; }
    selectLater(path);
    emit openProgramRequested(path);
}

void ProjectExplorer::newFolder() {
    const QString dir = targetDir();
    if (dir.isEmpty()) { emit message("Sélectionne d'abord un dossier."); return; }
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Nouveau dossier", "Nom du dossier :", QLineEdit::Normal,
                                               QFileInfo(uniquePath(dir, "Nouveau dossier")).fileName(), &ok).trimmed();
    if (!ok) return;
    QString why;
    if (!isValidName(name, &why)) { QMessageBox::warning(this, "Nom invalide", why); return; }
    const QString path = QDir(dir).filePath(name);
    if (!createFolder(path, &why)) { QMessageBox::warning(this, "Impossible de créer", why); return; }
    tree_->expand(model_->index(dir));
    selectLater(path);
}

void ProjectExplorer::newTextFile() {
    const QString dir = targetDir();
    if (dir.isEmpty()) { emit message("Sélectionne d'abord un dossier."); return; }
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Nouveau fichier", "Nom du fichier (avec son extension) :", QLineEdit::Normal,
                                               QFileInfo(uniquePath(dir, "notes", ".txt")).fileName(), &ok).trimmed();
    if (!ok) return;
    QString why;
    if (!isValidName(name, &why)) { QMessageBox::warning(this, "Nom invalide", why); return; }
    const QString path = QDir(dir).filePath(name);
    if (!createTextFile(path, &why)) { QMessageBox::warning(this, "Impossible de créer", why); return; }
    tree_->expand(model_->index(dir));
    selectLater(path);
}

void ProjectExplorer::importFiles() {
    const QString dir = targetDir();
    if (dir.isEmpty()) { emit message("Sélectionne d'abord un dossier."); return; }
    const QStringList files = QFileDialog::getOpenFileNames(this, "Importer des fichiers dans le projet");
    int n = 0;
    for (const QString& f : files) {
        const QFileInfo fi(f);
        const QString ext = fi.suffix().isEmpty() ? QString() : "." + fi.suffix();
        const QString dst = QFileInfo::exists(QDir(dir).filePath(fi.fileName())) ? uniquePath(dir, fi.completeBaseName(), ext)
                                                                                : QDir(dir).filePath(fi.fileName());
        if (QFile::copy(f, dst)) ++n;
    }
    if (n) emit message(QString("%1 fichier(s) importé(s).").arg(n));
}

// ---------------------------------------------------------------- modifications
void ProjectExplorer::renameSelected() {
    const QModelIndex i = tree_->currentIndex();
    if (!i.isValid()) return;
    if (isProtectedPath(model_->filePath(i))) { emit message("Ce dossier système ne peut pas être renommé ici."); return; }
    tree_->edit(i);
}

void ProjectExplorer::duplicateSelected() {
    for (const QString& p : selectedPaths()) {
        QString err;
        const QString d = duplicatePath(p, &err);
        if (d.isEmpty()) QMessageBox::warning(this, "Duplication impossible", err);
        else selectLater(d);
    }
}

void ProjectExplorer::deleteSelected() {
    const QStringList sel = selectedPaths();
    if (sel.isEmpty()) return;
    for (const QString& p : sel)
        if (isProtectedPath(p)) {
            QMessageBox::warning(this, "Suppression refusée",
                                 QString("« %1 » est un lecteur ou un dossier système / personnel : il ne peut pas être supprimé depuis Blocs.")
                                     .arg(QDir::toNativeSeparators(p)));
            return;
        }
    const QString what = sel.size() == 1 ? QString("« %1 »").arg(QFileInfo(sel.first()).fileName())
                                         : QString("ces %1 éléments").arg(sel.size());
    if (QMessageBox::question(this, "Supprimer", QString("Mettre %1 à la corbeille ?").arg(what)) != QMessageBox::Yes) return;
    for (const QString& p : sel) {
        if (!trashPath(p)) {
            if (QMessageBox::question(this, "Corbeille indisponible",
                                      QString("« %1 » ne peut pas aller à la corbeille. Le supprimer définitivement ?")
                                          .arg(QFileInfo(p).fileName())) != QMessageBox::Yes)
                continue;
            if (QFileInfo(p).isDir()) QDir(p).removeRecursively();
            else QFile::remove(p);
        }
        emit pathRemoved(p);
    }
}

void ProjectExplorer::copySelected(bool cut) {
    clip_ = selectedPaths();
    if (cut)
        for (const QString& p : std::as_const(clip_))
            if (isProtectedPath(p)) { clip_.clear(); emit message("Ce dossier ne peut pas être déplacé depuis Blocs."); return; }
    clipCut_ = cut;
    if (!clip_.isEmpty()) emit message(QString("%1 élément(s) %2.").arg(clip_.size()).arg(cut ? "coupé(s)" : "copié(s)"));
}

void ProjectExplorer::paste() {
    const QString dir = targetDir();
    if (clip_.isEmpty() || dir.isEmpty()) return;
    for (const QString& src : std::as_const(clip_)) {
        const QFileInfo fi(src);
        if (!fi.exists()) continue;
        const QString ext = fi.isDir() || fi.suffix().isEmpty() ? QString() : "." + fi.suffix();
        const QString base = fi.isDir() ? fi.fileName() : fi.completeBaseName();
        QString dst = QDir(dir).filePath(fi.fileName());
        if (QFileInfo::exists(dst)) {
            if (clipCut_ && samePath(dst, src)) continue;            // couper-coller au même endroit : rien à faire
            dst = uniquePath(dir, base + (clipCut_ ? QString() : " - copie"), ext);
        }
        QString err;
        if (clipCut_) {
            if (fi.isDir() && isUnder(dir, src)) { QMessageBox::warning(this, "Déplacement impossible", "Un dossier ne peut pas être déplacé dans lui-même."); continue; }
            if (QFile::rename(src, dst) || (copyRecursively(src, dst, &err) &&
                                            (fi.isDir() ? QDir(src).removeRecursively() : QFile::remove(src))))
                emit pathRenamed(src, dst);
            else QMessageBox::warning(this, "Déplacement impossible", err);
        } else if (!copyRecursively(src, dst, &err)) {
            QMessageBox::warning(this, "Copie impossible", err);
        }
        selectLater(dst);
    }
    if (clipCut_) clip_.clear();
}

// ---------------------------------------------------------------- menu et clavier
void ProjectExplorer::showContextMenu(const QPoint& pos) {
    const QModelIndex idx = tree_->indexAt(pos);
    if (idx.isValid() && !tree_->selectionModel()->isSelected(idx)) {
        tree_->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        tree_->setCurrentIndex(idx);
    }
    if (!idx.isValid()) tree_->clearSelection();
    const QStringList sel = selectedPaths();
    const QString one = sel.size() == 1 ? sel.first() : QString();
    const QString ext = QFileInfo(one).suffix().toLower();

    QMenu menu(this);
    if (!one.isEmpty() && QFileInfo(one).isDir() && !samePath(one, root_))
        menu.addAction("Ouvrir comme dossier de projet", this, [this, one] { emit openFolderPathRequested(one); });
    if (!one.isEmpty() && !QFileInfo(one).isDir()) {
        if (ext == "blocs") menu.addAction("Ouvrir dans Blocs", this, [this, one] { emit openProgramRequested(one); });
        else menu.addAction("Ouvrir", this, [this, one] { activatePath(one); });
        if (QStringList{"png", "jpg", "jpeg", "gif", "bmp", "wav"}.contains(ext))
            menu.addAction("Importer comme ressource du projet", this, [this, one] { emit importAssetRequested(one); });
        menu.addSeparator();
    }
    menu.addAction("Nouveau programme Blocs…", this, [this] { newProgram(); });
    menu.addAction("Nouveau dossier…", this, [this] { newFolder(); });
    menu.addAction("Nouveau fichier texte…", this, [this] { newTextFile(); });
    menu.addAction("Importer des fichiers…", this, [this] { importFiles(); });
    menu.addSeparator();
    if (!sel.isEmpty()) {
        menu.addAction("Couper", QKeySequence::Cut, this, [this] { copySelected(true); });
        menu.addAction("Copier", QKeySequence::Copy, this, [this] { copySelected(false); });
    }
    QAction* pa = menu.addAction("Coller", QKeySequence::Paste, this, [this] { paste(); });
    pa->setEnabled(!clip_.isEmpty());
    if (!sel.isEmpty()) {
        menu.addAction("Dupliquer", this, [this] { duplicateSelected(); });
        menu.addSeparator();
        QAction* ra = menu.addAction("Renommer", QKeySequence(Qt::Key_F2), this, [this] { renameSelected(); });
        ra->setEnabled(sel.size() == 1);
        menu.addAction("Supprimer", QKeySequence::Delete, this, [this] { deleteSelected(); });
        menu.addSeparator();
    }
    if (!one.isEmpty()) {
        menu.addAction("Copier le chemin", this, [one] { QApplication::clipboard()->setText(QDir::toNativeSeparators(one)); });
        menu.addAction("Afficher dans l'explorateur de fichiers", this, [one] {
            const QFileInfo fi(one);
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath()));
        });
    } else if (!root_.isEmpty()) {
        menu.addAction("Afficher le dossier dans l'explorateur de fichiers", this,
                       [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(root_)); });
    }
    menu.addAction("Actualiser", this, [this] { refresh(); });
    menu.exec(tree_->viewport()->mapToGlobal(pos));
}

bool ProjectExplorer::eventFilter(QObject* obj, QEvent* e) {
    if (obj == tree_ && (e->type() == QEvent::ShortcutOverride || e->type() == QEvent::KeyPress)) {
        auto* ke = static_cast<QKeyEvent*>(e);
        const bool ctrl = ke->modifiers() == Qt::ControlModifier;
        const bool del = ke->key() == Qt::Key_Delete && ke->modifiers() == Qt::NoModifier;
        const bool copy = ctrl && ke->key() == Qt::Key_C, cut = ctrl && ke->key() == Qt::Key_X, pas = ctrl && ke->key() == Qt::Key_V;
        if (del || copy || cut || pas) {
            if (e->type() == QEvent::ShortcutOverride) { e->accept(); return true; }   // on garde la touche pour l'explorateur
            if (del) deleteSelected();
            else if (copy) copySelected(false);
            else if (cut) copySelected(true);
            else paste();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}
