#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QUrl>
#include <QFontDatabase>
#include <QCursor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QStatusBar>
#include <QTextCursor>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "codegen.h"
#include "editor.h"
#include "assetsdialog.h"
#include "examples.h"
#include "explorer.h"
#include "exporter.h"
#include "gamewindow.h"
#include "highlighter.h"
#include "runner.h"
#include "tutorialdialog.h"
#include "tutorials.h"

MainWindow::MainWindow() {
    resize(1400, 860);
    scene_ = new NodeScene(this);
    view_ = new NodeEditorView(scene_);
    setCentralWidget(view_);

    runner_ = new Runner(this);
    exporter_ = new Exporter(this);
    snapTimer_ = new QTimer(this);
    snapTimer_->setSingleShot(true);
    snapTimer_->setInterval(400);          // on regroupe les modifications rapprochées en une seule étape
    connect(snapTimer_, &QTimer::timeout, this, [this] { commitSnapshot(); });

    buildPalette();
    buildExplorer();
    buildDocks();
    buildConsole();
    buildMenus();

    connect(view_, &NodeEditorView::contextMenuWanted, this,
            [this](const QPoint& g, const QPointF& s) { showContextMenu(g, s); });
    connect(runner_, &Runner::output, this, [this](const QString& t, int k) { appendConsole(t, k); });
    connect(runner_, &Runner::gameData, this, [this](const QString& payload) {
        if (!game_) {                       // la fenêtre de jeu est créée à la première image reçue
            game_ = new GameWindow(this);
            game_->setAssets(scene_->assets());
            connect(game_, &GameWindow::inputEvent, this, [this](const QString& line) { runner_->sendInput(line); });
            connect(game_, &GameWindow::closedByUser, this, [this] { runner_->stop(); });
        }
        game_->handle(payload);
    });
    connect(runner_, &Runner::runningChanged, this, [this](bool running) {
        if (game_) game_->setFinished(!running);
        runAct_->setEnabled(!running);
        stopAct_->setEnabled(running);
        consoleInput_->setEnabled(running);
        if (running) consoleInput_->setFocus();
    });

    connect(scene_, &NodeScene::structureChanged, this,
            [this] { setWindowModified(true); scheduleRefresh(true); scheduleSnapshot(); });
    connect(scene_, &NodeScene::paramsChanged, this,
            [this] { setWindowModified(true); scheduleRefresh(false); scheduleSnapshot(); });
    connect(scene_, &NodeScene::layoutChanged, this, [this] { setWindowModified(true); scheduleSnapshot(); });
    connect(scene_, &QGraphicsScene::selectionChanged, this, [this] { scheduleRefresh(true); });
    connect(scene_, &NodeScene::assetsChanged, this, [this] { setWindowModified(true); scheduleRefresh(true); });
    connect(exporter_, &Exporter::output, this, [this](const QString& t, int k) { appendConsole(t, k); });
    connect(exporter_, &Exporter::finished, this, [this](bool ok, const QString& path) {
        exportAct_->setEnabled(true);
        if (ok && !path.isEmpty()) {
            statusBar()->showMessage("Export terminé : " + QDir::toNativeSeparators(path), 8000);
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });
    connect(scene_, &NodeScene::message, this, [this](const QString& m) { statusBar()->showMessage(m, 6000); });

    statusBar()->showMessage(
        "Clic droit ou double-clic : menu  •  Glisser d'un port à un autre pour relier  •  "
        "Molette : zoom  •  Clic droit + glisser : déplacer la vue  •  F5 : exécuter  •  F1 : aide et tutoriels");
    resizeDocks({paletteDock_, explorerDock_}, {250, 250}, Qt::Horizontal);   // panneau de gauche : largeur raisonnable
    rebuildRecentMenu();
    const QString saved = QSettings("Blocs", "Blocs").value("workspace").toString();
    if (!saved.isEmpty() && QDir(saved).exists()) setWorkspace(saved);       // on retrouve son dossier de projet
    loadDemo();
}

// ---------------------------------------------------------------------------
//  Interface
// ---------------------------------------------------------------------------
void MainWindow::buildPalette() {
    auto* dock = new QDockWidget("Blocs", this);
    paletteDock_ = dock;
    dock->setFeatures(QDockWidget::DockWidgetMovable);
    auto* tree = new QTreeWidget;
    tree->setHeaderHidden(true);
    tree->setMinimumWidth(200);
    tree->setIndentation(12);

    QMap<QString, QTreeWidgetItem*> cats;
    for (const NodeDef& d : nodeCatalog()) {
        if (!cats.contains(d.category)) {
            auto* c = new QTreeWidgetItem(tree, QStringList(d.category));
            QFont f = c->font(0);
            f.setBold(true);
            c->setFont(0, f);
            c->setFlags(Qt::ItemIsEnabled);
            cats[d.category] = c;
        }
        auto* it = new QTreeWidgetItem(cats[d.category], QStringList(d.title));
        it->setData(0, Qt::UserRole, d.id);
        it->setToolTip(0, d.help);
    }
    tree->expandAll();

    connect(tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* it, int) {
        const QString id = it->data(0, Qt::UserRole).toString();
        if (id.isEmpty()) return;
        const QPointF c = view_->mapToScene(view_->viewport()->rect().center());
        spawn_ = (spawn_ + 1) % 8;
        scene_->addNode(id, c + QPointF(-100 + spawn_ * 20, -40 + spawn_ * 20));
    });

    dock->setWidget(tree);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}

void MainWindow::buildDocks() {
    // --- Propriétés
    auto* propsDock = new QDockWidget("Propriétés", this);
    propsScroll_ = new QScrollArea;
    propsScroll_->setWidgetResizable(true);
    propsScroll_->setFrameShape(QFrame::NoFrame);
    propsDock->setWidget(propsScroll_);
    addDockWidget(Qt::RightDockWidgetArea, propsDock);

    // --- Code C#
    auto* codeDock = new QDockWidget("Code C# généré", this);
    auto* box = new QWidget;
    auto* lay = new QVBoxLayout(box);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    code_ = new QPlainTextEdit;
    code_->setReadOnly(true);
    code_->setLineWrapMode(QPlainTextEdit::NoWrap);
    code_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    code_->setStyleSheet("QPlainTextEdit { background:#12131a; color:#d4d4d4; border:none; }");
    new CSharpHighlighter(code_->document());
    lay->addWidget(code_, 1);

    problems_ = new QListWidget;
    problems_->setMaximumHeight(110);
    problems_->setStyleSheet("QListWidget { background:#2a1c1c; color:#ff9e9e; border:none; }");
    problems_->setWordWrap(true);
    problems_->hide();
    lay->addWidget(problems_);

    codeDock->setWidget(box);
    addDockWidget(Qt::RightDockWidgetArea, codeDock);
    splitDockWidget(propsDock, codeDock, Qt::Vertical);
    resizeDocks({propsDock, codeDock}, {270, 560}, Qt::Vertical);
    resizeDocks({propsDock}, {430}, Qt::Horizontal);
}

void MainWindow::buildConsole() {
    consoleDock_ = new QDockWidget("Console", this);
    auto* box = new QWidget;
    auto* lay = new QVBoxLayout(box);
    lay->setContentsMargins(0, 0, 0, 4);
    lay->setSpacing(4);

    console_ = new QPlainTextEdit;
    console_->setReadOnly(true);
    console_->setMaximumBlockCount(5000);
    console_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    console_->setStyleSheet("QPlainTextEdit { background:#0e0f14; color:#d4d4d4; border:none; }");
    lay->addWidget(console_, 1);

    auto* row = new QHBoxLayout;
    row->setContentsMargins(4, 0, 4, 0);
    consoleInput_ = new QLineEdit;
    consoleInput_->setPlaceholderText("Réponse au programme (Console.ReadLine) — Entrée pour envoyer");
    consoleInput_->setEnabled(false);
    auto* send = new QPushButton("Envoyer");
    auto* clear = new QPushButton("Effacer");
    row->addWidget(consoleInput_, 1);
    row->addWidget(send);
    row->addWidget(clear);
    lay->addLayout(row);

    connect(consoleInput_, &QLineEdit::returnPressed, this, [this] { sendConsoleInput(); });
    connect(send, &QPushButton::clicked, this, [this] { sendConsoleInput(); });
    connect(clear, &QPushButton::clicked, console_, &QPlainTextEdit::clear);

    consoleDock_->setWidget(box);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock_);
    // la console se place sous la zone centrale, pas sous les panneaux latéraux
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    resizeDocks({consoleDock_}, {200}, Qt::Vertical);
}

void MainWindow::appendConsole(const QString& text_, int kind) {
    static const char* colors[] = {"#d4d4d4", "#ff8a80", "#7f87a1", "#81c784"};
    QString text = text_;
    if (text.contains('\f')) {                  // « Effacer l'écran » du programme
        console_->clear();
        text = text.mid(text.lastIndexOf('\f') + 1);
    }
    QTextCharFormat fmt;
    fmt.setForeground(QColor(colors[qBound(0, kind, 3)]));
    QTextCursor cur = console_->textCursor();
    cur.movePosition(QTextCursor::End);
    cur.insertText(text, fmt);
    console_->setTextCursor(cur);
    console_->ensureCursorVisible();
}

void MainWindow::sendConsoleInput() {
    if (!runner_->isRunning()) return;
    const QString t = consoleInput_->text();
    consoleInput_->clear();
    appendConsole(t + "\n", Runner::UserInput);
    runner_->sendInput(t);
}

// Images / sons cités dans les blocs mais absents des ressources du projet
QStringList MainWindow::missingAssetWarnings() const {
    QStringList res;
    for (NodeItem* n : scene_->nodeItems()) {
        const QString id = n->def().id;
        if (id != "g_image" && id != "g_sound") continue;
        const QString key = id == "g_image" ? "img" : "snd";
        const PortItem* p = n->port(PortDir::In, key);
        if (p && p->isConnected()) continue;
        const QString name = n->param(key).trimmed();
        bool found = false;
        for (auto it = scene_->assets().begin(); it != scene_->assets().end(); ++it)
            if (it.key().compare(name, Qt::CaseInsensitive) == 0 && it.value().isImage() == (id == "g_image")) found = true;
        const QString w = QString("%1 « %2 » introuvable : importe-le avec le menu Jeu > Importer…")
                              .arg(id == "g_image" ? "Image" : "Son", name);
        if (!found && !res.contains(w)) res << w;
    }
    return res;
}

void MainWindow::runProgram() {
    if (runner_->isRunning()) return;
    consoleDock_->show();
    console_->clear();
    if (game_) {                            // la fenêtre de jeu se rouvre toute seule si le programme en a besoin
        game_->hide();
        game_->setAssets(scene_->assets());
    }
    for (const QString& w : missingAssetWarnings()) appendConsole("⚠ " + w + "\n", Runner::Error);
    const CodeResult r = generateCSharp(*scene_, true);
    if (!r.code.contains("class Program")) {
        appendConsole("✖ Ajoute un bloc « ▶ Début » et relie-le à d'autres blocs.\n", Runner::Error);
        return;
    }
    if (!r.problems.isEmpty()) {
        appendConsole("✖ Corrige d'abord ces problèmes :\n", Runner::Error);
        for (const QString& p : r.problems) appendConsole("  • " + p + "\n", Runner::Error);
        return;
    }
    runner_->start(r.code);
}

void MainWindow::buildMenus() {
    QMenu* file = menuBar()->addMenu("&Fichier");
    file->addAction("&Nouveau", QKeySequence::New, this, [this] { newProject(); });
    file->addAction("&Ouvrir…", QKeySequence::Open, this, [this] { openFile(); });
    file->addAction("Ouvrir un &dossier…", QKeySequence("Ctrl+Shift+O"), this, [this] { openFolder(); });
    recentMenu_ = file->addMenu("Dossiers &récents");
    file->addAction("&Nouveau projet (dossier)…", this, [this] { newProjectFolder(); });
    file->addAction("&Fermer le dossier", this, [this] { setWorkspace(QString()); });
    file->addSeparator();
    QMenu* ex = file->addMenu("E&xemples");
    for (const ExampleInfo& e : examples())
        if (e.inMenu) {
            const int id = e.id;
            ex->addAction(QString::fromUtf8(e.name), this, [this, id] { loadExample(id); });
        }
    file->addAction("&Enregistrer", QKeySequence::Save, this, [this] { saveFile(false); });
    file->addAction("Enregistrer &sous…", QKeySequence::SaveAs, this, [this] { saveFile(true); });
    file->addSeparator();
    exportAct_ = file->addAction("Exporter l'&application (.exe)…", QKeySequence("Ctrl+Shift+B"), this, [this] { exportApplication(); });
    file->addAction("E&xporter le code C#…", QKeySequence("Ctrl+E"), this, [this] { exportCSharp(); });
    file->addAction("&Copier le code C#", QKeySequence("Ctrl+Shift+C"), this, [this] {
        QApplication::clipboard()->setText(lastCode_);
        statusBar()->showMessage("Code C# copié dans le presse-papiers.", 4000);
    });
    file->addSeparator();
    file->addAction("&Quitter", QKeySequence::Quit, this, &QWidget::close);

    QMenu* edit = menuBar()->addMenu("&Édition");
    undoAct_ = edit->addAction("&Annuler", QKeySequence::Undo, this, [this] { undo(); });
    redoAct_ = edit->addAction("&Rétablir", QKeySequence::Redo, this, [this] { redo(); });
    undoAct_->setEnabled(false);
    redoAct_->setEnabled(false);
    edit->addSeparator();
    edit->addAction("&Supprimer la sélection", QKeySequence::Delete, this, [this] { scene_->deleteSelection(); });
    edit->addAction("Cou&per", QKeySequence::Cut, this, [this] { cutSelection(); });
    edit->addAction("&Copier les blocs", QKeySequence::Copy, this, [this] { copySelection(); });
    edit->addAction("C&oller", QKeySequence::Paste, this, [this] { pasteAt(cursorScenePos()); });
    edit->addAction("&Dupliquer", QKeySequence("Ctrl+D"), this, [this] { scene_->duplicateSelection(); });
    edit->addAction("Tout &sélectionner", QKeySequence::SelectAll, this, [this] {
        for (QGraphicsItem* it : scene_->items()) it->setSelected(true);
    });

    runAct_ = new QAction("▶ Exécuter", this);
    runAct_->setShortcut(QKeySequence("F5"));
    connect(runAct_, &QAction::triggered, this, [this] { runProgram(); });
    stopAct_ = new QAction("■ Arrêter", this);
    stopAct_->setShortcut(QKeySequence("Shift+F5"));
    stopAct_->setEnabled(false);
    connect(stopAct_, &QAction::triggered, this, [this] { runner_->stop(); });
    QMenu* run = menuBar()->addMenu("E&xécution");
    run->addAction(runAct_);
    run->addAction(stopAct_);

    auto* tb = addToolBar("Exécution");
    tb->setMovable(false);
    tb->addAction(runAct_);
    tb->addAction(stopAct_);

    QMenu* game = menuBar()->addMenu("&Jeu");
    game->addAction("Importer une &image…", this, [this] { AssetsDialog::importFile(this, scene_, true); });
    game->addAction("Importer un &son (WAV)…", this, [this] { AssetsDialog::importFile(this, scene_, false); });
    game->addAction("&Ressources du projet…", this, [this] { (new AssetsDialog(scene_, this))->show(); });
    game->addSeparator();
    game->addAction("Exporter l'&application (.exe)…", this, [this] { exportApplication(); });

    QMenu* view = menuBar()->addMenu("&Affichage");
    view->addAction("&Zoom 100 %", QKeySequence("Ctrl+0"), this, [this] { view_->resetTransform(); });
    view->addAction("&Recentrer", QKeySequence("Home"), this, [this] { view_->centerOn(0, 0); });
    view->addAction("&Explorateur de fichiers", QKeySequence("Ctrl+Shift+E"), this, [this] {
        explorerDock_->show();
        explorerDock_->raise();
        explorer_->tree()->setFocus();
    });
    view->addAction("Liste des &blocs", this, [this] { paletteDock_->show(); paletteDock_->raise(); });
    view->addAction(consoleDock_->toggleViewAction());

    QMenu* help = menuBar()->addMenu("&Aide");
    help->addAction("&Tutoriels et aide…", QKeySequence::HelpContents, this, [this] { openHelp(0); });
    QMenu* tuts = help->addMenu("&Aller directement à…");
    for (int i = 0; i < tutorials().size(); ++i)
        tuts->addAction(tutorials()[i].title, this, [this, i] { openHelp(i); });
    help->addAction("&Référence des blocs…", this, [this] { showBlockReference(this); });
    help->addSeparator();
    help->addAction("Gestes et &raccourcis", this, [this] {
        QMessageBox::information(this, "Gestes et raccourcis",
            "• Clic sur un bloc de la liste : l'ajouter.\n"
            "• Glisser d'un rond à un autre : relier (▶ flux, ● données).\n"
            "• Clic droit ou double-clic : menu (ajouter, copier, coller, supprimer…).\n"
            "• Clic droit + glisser, ou molette enfoncée : déplacer la vue.   Molette : zoom.\n"
            "• Suppr : supprimer.   Ctrl+C / X / V : copier / couper / coller.   Ctrl+D : dupliquer.\n"
            "• Ctrl+Z / Ctrl+Y : annuler / rétablir.   Ctrl+S : enregistrer.\n"
            "• F5 : exécuter.   Maj+F5 : arrêter.   Ctrl+E : exporter le code .cs.   F1 : aide.\n"
            "• Ctrl+Maj+O : ouvrir un dossier.   Ctrl+Maj+E : explorateur de fichiers.   Ctrl+Maj+B : exporter l'application.\n"
            "• Dans l'explorateur : F2 renommer, Suppr supprimer (corbeille), Ctrl+C / X / V copier, couper, coller, clic droit pour le menu.");
    });
}

void MainWindow::openHelp(int tutorial) {
    if (!help_) {
        help_ = new TutorialDialog([this](int id) { loadExample(id); }, this);
        help_->setAttribute(Qt::WA_DeleteOnClose, false);
    }
    help_->showTutorial(tutorial);
}

void MainWindow::showWelcomeIfFirstRun() {
    QSettings st("Blocs", "Blocs");
    if (st.value("welcomeShown", false).toBool()) return;
    st.setValue("welcomeShown", true);
    openHelp(0);
}

// ---------------------------------------------------------------------------
//  Mises à jour (regroupées : un seul rafraîchissement par tour de boucle)
// ---------------------------------------------------------------------------
void MainWindow::scheduleRefresh(bool rebuildPanel) {
    // Une simple modification de valeur ne reconstruit PAS le panneau (sinon on perdrait le curseur en tapant)
    propsPending_ = propsPending_ || rebuildPanel;
    if (refreshPending_) return;
    refreshPending_ = true;
    QTimer::singleShot(0, this, [this] {
        refreshPending_ = false;
        if (propsPending_) rebuildProps();
        propsPending_ = false;
        refreshCode();
    });
}

void MainWindow::refreshCode() {
    const CodeResult r = generateCSharp(*scene_);
    lastCode_ = r.code;
    if (code_->toPlainText() != r.code) {
        const int v = code_->verticalScrollBar()->value();
        code_->setPlainText(r.code);
        code_->verticalScrollBar()->setValue(v);
    }
    problems_->clear();
    for (const QString& p : r.problems) problems_->addItem("⚠ " + p);
    problems_->setVisible(!r.problems.isEmpty());
}

void MainWindow::rebuildProps() {
    auto* inner = new QWidget;
    auto* lay = new QVBoxLayout(inner);
    lay->setAlignment(Qt::AlignTop);

    QList<NodeItem*> sel;
    for (QGraphicsItem* it : scene_->selectedItems())
        if (auto* n = qgraphicsitem_cast<NodeItem*>(it)) sel.append(n);

    if (sel.size() != 1) {
        auto* l = new QLabel(sel.isEmpty() ? "Sélectionne un bloc pour modifier ses valeurs."
                                           : "Sélectionne un seul bloc pour modifier ses valeurs.");
        l->setWordWrap(true);
        l->setStyleSheet("color:#888;");
        lay->addWidget(l);
        propsScroll_->setWidget(inner);
        return;
    }

    NodeItem* n = sel.first();
    const NodeDef& d = n->def();
    auto* title = new QLabel(d.title);
    QFont tf = title->font();
    tf.setBold(true);
    tf.setPointSize(tf.pointSize() + 2);
    title->setFont(tf);
    lay->addWidget(title);
    if (!d.help.isEmpty()) {                       // l'aide du bloc, toujours visible
        auto* help = new QLabel(d.help);
        help->setWordWrap(true);
        help->setStyleSheet("color:#b4b9cc; background:#2a2c38; padding:6px; border-radius:4px;");
        lay->addWidget(help);
    }

    auto heading = [&](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:#4fc3f7; margin-top:10px; font-weight:bold;");
        lay->addWidget(l);
    };
    auto lineEdit = [&](const QString& id) {
        auto* le = new QLineEdit(n->param(id));
        connect(le, &QLineEdit::textEdited, le, [this, n, id](const QString& t) {
            n->setParam(id, t);
            scene_->notifyParamsChanged();
        });
        return le;
    };

    auto codeEdit = [&](const QString& id) {         // champs « code » : plusieurs lignes
        auto* te = new QPlainTextEdit(n->param(id));
        te->setMinimumHeight(120);
        te->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        te->setLineWrapMode(QPlainTextEdit::NoWrap);
        connect(te, &QPlainTextEdit::textChanged, te, [this, n, id, te] {
            n->setParam(id, te->toPlainText());
            scene_->notifyParamsChanged();
        });
        return te;
    };

    if (!d.dataIns.isEmpty()) heading("Valeurs d'entrée");
    for (const DataIn& di : d.dataIns) {
        lay->addWidget(new QLabel(QString("%1  (%2)").arg(di.label, typeName(di.type))));
        const PortItem* p = n->port(PortDir::In, di.id);
        if (p && p->isConnected()) {
            auto* l = new QLabel("relié par un fil ✔");
            l->setStyleSheet("color:#81c784; font-style:italic;");
            lay->addWidget(l);
        } else if (di.type == DataType::Bool) {
            auto* cb = new QComboBox;
            cb->addItems({"true", "false"});
            cb->setCurrentText(n->param(di.id) == "false" ? "false" : "true");
            const QString id = di.id;
            connect(cb, &QComboBox::currentTextChanged, cb, [this, n, id](const QString& t) {
                n->setParam(id, t);
                scene_->notifyParamsChanged();
            });
            lay->addWidget(cb);
        } else {
            auto* le = lineEdit(di.id);
            lay->addWidget(le);
            if (di.id == "img" || di.id == "snd") {         // aide au choix d'une ressource importée
                auto* cb = new QComboBox;
                cb->addItem("— choisir une ressource importée —");
                for (auto it = scene_->assets().begin(); it != scene_->assets().end(); ++it)
                    if (it.value().isImage() == (di.id == "img")) cb->addItem(it.key());
                if (cb->count() == 1) cb->setItemText(0, "(aucune : menu Jeu > Importer…)");
                const QString id = di.id;
                connect(cb, &QComboBox::activated, cb, [this, n, id, le, cb](int i) {
                    if (i <= 0) return;
                    le->setText(cb->itemText(i));
                    n->setParam(id, cb->itemText(i));
                    scene_->notifyParamsChanged();
                });
                lay->addWidget(cb);
            }
        }
    }

    if (!d.fields.isEmpty()) heading("Réglages");
    for (const FieldDef& f : d.fields) {
        lay->addWidget(new QLabel(f.label));
        if (!f.options.isEmpty()) {
            auto* cb = new QComboBox;
            cb->addItems(f.options);
            cb->setCurrentText(n->param(f.id));
            const QString id = f.id;
            connect(cb, &QComboBox::currentTextChanged, cb, [this, n, id](const QString& t) {
                n->setParam(id, t);
                scene_->notifyParamsChanged();
            });
            lay->addWidget(cb);
        } else if (f.id == "code") {
            lay->addWidget(codeEdit(f.id));
        } else {
            lay->addWidget(lineEdit(f.id));
        }
    }

    if (d.id == "funcdef") {
        auto* hint = new QLabel("Paramètres : « nom:type » séparés par des virgules, 3 maximum.\n"
                                "Ex. :  a:nombre, titre:texte\n"
                                "Dans la fonction, utilise un bloc « Variable » avec le nom du paramètre. "
                                "Relie le dernier bloc à « Retourner » si la fonction renvoie une valeur.");
        hint->setWordWrap(true);
        hint->setStyleSheet("color:#888; margin-top:8px;");
        lay->addWidget(hint);
    }
    if (d.id == "call" || d.id == "callv") {
        auto* hint = new QLabel("Les arguments sont donnés dans l'ordre des paramètres de la fonction "
                                "(argument 1 = premier paramètre…). Les arguments en trop sont ignorés.");
        hint->setWordWrap(true);
        hint->setStyleSheet("color:#888; margin-top:8px;");
        lay->addWidget(hint);
    }
    if (d.category == "Listes" || (d.id == "setvar" && n->param("type").startsWith("liste"))) {
        auto* hint = new QLabel("La liste est une variable : écris son nom dans « liste » (créée avec « Définir "
                                "variable » de type liste). Valeur initiale : éléments séparés par des virgules.");
        hint->setWordWrap(true);
        hint->setStyleSheet("color:#888; margin-top:8px;");
        lay->addWidget(hint);
    }
    if (d.id == "getvar" || d.id == "setvar" || d.id == "addvar" || d.id == "repeat") {
        auto* hint = new QLabel("Le nom de variable ne doit pas contenir d'espaces ni d'accents spéciaux "
                                "(les caractères invalides sont remplacés par « _ »).");
        hint->setWordWrap(true);
        hint->setStyleSheet("color:#888; margin-top:8px;");
        lay->addWidget(hint);
    }
    propsScroll_->setWidget(inner);
}

// ---------------------------------------------------------------------------
//  Menu contextuel + copier / coller
// ---------------------------------------------------------------------------
QPointF MainWindow::cursorScenePos() const {
    const QPoint local = view_->viewport()->mapFromGlobal(QCursor::pos());
    return view_->mapToScene(view_->viewport()->rect().contains(local) ? local : view_->viewport()->rect().center());
}

QJsonObject MainWindow::clipboardBlocs() const {
    const QJsonDocument d = QJsonDocument::fromJson(QApplication::clipboard()->text().toUtf8());
    return (d.isObject() && d.object()["app"].toString() == "Blocs") ? d.object() : QJsonObject();
}

void MainWindow::copySelection() {
    const QJsonObject o = scene_->selectionToJson();
    if (o.isEmpty()) return;
    QApplication::clipboard()->setText(QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)));
    statusBar()->showMessage(QString("%1 bloc(s) copié(s).").arg(scene_->selectedNodes().size()), 3000);
}

void MainWindow::cutSelection() {
    copySelection();
    scene_->deleteSelection();
}

void MainWindow::pasteAt(const QPointF& scenePos) {
    const QJsonObject o = clipboardBlocs();
    if (!o.isEmpty()) scene_->pasteJson(o, scenePos);
}

void MainWindow::showContextMenu(const QPoint& globalPos, const QPointF& scenePos) {
    PortItem* port = nullptr;
    NodeItem* node = nullptr;
    ConnectionItem* wire = nullptr;
    for (QGraphicsItem* it : scene_->items(scenePos)) {
        if (!port) port = qgraphicsitem_cast<PortItem*>(it);
        if (!node) node = qgraphicsitem_cast<NodeItem*>(it);
        if (!wire) wire = qgraphicsitem_cast<ConnectionItem*>(it);
    }
    if (port) node = port->node();

    QMenu menu(this);

    if (node) {
        if (!node->isSelected()) {
            scene_->clearSelection();
            node->setSelected(true);
        }
        const int count = scene_->selectedNodes().size();
        const QString suffix = count > 1 ? QString(" (%1 blocs)").arg(count) : QString();

        if (port && port->isConnected()) {
            menu.addAction("Déconnecter ce port", this, [this, port] { scene_->disconnectPort(port); });
            menu.addSeparator();
        }
        menu.addAction("Copier" + suffix, QKeySequence::Copy, this, [this] { copySelection(); });
        menu.addAction("Couper" + suffix, QKeySequence::Cut, this, [this] { cutSelection(); });
        menu.addAction("Dupliquer" + suffix, QKeySequence("Ctrl+D"), this, [this] { scene_->duplicateSelection(); });
        menu.addSeparator();
        menu.addAction("Déconnecter tous les fils" + suffix, this,
                       [this] { scene_->disconnectNodes(scene_->selectedNodes()); });
        menu.addSeparator();
        menu.addAction("Supprimer" + suffix, QKeySequence::Delete, this, [this] { scene_->deleteSelection(); });
    } else if (wire) {
        scene_->clearSelection();
        wire->setSelected(true);
        menu.addAction("Supprimer ce fil", QKeySequence::Delete, this, [this] { scene_->deleteSelection(); });
    } else {
        QMenu* add = menu.addMenu("Ajouter un bloc");
        QMap<QString, QMenu*> cats;
        for (const NodeDef& d : nodeCatalog()) {
            if (!cats.contains(d.category)) cats[d.category] = add->addMenu(d.category);
            const QString id = d.id;
            cats[d.category]->addAction(d.title, this,
                                        [this, id, scenePos] { scene_->addNode(id, scenePos - QPointF(100, 14)); });
        }
        QAction* paste = menu.addAction("Coller ici", QKeySequence::Paste, this, [this, scenePos] { pasteAt(scenePos); });
        paste->setEnabled(!clipboardBlocs().isEmpty());
        menu.addSeparator();
        menu.addAction("Tout sélectionner", QKeySequence::SelectAll, this, [this] {
            for (QGraphicsItem* it : scene_->items()) it->setSelected(true);
        });
        menu.addAction("Recentrer la vue", QKeySequence("Home"), this, [this] { view_->centerOn(0, 0); });
        menu.addSeparator();
        QAction* run = menu.addAction("▶ Exécuter le programme", QKeySequence("F5"), this, [this] { runProgram(); });
        run->setEnabled(!runner_->isRunning());
    }
    menu.exec(globalPos);
}

// ---------------------------------------------------------------------------
//  Fichiers
// ---------------------------------------------------------------------------
void MainWindow::updateTitle() {
    QString t = QString("Blocs — %1[*]").arg(file_.isEmpty() ? "nouveau projet" : QFileInfo(file_).fileName());
    if (!workspace_.isEmpty()) t += " — " + QFileInfo(workspace_).fileName();
    setWindowTitle(t);
}

void MainWindow::setCurrentFile(const QString& path) {
    file_ = path;
    updateTitle();
    setWindowModified(false);
    explorer_->setCurrentFile(path);
    if (!path.isEmpty() && workspace_.isEmpty()) setWorkspace(QFileInfo(path).absolutePath());   // dossier = celui du programme
}

bool MainWindow::maybeSave() {
    if (!isWindowModified()) return true;
    const auto r = QMessageBox::question(this, "Blocs", "Enregistrer les modifications du projet ?",
                                         QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save) return saveFile(false);
    return r == QMessageBox::Discard;
}

bool MainWindow::saveFile(bool askName) {
    QString path = file_;
    if (askName || path.isEmpty()) {
        const QString start = !file_.isEmpty() ? file_
                              : !workspace_.isEmpty() ? ProjectExplorer::uniquePath(workspace_, "Programme", ".blocs") : QString();
        path = QFileDialog::getSaveFileName(this, "Enregistrer le projet", start, "Projet Blocs (*.blocs)");
        if (path.isEmpty()) return false;
        if (!path.endsWith(".blocs", Qt::CaseInsensitive)) path += ".blocs";
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'écrire le fichier :\n" + path);
        return false;
    }
    f.write(QJsonDocument(scene_->toJson()).toJson(QJsonDocument::Indented));
    setCurrentFile(path);
    statusBar()->showMessage("Projet enregistré.", 3000);
    return true;
}

void MainWindow::openFile() {
    if (!maybeSave()) return;
    const QString path = QFileDialog::getOpenFileName(this, "Ouvrir un projet", file_.isEmpty() ? workspace_ : file_,
                                                      "Projet Blocs (*.blocs)");
    if (!path.isEmpty()) openPath(path);
}

void MainWindow::openPath(const QString& path) {
    QFile f(path);
    const QJsonDocument doc = f.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(f.readAll()) : QJsonDocument();
    if (!doc.isObject() || !scene_->fromJson(doc.object())) {
        QMessageBox::critical(this, "Erreur", "Ce fichier n'est pas un projet Blocs valide :\n" + path);
        return;
    }
    view_->resetTransform();
    view_->centerOn(0, 0);
    setCurrentFile(path);
    resetHistory();
}

void MainWindow::newProject() {
    if (!maybeSave()) return;
    scene_->clearAll();
    setCurrentFile(QString());
    resetHistory();
}

void MainWindow::exportCSharp() {
    const QString path = QFileDialog::getSaveFileName(this, "Exporter le code C#", "Program.cs", "Fichier C# (*.cs)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'écrire le fichier :\n" + path);
        return;
    }
    f.write(lastCode_.toUtf8());
    statusBar()->showMessage("Code C# exporté : " + path, 5000);
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (maybeSave()) {
        exporter_->cancel();
        runner_->stop();
        if (game_) game_->close();
        e->accept();
    }
    else
        e->ignore();
}

// ---------------------------------------------------------------------------
//  Dossier de projet + explorateur de fichiers
// ---------------------------------------------------------------------------
void MainWindow::buildExplorer() {
    explorer_ = new ProjectExplorer;
    explorerDock_ = new QDockWidget("Explorateur", this);
    explorerDock_->setFeatures(QDockWidget::DockWidgetMovable);
    explorerDock_->setWidget(explorer_);
    addDockWidget(Qt::LeftDockWidgetArea, explorerDock_);
    tabifyDockWidget(paletteDock_, explorerDock_);       // deux onglets : « Blocs » et « Explorateur »
    paletteDock_->raise();

    connect(explorer_, &ProjectExplorer::openProgramRequested, this, [this](const QString& path) {
        if (!maybeSave()) return;
        openPath(path);
    });
    connect(explorer_, &ProjectExplorer::importAssetRequested, this,
            [this](const QString& path) { AssetsDialog::importPath(this, scene_, path); });
    connect(explorer_, &ProjectExplorer::pathRenamed, this, [this](const QString& oldPath, const QString& newPath) {
        if (file_.isEmpty() || !ProjectExplorer::isUnder(file_, oldPath)) return;
        file_ = QDir::cleanPath(newPath) + QDir::cleanPath(file_).mid(QDir::cleanPath(oldPath).size());   // le programme suit son fichier
        updateTitle();
        explorer_->setCurrentFile(file_);
    });
    connect(explorer_, &ProjectExplorer::pathRemoved, this, [this](const QString& path) {
        if (file_.isEmpty() || !ProjectExplorer::isUnder(file_, path)) return;
        file_.clear();                                   // le fichier ouvert a été supprimé : le projet reste en mémoire
        updateTitle();
        setWindowModified(true);
        explorer_->setCurrentFile(QString());
        statusBar()->showMessage("Le programme ouvert a été supprimé du dossier : enregistre-le pour le conserver.", 8000);
    });
    connect(explorer_, &ProjectExplorer::openFolderRequested, this, [this] { openFolder(); });
    connect(explorer_, &ProjectExplorer::newProjectRequested, this, [this] { newProjectFolder(); });
    connect(explorer_, &ProjectExplorer::openFolderPathRequested, this, [this](const QString& dir) { setWorkspace(dir); });
    connect(explorer_, &ProjectExplorer::browseComputerRequested, this, [this] { setWorkspace(QString()); });
    connect(explorer_, &ProjectExplorer::message, this, [this](const QString& m) { statusBar()->showMessage(m, 5000); });
}

void MainWindow::setWorkspace(const QString& dir) {
    workspace_ = dir;
    explorer_->setRootPath(dir);
    explorer_->setCurrentFile(file_);
    QSettings st("Blocs", "Blocs");
    st.setValue("workspace", dir);
    if (!dir.isEmpty()) {
        QStringList recent = st.value("recentFolders").toStringList();
        recent.removeAll(dir);
        recent.prepend(dir);
        while (recent.size() > 8) recent.removeLast();
        st.setValue("recentFolders", recent);
        explorerDock_->show();
        explorerDock_->raise();
    }
    rebuildRecentMenu();
    updateTitle();
}

void MainWindow::rebuildRecentMenu() {
    if (!recentMenu_) return;
    recentMenu_->clear();
    for (const QString& d : QSettings("Blocs", "Blocs").value("recentFolders").toStringList())
        if (QDir(d).exists()) recentMenu_->addAction(QDir::toNativeSeparators(d), this, [this, d] { setWorkspace(d); });
    recentMenu_->setEnabled(!recentMenu_->isEmpty());
}

void MainWindow::openFolder() {
    const QString dir = QFileDialog::getExistingDirectory(this, "Ouvrir le dossier du projet",
                                                          workspace_.isEmpty() ? QDir::homePath() : workspace_);
    if (!dir.isEmpty()) setWorkspace(dir);
}

void MainWindow::newProjectFolder() {
    if (!maybeSave()) return;
    const QString parent = QFileDialog::getExistingDirectory(this, "Où créer le nouveau projet ?",
                                                             workspace_.isEmpty() ? QDir::homePath() : QFileInfo(workspace_).absolutePath());
    if (parent.isEmpty()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Nouveau projet", "Nom du projet (un dossier du même nom sera créé) :",
                                               QLineEdit::Normal, "MonProjet", &ok).trimmed();
    if (!ok) return;
    QString why;
    if (!ProjectExplorer::isValidName(name, &why)) { QMessageBox::warning(this, "Nom invalide", why); return; }
    const QString folder = QDir(parent).filePath(name);
    if (QFileInfo::exists(folder) && !QDir(folder).isEmpty()) {
        QMessageBox::warning(this, "Dossier existant", "Ce dossier existe déjà et n'est pas vide.");
        return;
    }
    const QString program = QDir(folder).filePath(name + ".blocs");
    if (!ProjectExplorer::createFolder(folder, &why) && !QDir(folder).exists()) { QMessageBox::warning(this, "Erreur", why); return; }
    if (!ProjectExplorer::createProgram(program, &why)) { QMessageBox::warning(this, "Erreur", why); return; }
    setWorkspace(folder);
    openPath(program);
}

// ---------------------------------------------------------------------------
//  Export d'une application Windows autonome
// ---------------------------------------------------------------------------
void MainWindow::exportApplication() {
    if (exporter_->isRunning()) return;
    const CodeResult r = generateCSharp(*scene_, CodeMode::Standalone);
    consoleDock_->show();
    if (!r.code.contains("class Program")) {
        QMessageBox::information(this, "Exporter", "Ajoute d'abord un bloc « ▶ Début » et relie-le à d'autres blocs.");
        return;
    }
    if (!r.problems.isEmpty()) {
        console_->clear();
        appendConsole("✖ Corrige d'abord ces problèmes avant d'exporter :\n", Runner::Error);
        for (const QString& p : r.problems) appendConsole("  • " + p + "\n", Runner::Error);
        return;
    }
    const bool graphical = r.code.contains("class JeuFenetre");

    const QStringList kinds = {
        ".exe léger — un petit fichier (l'autre PC doit avoir le « runtime .NET » installé)",
        ".exe autonome — tout inclus, fonctionne sur n'importe quel Windows (gros fichier ; Internet nécessaire pour le fabriquer)",
        "Projet Visual Studio — un dossier à ouvrir et à modifier"};
    bool ok = false;
    const QString choice = QInputDialog::getItem(this, "Exporter l'application",
        graphical ? "Ton jeu va devenir une vraie fenêtre Windows. Que veux-tu créer ?" : "Que veux-tu créer ?",
        kinds, 0, false, &ok);
    if (!ok) return;

    ExportOptions opt;
    opt.kind = ExportOptions::Kind(kinds.indexOf(choice));
    const QString base = file_.isEmpty() ? QString("MonJeu") : QFileInfo(file_).completeBaseName();
    const QString name = QInputDialog::getText(this, "Nom de l'application", "Nom du programme (lettres, chiffres, _) :",
                                               QLineEdit::Normal, Exporter::sanitizeName(base), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    opt.name = Exporter::sanitizeName(name);
    opt.outDir = QFileDialog::getExistingDirectory(this, "Dossier où créer l'application", workspace_);
    if (opt.outDir.isEmpty()) return;

    console_->clear();
    for (const QString& w : missingAssetWarnings()) appendConsole("⚠ " + w + "\n", Runner::Error);
    exportAct_->setEnabled(false);
    exporter_->start(r.code, scene_->assets(), graphical, opt);
}

// ---------------------------------------------------------------------------
//  Historique + exemples
// ---------------------------------------------------------------------------
static QByteArray snapshotOf(const NodeScene& s) { return QJsonDocument(s.toJson(false)).toJson(QJsonDocument::Compact); }

void MainWindow::resetHistory() {
    snapTimer_->stop();
    history_.reset(snapshotOf(*scene_));
    undoAct_->setEnabled(false);
    redoAct_->setEnabled(false);
}

void MainWindow::scheduleSnapshot() {
    if (!restoring_) snapTimer_->start();
}

void MainWindow::commitSnapshot() {
    snapTimer_->stop();
    history_.push(snapshotOf(*scene_));
    undoAct_->setEnabled(history_.canUndo());
    redoAct_->setEnabled(history_.canRedo());
}

void MainWindow::restoreState(const QByteArray& state) {
    restoring_ = true;
    scene_->fromJson(QJsonDocument::fromJson(state).object());
    restoring_ = false;
    snapTimer_->stop();
    setWindowModified(true);
    undoAct_->setEnabled(history_.canUndo());
    redoAct_->setEnabled(history_.canRedo());
}

void MainWindow::undo() {
    commitSnapshot();                      // enregistre d'abord ce qui est en attente
    if (history_.canUndo()) restoreState(history_.undo());
}

void MainWindow::redo() {
    commitSnapshot();
    if (history_.canRedo()) restoreState(history_.redo());
}

void MainWindow::loadExample(int index) {
    if (!maybeSave()) return;
    buildExample(*scene_, index);
    view_->resetTransform();
    view_->centerOn(0, 0);
    setCurrentFile(QString());
    resetHistory();
    statusBar()->showMessage(QString("Exemple chargé : %1 — appuie sur F5 pour l'exécuter.")
                                 .arg(QString::fromUtf8(exampleName(index))), 6000);
}

void MainWindow::loadDemo() {
    buildExample(*scene_, 0);
    view_->centerOn(150, 20);
    setCurrentFile(QString());
    resetHistory();
}
