// assetsdialog.h — gestion des ressources du projet (images pour les sprites, sons WAV).
#pragma once

#include <QDialog>

class NodeScene;
class QListWidget;

class AssetsDialog : public QDialog {
public:
    AssetsDialog(NodeScene* scene, QWidget* parent);

    // Choisit un fichier, le nomme et l'ajoute au projet. Renvoie true si une ressource a été ajoutée.
    static bool importFile(QWidget* parent, NodeScene* scene, bool image);
    // Importe un fichier connu (image ou WAV, d'après l'extension) : demande seulement le nom.
    static bool importPath(QWidget* parent, NodeScene* scene, const QString& path);

private:
    void refresh();

    NodeScene* scene_;
    QListWidget* list_;
};
