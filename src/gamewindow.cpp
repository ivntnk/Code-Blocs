#include "gamewindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {

QColor gameColor(QString s) {
    s = s.trimmed().toLower();
    static const QMap<QString, QColor> names = {
        {"noir", QColor("#101018")},   {"blanc", QColor("#f5f5f5")},  {"rouge", QColor("#e53935")},
        {"vert", QColor("#43a047")},   {"bleu", QColor("#1e88e5")},   {"jaune", QColor("#fdd835")},
        {"orange", QColor("#fb8c00")}, {"violet", QColor("#8e24aa")}, {"rose", QColor("#ec407a")},
        {"marron", QColor("#6d4c41")}, {"gris", QColor("#9e9e9e")},   {"cyan", QColor("#26c6da")}};
    if (names.contains(s)) return names[s];
    const QColor c(s);                 // « #ff8800 » ou un nom anglais (« red »)
    return c.isValid() ? c : QColor("#f5f5f5");
}

QString keyName(const QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_Left: return "Gauche";
    case Qt::Key_Right: return "Droite";
    case Qt::Key_Up: return "Haut";
    case Qt::Key_Down: return "Bas";
    case Qt::Key_Space: return "Espace";
    case Qt::Key_Return:
    case Qt::Key_Enter: return "Entree";
    case Qt::Key_Escape: return "Echap";
    default: break;
    }
    if (e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z) return QString(QChar('A' + (e->key() - Qt::Key_A)));
    if (e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9) return QString(QChar('0' + (e->key() - Qt::Key_0)));
    return QString();
}

}  // namespace

GameWindow::GameWindow(QWidget* parent) : QWidget(parent, Qt::Window) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setWindowTitle(title_);
    img_ = QImage(640, 480, QImage::Format_ARGB32_Premultiplied);
    img_.fill(QColor("#101018"));
    setFixedSize(img_.size());
}

void GameWindow::setAssets(const QMap<QString, Asset>& assets) {
    assets_ = assets;
    spriteCache_.clear();
}

const Asset* GameWindow::findAsset(const QString& name, bool image) const {
    for (auto it = assets_.begin(); it != assets_.end(); ++it)
        if (it.key().compare(name, Qt::CaseInsensitive) == 0 && it.value().isImage() == image) return &it.value();
    return nullptr;
}

QImage GameWindow::spriteImage(const QString& name) {
    const QString key = name.toLower();
    auto it = spriteCache_.find(key);
    if (it != spriteCache_.end()) return it.value();
    QImage im;
    if (const Asset* a = findAsset(name, true)) im = QImage::fromData(a->data);
    spriteCache_.insert(key, im);
    return im;
}

void GameWindow::playSound(const QString& name) {
    const Asset* a = findAsset(name, false);
    if (!a) return;
#ifdef Q_OS_WIN
    const QByteArray next = a->data;      // on garde l'ancien son jusqu'à ce que le nouveau ait démarré
    PlaySoundW(reinterpret_cast<LPCWSTR>(next.constData()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    sound_ = next;
#else
    QApplication::beep();
#endif
}

void GameWindow::setFinished(bool finished) {
    finished_ = finished;
    setWindowTitle(finished ? title_ + "  (programme terminé)" : title_);
}

void GameWindow::handle(const QString& payload) {
    const QStringList lines = payload.split('\n', Qt::SkipEmptyParts);

    // 1) taille et titre d'abord (ils changent l'image elle-même)
    bool opened = false;
    for (const QString& line : lines) {
        const QStringList p = line.split(' ');
        if (p[0] == "size" && p.size() >= 3) {
            const int w = qBound(100, p[1].toInt(), 3000), h = qBound(100, p[2].toInt(), 2000);
            img_ = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
            img_.fill(QColor("#101018"));
            setFixedSize(w, h);
            opened = true;
        } else if (p[0] == "title") {
            title_ = line.section(' ', 1);
            setWindowTitle(finished_ ? title_ + "  (programme terminé)" : title_);
        }
    }

    // 2) dessin
    {
        QPainter pt(&img_);
        pt.setRenderHint(QPainter::Antialiasing);
        pt.setRenderHint(QPainter::SmoothPixmapTransform);
        for (const QString& line : lines) {
            const QStringList p = line.split(' ');
            const QString cmd = p[0];
            auto num = [&](int i) { return i < p.size() ? p[i].toDouble() : 0.0; };
            auto col = [&](int i) { return gameColor(i < p.size() ? p[i] : QString()); };
            if (cmd == "clear" && p.size() >= 2) {
                img_.fill(gameColor(p[1]));
            } else if (cmd == "rect" && p.size() >= 7) {
                if (p[6] == "1") pt.fillRect(QRectF(num(1), num(2), num(3), num(4)), col(5));
                else { pt.setPen(QPen(col(5), 2)); pt.setBrush(Qt::NoBrush); pt.drawRect(QRectF(num(1), num(2), num(3), num(4))); }
            } else if (cmd == "circle" && p.size() >= 6) {
                if (p[5] == "1") { pt.setPen(Qt::NoPen); pt.setBrush(col(4)); }
                else { pt.setPen(QPen(col(4), 2)); pt.setBrush(Qt::NoBrush); }
                pt.drawEllipse(QPointF(num(1), num(2)), num(3), num(3));
            } else if (cmd == "line" && p.size() >= 7) {
                pt.setPen(QPen(col(5), qMax(1.0, num(6)), Qt::SolidLine, Qt::RoundCap));
                pt.drawLine(QPointF(num(1), num(2)), QPointF(num(3), num(4)));
            } else if (cmd == "text" && p.size() >= 6) {
                QFont f = pt.font();
                f.setPixelSize(qMax(6, int(num(3))));
                pt.setFont(f);
                pt.setPen(col(4));
                pt.drawText(QRectF(num(1), num(2), 4000, num(3) * 2), Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,
                            line.section(' ', 5));
            } else if (cmd == "sprite" && p.size() >= 7) {
                const QImage im = spriteImage(line.section(' ', 6));
                if (!im.isNull()) {
                    pt.save();
                    pt.translate(num(1) + num(3) / 2, num(2) + num(4) / 2);
                    pt.rotate(num(5));
                    pt.drawImage(QRectF(-num(3) / 2, -num(4) / 2, num(3), num(4)), im);
                    pt.restore();
                }
            } else if (cmd == "sound" && p.size() >= 2) {
                playSound(line.section(' ', 1));
            } else if (cmd == "beep") {
                QApplication::beep();
            }
        }
    }

    if (opened && !isVisible()) {
        show();
        raise();
        activateWindow();
        setFocus();
    }
    update();
}

void GameWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.drawImage(0, 0, img_);
}

void GameWindow::keyPressEvent(QKeyEvent* e) {
    if (e->isAutoRepeat()) return;
    const QString n = keyName(e);
    if (!n.isEmpty() && !down_.contains(n)) {
        down_.insert(n);
        emit inputEvent("kd " + n);
    }
    e->accept();
}

void GameWindow::keyReleaseEvent(QKeyEvent* e) {
    if (e->isAutoRepeat()) return;
    const QString n = keyName(e);
    if (!n.isEmpty() && down_.remove(n)) emit inputEvent("ku " + n);
    e->accept();
}

void GameWindow::releaseAllKeys() {
    for (const QString& n : std::as_const(down_)) emit inputEvent("ku " + n);
    down_.clear();
}

void GameWindow::focusOutEvent(QFocusEvent*) { releaseAllKeys(); }

void GameWindow::sendMouse(const char* kind, const QPoint& p) {
    emit inputEvent(QString("%1 %2 %3").arg(kind).arg(p.x()).arg(p.y()));
}

void GameWindow::mouseMoveEvent(QMouseEvent* e) { sendMouse("mm", e->position().toPoint()); }

void GameWindow::mousePressEvent(QMouseEvent* e) {
    setFocus();
    if (e->button() == Qt::LeftButton) sendMouse("md", e->position().toPoint());
}

void GameWindow::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) sendMouse("mu", e->position().toPoint());
}

void GameWindow::closeEvent(QCloseEvent* e) {
    releaseAllKeys();
    emit closedByUser();
    e->accept();
}
