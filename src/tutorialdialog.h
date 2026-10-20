// tutorialdialog.h — fenêtre d'aide : tutoriels pas à pas et référence des blocs.
#pragma once

#include <QDialog>
#include <functional>

class QLabel;
class QListWidget;
class QPushButton;
class QTextBrowser;

class TutorialDialog : public QDialog {
public:
    // loadExample : appelée quand l'utilisateur clique sur « Charger l'exemple »
    TutorialDialog(std::function<void(int)> loadExample, QWidget* parent);
    void showTutorial(int index);

private:
    void showStep(int step);

    std::function<void(int)> load_;
    QListWidget* list_;
    QLabel* title_;
    QLabel* progress_;
    QTextBrowser* view_;
    QPushButton* prev_;
    QPushButton* next_;
    QPushButton* loadBtn_;
    int tut_ = 0;
    int step_ = 0;
};

// Fenêtre « Référence des blocs » (générée à partir du catalogue)
void showBlockReference(QWidget* parent);
