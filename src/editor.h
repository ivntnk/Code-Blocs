// editor.h — la scène (le « plan de travail »), la vue (zoom, déplacement) et la sauvegarde.
#pragma once

#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QByteArray>
#include <QJsonObject>
#include <QMap>

#include "graph.h"

// Une ressource du projet (image ou son), stockée dans le fichier .blocs
struct Asset {
    QString ext;          // « png », « jpg », « wav »…
    QByteArray data;
    bool isImage() const { return ext != "wav"; }
};

class NodeScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit NodeScene(QObject* parent = nullptr);

    NodeItem* addNode(const QString& defId, QPointF pos, bool select = true);
    bool connectNodes(NodeItem* a, const QString& outId, NodeItem* b, const QString& inId);
    void deleteSelection();
    void duplicateSelection();
    void clearAll();
    void disconnectNodes(const QList<NodeItem*>& nodes);   // enlève tous les fils de ces blocs
    void disconnectPort(PortItem* port);

    QList<NodeItem*> nodeItems() const;          // triés de haut en bas, gauche à droite
    QList<NodeItem*> selectedNodes() const;
    void notifyParamsChanged() { emit paramsChanged(); }

    // ressources (images, sons)
    const QMap<QString, Asset>& assets() const { return assets_; }
    void setAsset(const QString& name, const Asset& a) { assets_[name] = a; emit assetsChanged(); }
    void removeAsset(const QString& name) { if (assets_.remove(name)) emit assetsChanged(); }

    QJsonObject toJson(bool withAssets = true) const;   // tout le projet (sans les ressources : historique)
    QJsonObject selectionToJson() const;               // seulement les blocs sélectionnés (copier)
    bool fromJson(const QJsonObject& root);            // remplace tout le projet
    void pasteJson(const QJsonObject& root, QPointF at);   // ajoute des blocs (coller)

signals:
    void structureChanged();        // bloc ou fil ajouté / supprimé
    void paramsChanged();           // une valeur a été modifiée
    void layoutChanged();           // un bloc a été déplacé
    void assetsChanged();
    void message(const QString& text);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* e) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e) override;

private:
    PortItem* portAt(const QPointF& scenePos) const;
    bool tryConnect(PortItem* a, PortItem* b);
    QJsonObject jsonFor(const QList<NodeItem*>& nodes) const;
    QList<NodeItem*> createFromJson(const QJsonObject& root, QPointF offset);

    QMap<QString, Asset> assets_;
    PortItem* dragFrom_ = nullptr;          // port de départ pendant qu'on tire un fil
    QGraphicsPathItem* tempWire_ = nullptr; // fil provisoire
};

class NodeEditorView : public QGraphicsView {
    Q_OBJECT
public:
    explicit NodeEditorView(NodeScene* scene, QWidget* parent = nullptr);

signals:
    // Clic droit (sans glisser) ou double-clic gauche : demande d'ouverture du menu contextuel
    void contextMenuWanted(const QPoint& globalPos, const QPointF& scenePos);

protected:
    void wheelEvent(QWheelEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    bool panning_ = false;
    bool moved_ = false;
    QPoint lastPos_, pressPos_;
};
