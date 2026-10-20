// tutorials.h — contenu des tutoriels (texte HTML) affichés dans la fenêtre d'aide.
#pragma once

#include <QString>
#include <QVector>

struct TutorialStep {
    QString title;
    QString html;
    int example = -1;          // projet à charger pour cette étape (-1 = aucun)
    QString exampleLabel;
};

struct Tutorial {
    QString title;
    QString summary;
    QVector<TutorialStep> steps;
};

const QVector<Tutorial>& tutorials();
