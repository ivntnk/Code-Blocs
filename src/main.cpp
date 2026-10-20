// main.cpp — point d'entrée.
#include <QApplication>
#include <QIcon>
#include <QPalette>

#include "mainwindow.h"

static void applyDarkTheme(QApplication& app) {
    app.setStyle("Fusion");
    QPalette p;
    p.setColor(QPalette::Window, QColor("#25262e"));
    p.setColor(QPalette::WindowText, QColor("#e8e8ee"));
    p.setColor(QPalette::Base, QColor("#1c1d24"));
    p.setColor(QPalette::AlternateBase, QColor("#25262e"));
    p.setColor(QPalette::Text, QColor("#e8e8ee"));
    p.setColor(QPalette::Button, QColor("#32343f"));
    p.setColor(QPalette::ButtonText, QColor("#e8e8ee"));
    p.setColor(QPalette::ToolTipBase, QColor("#32343f"));
    p.setColor(QPalette::ToolTipText, QColor("#e8e8ee"));
    p.setColor(QPalette::Highlight, QColor("#4fc3f7"));
    p.setColor(QPalette::HighlightedText, Qt::black);
    p.setColor(QPalette::PlaceholderText, QColor("#777"));
    app.setPalette(p);
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Blocs");
    applyDarkTheme(app);
    app.setWindowIcon(QIcon(":/icon.png"));
    MainWindow w;
    w.show();
    if (argc > 1) w.openPath(QString::fromLocal8Bit(argv[1]));   // double-clic sur un fichier .blocs
    else w.showWelcomeIfFirstRun();                              // tutoriel au tout premier lancement
    return app.exec();
}
