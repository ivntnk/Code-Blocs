// codegen.h — transforme le graphe de blocs en code C#.
#pragma once

#include <QString>
#include <QStringList>

class NodeScene;

struct CodeResult {
    QString code;           // le programme C# complet
    QStringList problems;   // erreurs / avertissements détectés (types, variables…)
};

// forRun = true : ajoute au début de Main la configuration UTF-8 de la console (accents corrects
// quand Blocs lance le programme lui-même). Le code exporté, lui, reste « propre ».
enum class CodeMode {
    Export,       // fichier .cs « propre » (un jeu graphique dialogue avec la fenêtre de Blocs)
    Run,          // exécution depuis Blocs (console UTF-8, jeu dans la fenêtre de Blocs)
    Standalone    // application Windows autonome (jeu graphique = vraie fenêtre WinForms)
};

CodeResult generateCSharp(const NodeScene& scene, CodeMode mode);
CodeResult generateCSharp(const NodeScene& scene, bool forRun = false);
