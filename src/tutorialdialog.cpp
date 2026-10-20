#include "tutorialdialog.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "graph.h"
#include "tutorials.h"

static const char* kCss =
    "body { color:#e8e8ee; font-size:11pt; } h2 { color:#4fc3f7; } h3 { color:#ffb74d; } "
    "code { background-color:#3a3c4d; color:#ffd180; } li { margin-bottom:4px; } a { color:#4fc3f7; }";

TutorialDialog::TutorialDialog(std::function<void(int)> loadExample, QWidget* parent)
    : QDialog(parent), load_(std::move(loadExample)) {
    setWindowTitle("Aide et tutoriels");
    resize(1020, 660);

    auto* root = new QHBoxLayout(this);
    list_ = new QListWidget;
    list_->setFixedWidth(270);
    list_->setWordWrap(true);
    for (const Tutorial& t : tutorials()) {
        auto* it = new QListWidgetItem(t.title + "\n" + t.summary);
        list_->addItem(it);
    }
    root->addWidget(list_);

    auto* right = new QVBoxLayout;
    title_ = new QLabel;
    QFont f = title_->font();
    f.setPointSize(f.pointSize() + 4);
    f.setBold(true);
    title_->setFont(f);
    progress_ = new QLabel;
    progress_->setStyleSheet("color:#9aa0b4;");
    view_ = new QTextBrowser;
    view_->setOpenExternalLinks(true);
    view_->document()->setDefaultStyleSheet(kCss);
    view_->setStyleSheet("QTextBrowser { background:#1c1d24; border:1px solid #333645; padding:8px; }");

    auto* nav = new QHBoxLayout;
    prev_ = new QPushButton("◀ Précédent");
    next_ = new QPushButton("Suivant ▶");
    loadBtn_ = new QPushButton;
    loadBtn_->setStyleSheet("QPushButton { background:#1565c0; color:white; padding:6px 12px; }");
    auto* close = new QPushButton("Fermer");
    nav->addWidget(prev_);
    nav->addWidget(next_);
    nav->addStretch(1);
    nav->addWidget(loadBtn_);
    nav->addWidget(close);

    right->addWidget(title_);
    right->addWidget(progress_);
    right->addWidget(view_, 1);
    right->addLayout(nav);
    root->addLayout(right, 1);

    connect(list_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) { tut_ = row; showStep(0); }
    });
    connect(prev_, &QPushButton::clicked, this, [this] { showStep(step_ - 1); });
    connect(next_, &QPushButton::clicked, this, [this] { showStep(step_ + 1); });
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    connect(loadBtn_, &QPushButton::clicked, this, [this] {
        const int ex = tutorials()[tut_].steps[step_].example;
        if (ex >= 0 && load_) load_(ex);
    });
    showTutorial(0);
}

void TutorialDialog::showTutorial(int index) {
    list_->setCurrentRow(qBound(0, index, int(tutorials().size()) - 1));
    show();
    raise();
    activateWindow();
}

void TutorialDialog::showStep(int step) {
    const Tutorial& t = tutorials()[tut_];
    step_ = qBound(0, step, int(t.steps.size()) - 1);
    const TutorialStep& s = t.steps[step_];
    title_->setText(s.title);
    progress_->setText(QString("%1 — étape %2 sur %3").arg(t.title).arg(step_ + 1).arg(t.steps.size()));
    view_->setHtml(s.html);
    prev_->setEnabled(step_ > 0);
    next_->setEnabled(step_ + 1 < t.steps.size());
    loadBtn_->setVisible(s.example >= 0);
    loadBtn_->setText("▶ " + s.exampleLabel);
}

// ---------------------------------------------------------------------------
//  Référence des blocs
// ---------------------------------------------------------------------------
void showBlockReference(QWidget* parent) {
    QString html = "<h1 style='color:#4fc3f7'>Référence des blocs</h1>"
                   "<p>Tous les blocs, classés par catégorie. <b>▶</b> = flux, <b>●</b> = donnée.</p>";
    QString lastCat;
    for (const NodeDef& d : nodeCatalog()) {
        if (d.category != lastCat) {
            html += QString("<h2 style='color:#ffb74d'>%1</h2>").arg(d.category);
            lastCat = d.category;
        }
        QStringList ports;
        if (d.flowIn) ports << "▶ entrée";
        for (const auto& fo : d.flowOuts) ports << (fo.second.isEmpty() ? QString("▶ suite") : "▶ " + fo.second);
        for (const DataIn& di : d.dataIns) ports << QString("● %1 (%2)").arg(di.label, typeName(di.type));
        if (d.dataOut) ports << QString("● sortie (%1)").arg(typeName(d.outType));
        QStringList fields;
        for (const FieldDef& f : d.fields) fields << f.label;
        html += QString("<p><b>%1</b> — %2<br><span style='color:#9aa0b4'>Ports : %3%4</span></p>")
                    .arg(d.title.toHtmlEscaped(), d.help.toHtmlEscaped(), ports.join(" · ").toHtmlEscaped(),
                         fields.isEmpty() ? QString() : "<br>Réglages : " + fields.join(", ").toHtmlEscaped());
    }
    auto* dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Référence des blocs");
    dlg->resize(820, 680);
    auto* lay = new QVBoxLayout(dlg);
    auto* view = new QTextBrowser;
    view->document()->setDefaultStyleSheet(kCss);
    view->setStyleSheet("QTextBrowser { background:#1c1d24; border:none; padding:8px; }");
    view->setHtml(html);
    lay->addWidget(view);
    dlg->show();
}
