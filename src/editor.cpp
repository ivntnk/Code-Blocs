#include "editor.h"

#include <QGraphicsSceneMouseEvent>
#include <QJsonArray>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVector>
#include <algorithm>
#include <QtMath>

// ===========================================================================
//  NodeScene
// ===========================================================================
NodeScene::NodeScene(QObject* parent) : QGraphicsScene(parent) {
    setSceneRect(-8000, -8000, 16000, 16000);
}

NodeItem* NodeScene::addNode(const QString& defId, QPointF pos, bool select) {
    const NodeDef* def = findNodeDef(defId);
    if (!def) return nullptr;
    auto* node = new NodeItem(*def);
    addItem(node);
    node->setPos(pos);
    if (select) {
        clearSelection();
        node->setSelected(true);
    }
    emit structureChanged();
    return node;
}

bool NodeScene::connectNodes(NodeItem* a, const QString& outId, NodeItem* b, const QString& inId) {
    return tryConnect(a->port(PortDir::Out, outId), b->port(PortDir::In, inId));
}

void NodeScene::deleteSelection() {
    QList<NodeItem*> nodes;
    QList<ConnectionItem*> conns;
    for (QGraphicsItem* it : selectedItems()) {
        if (auto* n = qgraphicsitem_cast<NodeItem*>(it)) nodes.append(n);
        else if (auto* c = qgraphicsitem_cast<ConnectionItem*>(it)) conns.append(c);
    }
    qDeleteAll(conns);   // d'abord les fils...
    qDeleteAll(nodes);   // ...puis les blocs (qui emportent leurs fils restants)
    if (!conns.isEmpty() || !nodes.isEmpty()) emit structureChanged();
}

void NodeScene::duplicateSelection() {
    QList<NodeItem*> sel;
    for (QGraphicsItem* it : selectedItems())
        if (auto* n = qgraphicsitem_cast<NodeItem*>(it)) sel.append(n);
    if (sel.isEmpty()) return;
    clearSelection();
    for (NodeItem* n : sel) {
        NodeItem* c = addNode(n->def().id, n->pos() + QPointF(30, 30), false);
        for (auto it = n->params().begin(); it != n->params().end(); ++it) c->setParam(it.key(), it.value());
        c->setSelected(true);
    }
}

void NodeScene::clearAll() {
    clear();
    assets_.clear();
    emit structureChanged();
}

QList<NodeItem*> NodeScene::nodeItems() const {
    QList<NodeItem*> res;
    for (QGraphicsItem* it : items())
        if (auto* n = qgraphicsitem_cast<NodeItem*>(it)) res.append(n);
    std::sort(res.begin(), res.end(), [](NodeItem* a, NodeItem* b) {
        return a->y() != b->y() ? a->y() < b->y() : a->x() < b->x();
    });
    return res;
}

QList<NodeItem*> NodeScene::selectedNodes() const {
    QList<NodeItem*> res;
    for (QGraphicsItem* it : selectedItems())
        if (auto* n = qgraphicsitem_cast<NodeItem*>(it)) res.append(n);
    return res;
}

void NodeScene::disconnectNodes(const QList<NodeItem*>& nodes) {
    bool any = false;
    for (NodeItem* n : nodes)
        for (PortItem* p : n->ports()) {
            const auto copy = p->connections();
            if (!copy.isEmpty()) any = true;
            qDeleteAll(copy);
        }
    if (any) emit structureChanged();
}

void NodeScene::disconnectPort(PortItem* port) {
    const auto copy = port->connections();
    qDeleteAll(copy);
    if (!copy.isEmpty()) emit structureChanged();
}

// ---------------------------------------------------------------------------
//  Sauvegarde / chargement / copier-coller (JSON)
// ---------------------------------------------------------------------------
QJsonObject NodeScene::jsonFor(const QList<NodeItem*>& nodes) const {
    QMap<NodeItem*, int> index;
    QJsonArray jn, jl;
    for (int i = 0; i < nodes.size(); ++i) {
        NodeItem* n = nodes[i];
        index[n] = i;
        QJsonObject params;
        for (auto it = n->params().begin(); it != n->params().end(); ++it) params[it.key()] = it.value();
        jn.append(QJsonObject{{"type", n->def().id}, {"x", n->x()}, {"y", n->y()}, {"params", params}});
    }
    for (NodeItem* n : nodes)
        for (PortItem* p : n->ports())
            if (p->dir() == PortDir::Out)
                for (ConnectionItem* c : p->connections())
                    if (index.contains(c->to()->node()))      // seulement les fils internes à l'ensemble
                        jl.append(QJsonObject{{"from", index[n]}, {"fp", p->id()},
                                              {"to", index[c->to()->node()]}, {"tp", c->to()->id()}});
    return QJsonObject{{"app", "Blocs"}, {"version", 2}, {"nodes", jn}, {"links", jl}};
}

QJsonObject NodeScene::toJson(bool withAssets) const {
    QJsonObject root = jsonFor(nodeItems());
    if (withAssets && !assets_.isEmpty()) {
        QJsonObject a;
        for (auto it = assets_.begin(); it != assets_.end(); ++it)
            a[it.key()] = QJsonObject{{"ext", it.value().ext}, {"data", QString::fromLatin1(it.value().data.toBase64())}};
        root["assets"] = a;
    }
    return root;
}

QJsonObject NodeScene::selectionToJson() const {
    const QList<NodeItem*> sel = selectedNodes();
    return sel.isEmpty() ? QJsonObject() : jsonFor(sel);
}

QList<NodeItem*> NodeScene::createFromJson(const QJsonObject& root, QPointF offset) {
    QList<NodeItem*> nodes;
    for (const QJsonValue& v : root["nodes"].toArray()) {
        const QJsonObject o = v.toObject();
        NodeItem* n = addNode(o["type"].toString(),
                              QPointF(o["x"].toDouble(), o["y"].toDouble()) + offset, false);
        nodes.append(n);   // peut être nullptr si le type est inconnu : les indices restent cohérents
        if (!n) continue;
        const QJsonObject params = o["params"].toObject();
        for (auto it = params.begin(); it != params.end(); ++it) n->setParam(it.key(), it.value().toString());
    }
    for (const QJsonValue& v : root["links"].toArray()) {
        const QJsonObject o = v.toObject();
        const int a = o["from"].toInt(-1), b = o["to"].toInt(-1);
        if (a < 0 || b < 0 || a >= nodes.size() || b >= nodes.size() || !nodes[a] || !nodes[b]) continue;
        connectNodes(nodes[a], o["fp"].toString(), nodes[b], o["tp"].toString());
    }
    return nodes;
}

bool NodeScene::fromJson(const QJsonObject& root) {
    if (root["app"].toString() != "Blocs") return false;
    blockSignals(true);
    clear();
    if (root.contains("assets")) {                 // sans « assets » (historique) : on garde les ressources actuelles
        assets_.clear();
        const QJsonObject a = root["assets"].toObject();
        for (auto it = a.begin(); it != a.end(); ++it)
            assets_[it.key()] = Asset{it.value().toObject()["ext"].toString(),
                                      QByteArray::fromBase64(it.value().toObject()["data"].toString().toLatin1())};
    }
    createFromJson(root, QPointF());
    clearSelection();
    blockSignals(false);
    emit structureChanged();
    return true;
}

void NodeScene::pasteJson(const QJsonObject& root, QPointF at) {
    if (root["app"].toString() != "Blocs") return;
    // coin haut-gauche de ce qu'on colle → placé sous `at`
    double minX = 1e18, minY = 1e18;
    for (const QJsonValue& v : root["nodes"].toArray()) {
        minX = std::min(minX, v.toObject()["x"].toDouble());
        minY = std::min(minY, v.toObject()["y"].toDouble());
    }
    if (minX > 1e17) return;
    clearSelection();
    const QList<NodeItem*> nodes = createFromJson(root, at - QPointF(minX, minY));
    for (NodeItem* n : nodes)
        if (n) n->setSelected(true);
}

PortItem* NodeScene::portAt(const QPointF& scenePos) const {
    for (QGraphicsItem* it : items(scenePos))
        if (auto* p = qgraphicsitem_cast<PortItem*>(it)) return p;
    return nullptr;
}

bool NodeScene::tryConnect(PortItem* a, PortItem* b) {
    if (!a || !b || a == b) return false;
    if (a->node() == b->node()) return false;               // pas de fil d'un bloc vers lui-même
    if (a->kind() != b->kind()) return false;               // ▶ avec ▶, ● avec ●
    if (a->dir() == b->dir()) return false;                 // une sortie avec une entrée

    PortItem* out = (a->dir() == PortDir::Out) ? a : b;
    PortItem* in = (a->dir() == PortDir::In) ? a : b;

    if (a->kind() == PortKind::Data) {                      // les types doivent correspondre
        const DataType t1 = out->dataType(), t2 = in->dataType();
        if (t1 != DataType::Any && t2 != DataType::Any && t1 != t2) {
            emit message(QString("Types incompatibles : %1 → %2 (utilise un bloc de conversion)")
                             .arg(typeName(t1), typeName(t2)));
            return false;
        }
    }

    for (ConnectionItem* c : out->connections())
        if (c->to() == in) return false;                    // déjà relié

    if (out->kind() == PortKind::Flow) {                    // une sortie ▶ n'a qu'un seul fil
        const auto old = out->connections();
        qDeleteAll(old);
    }
    if (in->kind() == PortKind::Data) {                     // une entrée ● n'a qu'un seul fil
        const auto old = in->connections();
        qDeleteAll(old);
    }
    addItem(new ConnectionItem(out, in));
    emit structureChanged();
    return true;
}

void NodeScene::mousePressEvent(QGraphicsSceneMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        if (PortItem* p = portAt(e->scenePos())) {
            // Entrée ● déjà reliée : on « décroche » le fil pour le replacer ailleurs
            if (p->dir() == PortDir::In && p->kind() == PortKind::Data && !p->connections().isEmpty()) {
                ConnectionItem* c = p->connections().first();
                dragFrom_ = c->from();
                delete c;
                emit structureChanged();
            } else {
                dragFrom_ = p;
            }
            tempWire_ = new QGraphicsPathItem;
            tempWire_->setPen(QPen(QColor("#4fc3f7"), 2, Qt::DashLine));
            tempWire_->setZValue(-1);
            tempWire_->setAcceptedMouseButtons(Qt::NoButton);
            addItem(tempWire_);
            mouseMoveEvent(e);
            e->accept();
            return;
        }
    }
    QGraphicsScene::mousePressEvent(e);
}

void NodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e) {
    if (dragFrom_ && tempWire_) {
        const QPointF a = dragFrom_->scenePos(), b = e->scenePos();
        tempWire_->setPath(dragFrom_->dir() == PortDir::Out ? ConnectionItem::makePath(a, b)
                                                            : ConnectionItem::makePath(b, a));
        return;
    }
    QGraphicsScene::mouseMoveEvent(e);
}

void NodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {
    if (dragFrom_) {
        delete tempWire_;
        tempWire_ = nullptr;
        tryConnect(dragFrom_, portAt(e->scenePos()));
        dragFrom_ = nullptr;
        return;
    }
    QGraphicsScene::mouseReleaseEvent(e);
}

void NodeScene::drawBackground(QPainter* p, const QRectF& rect) {
    p->fillRect(rect, QColor("#1c1d24"));

    const int step = 20;
    const int left = qFloor(rect.left() / step) * step;
    const int top = qFloor(rect.top() / step) * step;
    QVector<QLineF> minor, major;
    for (int x = left; x < rect.right(); x += step)
        (x % (step * 5) == 0 ? major : minor).append(QLineF(x, rect.top(), x, rect.bottom()));
    for (int y = top; y < rect.bottom(); y += step)
        (y % (step * 5) == 0 ? major : minor).append(QLineF(rect.left(), y, rect.right(), y));
    p->setPen(QPen(QColor("#252733"), 0));
    p->drawLines(minor);
    p->setPen(QPen(QColor("#303345"), 0));
    p->drawLines(major);
}

// ===========================================================================
//  NodeEditorView
// ===========================================================================
NodeEditorView::NodeEditorView(NodeScene* scene, QWidget* parent) : QGraphicsView(scene, parent) {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);                 // glisser sur le fond = sélection
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setFrameShape(QFrame::NoFrame);
}

void NodeEditorView::wheelEvent(QWheelEvent* e) {
    const double factor = e->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const double target = transform().m11() * factor;
    if (target < 0.25 || target > 3.0) return;
    scale(factor, factor);
}

void NodeEditorView::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::MiddleButton || e->button() == Qt::RightButton) {   // déplacer la vue
        panning_ = true;
        moved_ = false;
        lastPos_ = pressPos_ = e->pos();
        viewport()->setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    QGraphicsView::mousePressEvent(e);
}

void NodeEditorView::mouseMoveEvent(QMouseEvent* e) {
    if (panning_) {
        const QPoint d = e->pos() - lastPos_;
        lastPos_ = e->pos();
        if ((e->pos() - pressPos_).manhattanLength() > 4) moved_ = true;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
        e->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(e);
}

void NodeEditorView::mouseReleaseEvent(QMouseEvent* e) {
    if (panning_ && (e->button() == Qt::MiddleButton || e->button() == Qt::RightButton)) {
        panning_ = false;
        viewport()->unsetCursor();
        e->accept();
        // clic droit sans avoir glissé = menu contextuel
        if (e->button() == Qt::RightButton && !moved_)
            emit contextMenuWanted(e->globalPosition().toPoint(), mapToScene(e->pos()));
        return;
    }
    QGraphicsView::mouseReleaseEvent(e);
}

void NodeEditorView::mouseDoubleClickEvent(QMouseEvent* e) {
    QGraphicsView::mouseDoubleClickEvent(e);
    if (e->button() == Qt::LeftButton)
        emit contextMenuWanted(e->globalPosition().toPoint(), mapToScene(e->pos()));
}
