#include "assetsdialog.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "editor.h"
#include "exporter.h"

bool AssetsDialog::importFile(QWidget* parent, NodeScene* scene, bool image) {
    const QString filter = image ? "Images (*.png *.jpg *.jpeg *.gif *.bmp)" : "Sons WAV (*.wav)";
    const QString path = QFileDialog::getOpenFileName(parent, image ? "Importer une image" : "Importer un son", QString(), filter);
    return !path.isEmpty() && importPath(parent, scene, path);
}

bool AssetsDialog::importPath(QWidget* parent, NodeScene* scene, const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "jpeg") ext = "jpg";
    const bool image = QStringList{"png", "jpg", "gif", "bmp"}.contains(ext);
    if (!image && ext != "wav") {
        QMessageBox::warning(parent, "Fichier non pris en charge", "Seuls les images (PNG, JPG, GIF, BMP) et les sons WAV peuvent devenir des ressources.");
        return false;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(parent, "Erreur", "Impossible de lire le fichier.");
        return false;
    }
    const QByteArray data = f.readAll();
    if (data.size() > 8 * 1024 * 1024) {
        QMessageBox::warning(parent, "Fichier trop gros", "Une ressource ne doit pas dépasser 8 Mo.");
        return false;
    }
    if (image) {
        if (QImage::fromData(data).isNull()) {
            QMessageBox::warning(parent, "Image illisible", "Ce fichier n'est pas une image valide.");
            return false;
        }
    } else if (data.size() < 12 || data.left(4) != "RIFF" || data.mid(8, 4) != "WAVE") {
        QMessageBox::warning(parent, "Son non pris en charge", "Seuls les fichiers WAV sont acceptés.");
        return false;
    }

    bool ok = false;
    const QString suggested = Exporter::sanitizeName(QFileInfo(path).completeBaseName()).remove(QRegularExpression("^Jeu_(?=\\d)"));
    QString name = QInputDialog::getText(parent, "Nom de la ressource",
                                         "Nom à utiliser dans les blocs (lettres, chiffres, _) :", QLineEdit::Normal, suggested, &ok);
    if (!ok) return false;
    name = Exporter::sanitizeName(name);
    if (scene->assets().contains(name) &&
        QMessageBox::question(parent, "Remplacer ?", QString("« %1 » existe déjà. Le remplacer ?").arg(name)) != QMessageBox::Yes)
        return false;
    scene->setAsset(name, Asset{ext, data});
    return true;
}

AssetsDialog::AssetsDialog(NodeScene* scene, QWidget* parent) : QDialog(parent), scene_(scene) {
    setWindowTitle("Ressources du projet (images et sons)");
    resize(560, 460);
    auto* lay = new QVBoxLayout(this);
    auto* hint = new QLabel("Utilise ces noms dans les blocs « Dessiner une image » et « Jouer un son ».\n"
                            "Les ressources sont enregistrées dans le fichier .blocs et intégrées à l'application exportée.");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#9aa0b4;");
    lay->addWidget(hint);

    list_ = new QListWidget;
    list_->setIconSize(QSize(48, 48));
    lay->addWidget(list_, 1);

    auto* row = new QHBoxLayout;
    auto* addImg = new QPushButton("Importer une image…");
    auto* addSnd = new QPushButton("Importer un son…");
    auto* del = new QPushButton("Supprimer");
    auto* close = new QPushButton("Fermer");
    row->addWidget(addImg);
    row->addWidget(addSnd);
    row->addWidget(del);
    row->addStretch(1);
    row->addWidget(close);
    lay->addLayout(row);

    connect(addImg, &QPushButton::clicked, this, [this] { importFile(this, scene_, true); });
    connect(addSnd, &QPushButton::clicked, this, [this] { importFile(this, scene_, false); });
    connect(del, &QPushButton::clicked, this, [this] {
        if (auto* it = list_->currentItem()) scene_->removeAsset(it->data(Qt::UserRole).toString());
    });
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    connect(scene_, &NodeScene::assetsChanged, this, [this] { refresh(); });
    refresh();
}

void AssetsDialog::refresh() {
    list_->clear();
    for (auto it = scene_->assets().begin(); it != scene_->assets().end(); ++it) {
        const Asset& a = it.value();
        auto* item = new QListWidgetItem(QString("%1   (%2, %3 Ko)").arg(it.key(), a.ext).arg((a.data.size() + 1023) / 1024));
        item->setData(Qt::UserRole, it.key());
        if (a.isImage()) {
            const QImage im = QImage::fromData(a.data);
            if (!im.isNull()) item->setIcon(QIcon(QPixmap::fromImage(im.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation))));
        } else {
            item->setText("🔊  " + item->text());
        }
        list_->addItem(item);
    }
}
