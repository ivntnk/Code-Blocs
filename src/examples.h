// examples.h — projets d'exemple intégrés (menu Fichier > Exemples) et étapes des tutoriels.
#pragma once

#include <QVector>

class NodeScene;

struct ExampleInfo {
    int id;
    const char* name;
    bool inMenu;       // false = étape intermédiaire d'un tutoriel (non listée dans le menu)
};

const QVector<ExampleInfo>& examples();
const char* exampleName(int id);
void buildExample(NodeScene& scene, int id);   // remplace le contenu de la scène
