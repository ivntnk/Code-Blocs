// graph.h — les éléments graphiques du graphe : blocs, ports, fils.
#pragma once

#include <QColor>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

class NodeItem;
class ConnectionItem;

enum class PortKind { Flow, Data };   // Flow = ordre d'exécution (▶), Data = valeur (●)
enum class PortDir { In, Out };

// Types de données (C#) : Any = « s'adapte » (accepte / produit n'importe quoi)
enum class DataType { Number, Text, Bool, Any, NumList, TextList };
inline bool isList(DataType t) { return t == DataType::NumList || t == DataType::TextList; }
inline DataType elemType(DataType t) { return t == DataType::TextList ? DataType::Text : DataType::Number; }

QColor typeColor(DataType t);
QString typeName(DataType t);               // "nombre", "texte", "booléen", "liste de nombres"…
DataType typeFromName(const QString& s);    // l'inverse (par défaut : nombre)

// Une entrée ● d'un bloc : si rien n'est relié, la valeur `def` (modifiable) est utilisée.
struct DataIn {
    QString id, label;
    DataType type = DataType::Any;
    QString def;
};

// Un réglage affiché dans le bloc (texte libre, ou liste de choix si `options` n'est pas vide).
struct FieldDef {
    QString id, label, def;
    QStringList options;
};

// ---------------------------------------------------------------------------
//  Description d'un TYPE de bloc (le « catalogue »).
// ---------------------------------------------------------------------------
struct NodeDef {
    QString id, title, category;
    QString help;                                // courte explication (info-bulle + référence)
    QColor color;
    bool flowIn = true;
    QVector<QPair<QString, QString>> flowOuts;   // sorties ▶ (id, libellé)
    QVector<DataIn> dataIns;                     // entrées ●
    bool dataOut = false;                        // sortie ● ?
    DataType outType = DataType::Any;
    QVector<FieldDef> fields;

    const DataIn* findInput(const QString& id) const {
        for (const DataIn& d : dataIns) if (d.id == id) return &d;
        return nullptr;
    }
};

const QVector<NodeDef>& nodeCatalog();
const NodeDef* findNodeDef(const QString& id);

// ---------------------------------------------------------------------------
//  Port : petit connecteur sur le bord d'un bloc
// ---------------------------------------------------------------------------
class PortItem : public QGraphicsItem {
public:
    enum { Type = UserType + 1 };

    PortItem(NodeItem* node, PortKind kind, PortDir dir, const QString& id, QPointF pos,
             DataType dtype = DataType::Any);
    ~PortItem() override;

    int type() const override { return Type; }
    QRectF boundingRect() const override { return QRectF(-10, -10, 20, 20); }
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;

    NodeItem* node() const { return node_; }
    PortKind kind() const { return kind_; }
    PortDir dir() const { return dir_; }
    DataType dataType() const { return dtype_; }
    const QString& id() const { return id_; }
    bool isConnected() const { return !conns_.isEmpty(); }
    const QList<ConnectionItem*>& connections() const { return conns_; }

    void addConnection(ConnectionItem* c) { conns_.append(c); update(); }
    void removeConnection(ConnectionItem* c) { conns_.removeOne(c); update(); }
    void updateConnections();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override { hovered_ = true; update(); }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override { hovered_ = false; update(); }

private:
    NodeItem* node_;
    PortKind kind_;
    PortDir dir_;
    DataType dtype_;
    QString id_;
    QList<ConnectionItem*> conns_;
    bool hovered_ = false;
};

// ---------------------------------------------------------------------------
//  Fil entre deux ports (de la sortie `from` vers l'entrée `to`)
// ---------------------------------------------------------------------------
class ConnectionItem : public QGraphicsPathItem {
public:
    enum { Type = UserType + 2 };

    ConnectionItem(PortItem* from, PortItem* to);
    ~ConnectionItem() override;

    int type() const override { return Type; }
    PortItem* from() const { return from_; }
    PortItem* to() const { return to_; }

    void updatePath();
    static QPainterPath makePath(QPointF a, QPointF b);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
    PortItem* from_;
    PortItem* to_;
};

// ---------------------------------------------------------------------------
//  Bloc
// ---------------------------------------------------------------------------
class NodeItem : public QGraphicsItem {
public:
    enum { Type = UserType + 3 };

    explicit NodeItem(const NodeDef& def);

    int type() const override { return Type; }
    QRectF boundingRect() const override;
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;

    const NodeDef& def() const { return def_; }
    QList<PortItem*> ports() const;
    PortItem* port(PortDir dir, const QString& id) const;
    qreal height() const;

    // Valeurs saisies par l'utilisateur (champs + valeurs par défaut des entrées)
    QString param(const QString& id) const { return params_.value(id); }
    void setParam(const QString& id, const QString& value) { params_[id] = value; update(); }
    const QMap<QString, QString>& params() const { return params_; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    NodeDef def_;
    QMap<QString, QString> params_;
};
