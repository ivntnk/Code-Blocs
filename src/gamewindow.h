// gamewindow.h — la fenêtre de jeu : affiche les dessins envoyés par le programme C#
// et lui renvoie le clavier et la souris.
#pragma once

#include <QHash>
#include <QImage>
#include <QSet>
#include <QWidget>

#include "editor.h"

class GameWindow : public QWidget {
    Q_OBJECT
public:
    explicit GameWindow(QWidget* parent = nullptr);

    void handle(const QString& payload);       // exécute les commandes d'une image
    void setAssets(const QMap<QString, Asset>& assets);   // images et sons du projet
    void setFinished(bool finished);           // le programme est terminé (ou relancé)
    const QImage& canvas() const { return img_; }

signals:
    void inputEvent(const QString& line);      // « kd Gauche », « ku Gauche », « mm 10 20 »…
    void closedByUser();

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void closeEvent(QCloseEvent* e) override;

private:
    void releaseAllKeys();
    void sendMouse(const char* kind, const QPoint& p);

    const Asset* findAsset(const QString& name, bool image) const;
    QImage spriteImage(const QString& name);
    void playSound(const QString& name);

    QMap<QString, Asset> assets_;
    QHash<QString, QImage> spriteCache_;
    QByteArray sound_;                 // reste valide pendant la lecture asynchrone
    QImage img_;
    QSet<QString> down_;
    QString title_ = "Jeu";
    bool finished_ = false;
};
