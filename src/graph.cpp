#include "graph.h"

#include <QCursor>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QtMath>
#include <algorithm>

#include "editor.h"

static constexpr qreal kW = 200;     // largeur d'un bloc
static constexpr qreal kHead = 28;   // hauteur de l'en-tête
static constexpr qreal kRow = 24;    // hauteur d'une ligne
static constexpr qreal kGrid = 10;   // pas d'aimantation à la grille

static const QColor kFlowColor("#f0f0f5");
static const QColor kSelColor("#4fc3f7");

// ===========================================================================
//  Types
// ===========================================================================
QColor typeColor(DataType t) {
    switch (t) {
    case DataType::Number: return QColor("#ffb74d");   // orange
    case DataType::Text:   return QColor("#81c784");   // vert
    case DataType::Bool:   return QColor("#e57373");   // rouge
    case DataType::NumList:  return QColor("#ba68c8"); // violet
    case DataType::TextList: return QColor("#9575cd"); // violet bleuté
    default:               return QColor("#b0bec5");   // gris (tout)
    }
}

QString typeName(DataType t) {
    switch (t) {
    case DataType::Number: return "nombre";
    case DataType::Text:   return "texte";
    case DataType::Bool:   return "booléen";
    case DataType::NumList:  return "liste de nombres";
    case DataType::TextList: return "liste de textes";
    default:               return "tout";
    }
}

DataType typeFromName(const QString& s) {
    const QString l = s.trimmed().toLower();
    if (l.startsWith("liste")) return l.contains("texte") ? DataType::TextList : DataType::NumList;
    if (l == "texte" || l == "string") return DataType::Text;
    if (l == "booléen" || l == "booleen" || l == "bool") return DataType::Bool;
    return DataType::Number;
}

// ===========================================================================
//  Catalogue des blocs  (pour ajouter un bloc : une ligne ici + son code dans codegen.cpp)
// ===========================================================================
namespace {
struct B {   // petit « constructeur » pour écrire le catalogue lisiblement
    NodeDef d;
    B(const QString& id, const QString& title, const QString& cat, const char* color) {
        d.id = id; d.title = title; d.category = cat; d.color = QColor(color);
        d.flowOuts = {{"next", ""}};
    }
    B& help(const QString& h) { d.help = h; return *this; }
    B& noFlowIn() { d.flowIn = false; return *this; }
    B& outs(QVector<QPair<QString, QString>> o) { d.flowOuts = o; return *this; }
    B& value() { d.flowIn = false; d.flowOuts.clear(); return *this; }   // bloc « valeur »
    B& in(const QString& id, const QString& label, DataType t, const QString& def) {
        d.dataIns.push_back({id, label, t, def}); return *this;
    }
    B& out(DataType t) { d.dataOut = true; d.outType = t; return *this; }
    B& field(const QString& id, const QString& label, const QString& def, QStringList opts = {}) {
        d.fields.push_back({id, label, def, opts}); return *this;
    }
};
}

const QVector<NodeDef>& nodeCatalog() {
    static QVector<NodeDef> cat;
    if (!cat.isEmpty()) return cat;
    using T = DataType;
    const QStringList types = {"nombre", "texte", "booléen", "liste de nombres", "liste de textes"};
    const QStringList retTypes = {"aucun", "nombre", "texte", "booléen", "liste de nombres", "liste de textes"};
    const QStringList listTypes = {"liste de nombres", "liste de textes"};

    // ======================= Flux
    cat << B("start", "▶ Début", "Flux", "#2e7d32").noFlowIn()
               .help("Point de départ du programme. Tout ce qui est relié à sa sortie ▶ s'exécute dans l'ordre.").d;
    cat << B("if", "Si", "Flux", "#00897b")
               .outs({{"then", "Alors"}, {"else", "Sinon"}, {"next", "Ensuite"}})
               .in("cond", "condition", T::Bool, "true")
               .help("Exécute « Alors » si la condition est vraie, sinon « Sinon » (facultatif). « Ensuite » continue après.").d;
    cat << B("repeat", "Répéter", "Flux", "#00897b")
               .outs({{"body", "Répéter"}, {"next", "Ensuite"}})
               .in("count", "fois", T::Number, "5").field("var", "compteur", "i")
               .help("Répète N fois. Le compteur vaut 0, 1, 2… N-1 (lis-le avec un bloc « Variable »).").d;
    cat << B("forrange", "Compter de … à …", "Flux", "#00897b")
               .outs({{"body", "Répéter"}, {"next", "Ensuite"}})
               .in("from", "de", T::Number, "1").in("to", "à", T::Number, "10").in("step", "pas", T::Number, "1")
               .field("var", "compteur", "i")
               .help("Compte de « de » à « à » (inclus) en avançant de « pas » (négatif pour descendre).").d;
    cat << B("while", "Tant que", "Flux", "#00897b")
               .outs({{"body", "Tant que"}, {"next", "Ensuite"}})
               .in("cond", "condition", T::Bool, "true")
               .help("Répète tant que la condition est vraie. Attention aux boucles infinies : utilise ■ Stop.").d;
    cat << B("break", "Arrêter la boucle", "Flux", "#00897b").outs({})
               .help("Sort immédiatement de la boucle en cours (Répéter, Compter, Tant que, Pour chaque).").d;
    cat << B("continue", "Tour suivant", "Flux", "#00897b").outs({})
               .help("Saute le reste du tour et passe directement au suivant dans la boucle en cours.").d;
    cat << B("wait", "Attendre", "Flux", "#00897b").in("seconds", "secondes", T::Number, "1")
               .help("Met le programme en pause (0.5 = une demi-seconde).").d;
    cat << B("exit", "Quitter le programme", "Flux", "#00897b").outs({})
               .help("Arrête tout le programme immédiatement.").d;

    // ======================= Variables
    cat << B("setvar", "Définir variable", "Variables", "#8e24aa")
               .in("value", "valeur", T::Any, "0").field("name", "nom", "x").field("type", "type", "nombre", types)
               .field("scope", "portée", "locale", {"locale", "globale"})
               .help("Crée ou modifie une variable. « globale » = visible aussi dans les fonctions. Le type est celui de la variable.").d;
    cat << B("addvar", "Ajouter à variable", "Variables", "#8e24aa")
               .in("amount", "quantité", T::Any, "1").field("name", "nom", "x")
               .help("Ajoute à une variable : nombre + quantité, ou texte + texte. Mets une quantité négative pour retirer.").d;
    cat << B("getvar", "Variable", "Variables", "#8e24aa").value().out(T::Any).field("name", "nom", "x")
               .help("Donne la valeur d'une variable (ou d'un compteur de boucle, ou d'un paramètre de fonction).").d;

    // ======================= Valeurs
    cat << B("number", "Nombre", "Valeurs", "#5c6bc0").value().out(T::Number).field("value", "valeur", "0")
               .help("Un nombre fixe (3, 2.5, -10).").d;
    cat << B("text", "Texte", "Valeurs", "#5c6bc0").value().out(T::Text).field("value", "texte", "Bonjour")
               .help("Un texte fixe.").d;
    cat << B("bool", "Booléen", "Valeurs", "#5c6bc0").value().out(T::Bool)
               .field("value", "valeur", "true", {"true", "false"}).help("Vrai (true) ou faux (false).").d;
    cat << B("random", "Nombre aléatoire", "Valeurs", "#5c6bc0").value().out(T::Number)
               .in("a", "min", T::Number, "1").in("b", "max", T::Number, "10")
               .help("Entier au hasard entre min et max (inclus). Change à CHAQUE lecture : range-le dans une variable pour le garder.").d;
    cat << B("randomreal", "Décimal aléatoire (0-1)", "Valeurs", "#5c6bc0").value().out(T::Number)
               .help("Nombre décimal au hasard entre 0 et 1 (utile pour des probabilités).").d;

    // ======================= Calcul
    cat << B("math", "Opération", "Calcul", "#ef6c00").value().out(T::Number)
               .in("a", "a", T::Number, "0").in("b", "b", T::Number, "0")
               .field("op", "opérateur", "+", {"+", "-", "*", "/", "%", "//", "puissance"})
               .help("a (opérateur) b. « % » = reste de la division, « // » = division entière.").d;
    cat << B("mathfn", "Fonction mathématique", "Calcul", "#ef6c00").value().out(T::Number)
               .in("x", "x", T::Number, "0")
               .field("fn", "fonction", "abs", {"abs", "racine", "arrondi", "plancher", "plafond", "sinus", "cosinus"})
               .help("Valeur absolue, racine carrée, arrondis, sinus, cosinus de x.").d;
    cat << B("minmax", "Minimum / Maximum", "Calcul", "#ef6c00").value().out(T::Number)
               .in("a", "a", T::Number, "0").in("b", "b", T::Number, "0")
               .field("op", "choix", "min", {"min", "max"}).help("Le plus petit ou le plus grand de a et b.").d;
    cat << B("compare", "Comparaison", "Calcul", "#ef6c00").value().out(T::Bool)
               .in("a", "a", T::Any, "0").in("b", "b", T::Any, "0")
               .field("op", "opérateur", "==", {"==", "!=", "<", ">", "<=", ">="})
               .help("Compare deux valeurs du même type. < > <= >= seulement pour les nombres.").d;
    cat << B("logic", "Logique (et / ou)", "Calcul", "#ef6c00").value().out(T::Bool)
               .in("a", "a", T::Bool, "true").in("b", "b", T::Bool, "true")
               .field("op", "opérateur", "et", {"et", "ou"}).help("« et » : les deux vraies. « ou » : au moins une vraie.").d;
    cat << B("not", "Non", "Calcul", "#ef6c00").value().out(T::Bool).in("a", "valeur", T::Bool, "true")
               .help("Inverse un booléen (vrai ↔ faux).").d;

    // ======================= Texte
    cat << B("join", "Assembler texte", "Texte", "#2e7d9a").value().out(T::Text)
               .in("a", "texte A", T::Any, "Bonjour ").in("b", "texte B", T::Any, "")
               .help("Colle deux valeurs bout à bout (les nombres sont convertis en texte). Chaîne plusieurs blocs pour en coller plus.").d;
    cat << B("textlen", "Longueur du texte", "Texte", "#2e7d9a").value().out(T::Number)
               .in("text", "texte", T::Text, "Bonjour").help("Nombre de caractères.").d;
    cat << B("textcase", "Majuscules / minuscules", "Texte", "#2e7d9a").value().out(T::Text)
               .in("text", "texte", T::Text, "Bonjour")
               .field("op", "transformation", "minuscules", {"majuscules", "minuscules", "sans espaces autour"})
               .help("Change la casse, ou retire les espaces au début et à la fin.").d;
    cat << B("textsub", "Extraire du texte", "Texte", "#2e7d9a").value().out(T::Text)
               .in("text", "texte", T::Text, "Bonjour").in("start", "début", T::Number, "0").in("len", "longueur", T::Number, "3")
               .help("Extrait « longueur » caractères à partir de la position « début » (0 = premier). Sans erreur si trop long.").d;
    cat << B("textchar", "Caractère n°", "Texte", "#2e7d9a").value().out(T::Text)
               .in("text", "texte", T::Text, "Bonjour").in("index", "position", T::Number, "0")
               .help("Le caractère à la position donnée (0 = premier). Texte vide si hors du texte.").d;
    cat << B("textreplace", "Remplacer dans le texte", "Texte", "#2e7d9a").value().out(T::Text)
               .in("text", "texte", T::Text, "Bonjour").in("find", "chercher", T::Text, "o").in("repl", "remplacer par", T::Text, "0")
               .help("Remplace toutes les occurrences de « chercher » par « remplacer par ».").d;
    cat << B("textcontains", "Le texte contient ?", "Texte", "#2e7d9a").value().out(T::Bool)
               .in("text", "texte", T::Text, "Bonjour").in("part", "morceau", T::Text, "jour")
               .field("op", "test", "contient", {"contient", "commence par", "finit par"})
               .help("Vrai si le texte contient / commence par / finit par le morceau.").d;
    cat << B("textsplit", "Découper en liste", "Texte", "#2e7d9a").value().out(T::TextList)
               .in("text", "texte", T::Text, "a,b,c").in("sep", "séparateur", T::Text, ",")
               .help("Coupe le texte à chaque séparateur et donne une liste de textes (à ranger dans une variable liste).").d;
    cat << B("tonumber", "Texte → nombre", "Texte", "#2e7d9a").value().out(T::Number)
               .in("x", "texte", T::Text, "0").in("def", "si invalide", T::Number, "0")
               .help("Convertit un texte en nombre (accepte 3,5 et 3.5). Si ce n'est pas un nombre : la valeur « si invalide » (le programme ne plante pas).").d;
    cat << B("isnumber", "Est un nombre ?", "Texte", "#2e7d9a").value().out(T::Bool)
               .in("x", "texte", T::Text, "12").help("Vrai si le texte représente un nombre valide.").d;

    // ======================= Entrées / Sorties
    cat << B("print", "Afficher", "Entrées / Sorties", "#1565c0").in("value", "valeur", T::Any, "Bonjour")
               .field("nl", "retour ligne", "oui", {"oui", "non"})
               .help("Affiche une valeur dans la console (les listes s'affichent [1, 2, 3]). « non » = reste sur la même ligne.").d;
    cat << B("input", "Demander (texte)", "Entrées / Sorties", "#1565c0").value().out(T::Text)
               .in("prompt", "message", T::Text, "Ton prénom ?")
               .help("Pose une question et renvoie la réponse (toujours du texte). Range-la dans une variable : elle est redemandée à chaque lecture !").d;
    cat << B("clear", "Effacer l'écran", "Entrées / Sorties", "#1565c0").help("Efface la console.").d;
    cat << B("waitkey", "Attendre une touche", "Entrées / Sorties", "#1565c0")
               .help("Met en pause jusqu'à une touche (dans la console Blocs : jusqu'à Entrée).").d;
    cat << B("readkey", "Lire une touche", "Entrées / Sorties", "#1565c0").value().out(T::Text)
               .help("Attend une touche et donne son nom (« A », « Spacebar », « LeftArrow »…). Dans la console Blocs : le texte tapé + Entrée.").d;

    // ======================= Fonctions
    cat << B("funcdef", "▶ Fonction", "Fonctions", "#c2185b").noFlowIn()
               .field("name", "nom", "maFonction").field("params", "paramètres", "n:nombre")
               .field("ret", "retour", "aucun", retTypes)
               .help("Définit une fonction réutilisable. Paramètres : « nom:type » séparés par des virgules (6 max). Relie le corps à sa sortie ▶.").d;
    cat << B("call", "Appeler fonction", "Fonctions", "#c2185b")
               .in("a1", "argument 1", T::Any, "0").in("a2", "argument 2", T::Any, "0").in("a3", "argument 3", T::Any, "0")
               .in("a4", "argument 4", T::Any, "0").in("a5", "argument 5", T::Any, "0").in("a6", "argument 6", T::Any, "0")
               .field("name", "fonction", "maFonction")
               .help("Exécute une fonction (arguments dans l'ordre des paramètres ; les arguments en trop sont ignorés).").d;
    cat << B("callv", "Appeler (valeur)", "Fonctions", "#c2185b").value().out(T::Any)
               .in("a1", "argument 1", T::Any, "0").in("a2", "argument 2", T::Any, "0").in("a3", "argument 3", T::Any, "0")
               .in("a4", "argument 4", T::Any, "0").in("a5", "argument 5", T::Any, "0").in("a6", "argument 6", T::Any, "0")
               .field("name", "fonction", "maFonction")
               .help("Appelle une fonction qui RETOURNE une valeur et utilise cette valeur.").d;
    cat << B("return", "Retourner", "Fonctions", "#c2185b").outs({}).in("value", "valeur", T::Any, "0")
               .help("Termine la fonction en renvoyant la valeur (ou termine simplement le programme/la fonction sans valeur).").d;

    // ======================= Listes
    cat << B("newlist", "Nouvelle liste", "Listes", "#6a1b9a").value().out(T::Any)
               .field("type", "type", "liste de nombres", listTypes).field("items", "éléments", "1, 2, 3")
               .help("Crée une liste avec des éléments séparés par des virgules (à ranger dans une variable de type liste).").d;
    cat << B("listadd", "Ajouter à la liste", "Listes", "#6a1b9a")
               .in("value", "valeur", T::Any, "0").field("liste", "liste", "maListe")
               .help("Ajoute un élément à la fin de la liste (désignée par le nom de sa variable).").d;
    cat << B("listset", "Remplacer un élément", "Listes", "#6a1b9a")
               .in("index", "position", T::Number, "0").in("value", "valeur", T::Any, "0").field("liste", "liste", "maListe")
               .help("Remplace l'élément à la position donnée (0 = premier).").d;
    cat << B("listremove", "Retirer de la liste", "Listes", "#6a1b9a")
               .in("index", "position", T::Number, "0").field("liste", "liste", "maListe")
               .help("Retire l'élément à la position donnée.").d;
    cat << B("listclear", "Vider la liste", "Listes", "#6a1b9a").field("liste", "liste", "maListe")
               .help("Retire tous les éléments.").d;
    cat << B("listorder", "Trier / inverser la liste", "Listes", "#6a1b9a").field("liste", "liste", "maListe")
               .field("op", "action", "trier croissant", {"trier croissant", "trier décroissant", "inverser"})
               .help("Trie la liste ou inverse son ordre.").d;
    cat << B("listget", "Élément de la liste", "Listes", "#6a1b9a").value().out(T::Any)
               .in("index", "position", T::Number, "0").field("liste", "liste", "maListe")
               .help("Lit l'élément à la position donnée (0 = premier).").d;
    cat << B("listcount", "Taille de la liste", "Listes", "#6a1b9a").value().out(T::Number).field("liste", "liste", "maListe")
               .help("Nombre d'éléments.").d;
    cat << B("listcontains", "La liste contient ?", "Listes", "#6a1b9a").value().out(T::Bool)
               .in("value", "valeur", T::Any, "0").field("liste", "liste", "maListe").help("Vrai si la valeur est dans la liste.").d;
    cat << B("listindexof", "Position dans la liste", "Listes", "#6a1b9a").value().out(T::Number)
               .in("value", "valeur", T::Any, "0").field("liste", "liste", "maListe")
               .help("Position de la valeur dans la liste, ou -1 si absente.").d;
    cat << B("listjoin", "Liste → texte", "Listes", "#6a1b9a").value().out(T::Text)
               .in("sep", "séparateur", T::Text, ", ").field("liste", "liste", "maListe")
               .help("Colle tous les éléments en un texte avec le séparateur.").d;
    cat << B("foreach", "Pour chaque élément", "Listes", "#6a1b9a")
               .outs({{"body", "Pour chaque"}, {"next", "Ensuite"}})
               .field("liste", "liste", "maListe").field("element", "élément", "item")
               .help("Répète pour chaque élément de la liste ; lis l'élément avec un bloc « Variable ».").d;

    // ======================= Jeu graphique (fenêtre de jeu intégrée à Blocs)
    QStringList keys = {"Gauche", "Droite", "Haut", "Bas", "Espace", "Entree", "Echap"};
    for (char ch = 'A'; ch <= 'Z'; ++ch) keys << QString(QChar(ch));
    for (char ch = '0'; ch <= '9'; ++ch) keys << QString(QChar(ch));
    const QString colorHelp = " Couleurs : noir, blanc, rouge, vert, bleu, jaune, orange, violet, rose, marron, gris, cyan, ou #RRGGBB.";
    const char* G = "#e65100";
    cat << B("g_open", "Ouvrir la fenêtre de jeu", "Jeu graphique", G)
               .in("w", "largeur", T::Number, "640").in("h", "hauteur", T::Number, "480").in("title", "titre", T::Text, "Mon jeu")
               .help("Ouvre la fenêtre de jeu. Coordonnées : (0, 0) en haut à gauche, x vers la droite, y vers le BAS.").d;
    cat << B("g_loop", "Boucle de jeu (60 images/s)", "Jeu graphique", G)
               .outs({{"body", "À chaque image"}, {"next", "Ensuite"}})
               .in("bg", "fond", T::Text, "noir")
               .help("Répète ~60 fois par seconde : efface l'écran avec la couleur de fond, exécute « À chaque image », puis affiche le tout." + colorHelp).d;
    cat << B("g_rect", "Rectangle", "Jeu graphique", G)
               .in("x", "x", T::Number, "50").in("y", "y", T::Number, "50").in("w", "largeur", T::Number, "100")
               .in("h", "hauteur", T::Number, "60").in("color", "couleur", T::Text, "rouge")
               .field("fill", "plein", "oui", {"oui", "non"})
               .help("Dessine un rectangle dont le coin haut-gauche est en (x, y)." + colorHelp).d;
    cat << B("g_circle", "Cercle", "Jeu graphique", G)
               .in("x", "x (centre)", T::Number, "100").in("y", "y (centre)", T::Number, "100").in("r", "rayon", T::Number, "25")
               .in("color", "couleur", T::Text, "jaune").field("fill", "plein", "oui", {"oui", "non"})
               .help("Dessine un cercle centré en (x, y)." + colorHelp).d;
    cat << B("g_line", "Ligne", "Jeu graphique", G)
               .in("x1", "x1", T::Number, "0").in("y1", "y1", T::Number, "0").in("x2", "x2", T::Number, "100")
               .in("y2", "y2", T::Number, "100").in("color", "couleur", T::Text, "blanc").in("width", "épaisseur", T::Number, "2")
               .help("Trace une ligne entre deux points." + colorHelp).d;
    cat << B("g_text", "Texte à l'écran", "Jeu graphique", G)
               .in("x", "x", T::Number, "10").in("y", "y", T::Number, "10").in("text", "texte", T::Any, "Score")
               .in("size", "taille", T::Number, "24").in("color", "couleur", T::Text, "blanc")
               .help("Écrit un texte (ou un nombre) à l'écran, coin haut-gauche en (x, y). Les emojis marchent : ⭐ 🚀 👾." + colorHelp).d;
    cat << B("g_show", "Afficher l'image maintenant", "Jeu graphique", G)
               .help("Pour un dessin fixe sans boucle de jeu : envoie le dessin à la fenêtre. L'image suivante repart d'un fond noir.").d;
    cat << B("g_image", "Dessiner une image (sprite)", "Jeu graphique", G)
               .in("img", "image", T::Text, "fusee").in("x", "x", T::Number, "100").in("y", "y", T::Number, "100")
               .in("w", "largeur", T::Number, "48").in("h", "hauteur", T::Number, "48").in("angle", "angle (°)", T::Number, "0")
               .help("Dessine une image importée (Jeu > Ressources). (x, y) = coin haut-gauche ; l'image est tournée autour de son centre.").d;
    cat << B("g_sound", "Jouer un son", "Jeu graphique", G).in("snd", "son", T::Text, "bip")
               .help("Joue un son WAV importé (Jeu > Ressources), au moment où l'image est affichée.").d;
    cat << B("g_beep", "Bip sonore", "Jeu graphique", G).help("Émet un petit bip (au moment où l'image est affichée).").d;
    cat << B("g_key", "Touche enfoncée ?", "Jeu graphique", G).value().out(T::Bool)
               .field("key", "touche", "Gauche", keys)
               .help("Vrai tant que la touche est maintenue (pour les déplacements). Clique d'abord dans la fenêtre de jeu.").d;
    cat << B("g_keyhit", "Touche appuyée (une fois)", "Jeu graphique", G).value().out(T::Bool)
               .field("key", "touche", "Espace", keys)
               .help("Vrai UNE seule fois quand la touche vient d'être appuyée (pour tirer, sauter…).").d;
    cat << B("g_mousex", "Souris : x", "Jeu graphique", G).value().out(T::Number).help("Position horizontale de la souris dans la fenêtre.").d;
    cat << B("g_mousey", "Souris : y", "Jeu graphique", G).value().out(T::Number).help("Position verticale de la souris dans la fenêtre.").d;
    cat << B("g_mousedown", "Clic souris enfoncé ?", "Jeu graphique", G).value().out(T::Bool).help("Vrai tant que le bouton gauche est enfoncé.").d;
    cat << B("g_time", "Temps écoulé (s)", "Jeu graphique", G).value().out(T::Number)
               .help("Secondes depuis le début du programme (pour des animations : sinus, cosinus…).").d;
    cat << B("g_dt", "Durée de l'image (s)", "Jeu graphique", G).value().out(T::Number)
               .help("Temps écoulé depuis l'image précédente (~0,016). Multiplie ta vitesse par cette valeur pour un mouvement régulier.").d;
    cat << B("g_overlap", "Rectangles qui se touchent ?", "Jeu graphique", G).value().out(T::Bool)
               .in("x1", "x1", T::Number, "0").in("y1", "y1", T::Number, "0").in("w1", "largeur 1", T::Number, "10").in("h1", "hauteur 1", T::Number, "10")
               .in("x2", "x2", T::Number, "0").in("y2", "y2", T::Number, "0").in("w2", "largeur 2", T::Number, "10").in("h2", "hauteur 2", T::Number, "10")
               .help("Vrai si les deux rectangles (coin haut-gauche + taille) se chevauchent : collisions !").d;
    cat << B("g_dist", "Distance entre deux points", "Jeu graphique", G).value().out(T::Number)
               .in("x1", "x1", T::Number, "0").in("y1", "y1", T::Number, "0").in("x2", "x2", T::Number, "0").in("y2", "y2", T::Number, "0")
               .help("Distance en pixels (collision de deux cercles : distance < somme des rayons).").d;

    // ======================= Avancé
    cat << B("comment", "Commentaire", "Avancé", "#78909c").field("text", "note", "Ma note")
               .help("Une note pour toi : elle apparaît comme commentaire dans le code, sans effet sur le programme.").d;
    cat << B("csharp", "Code C#", "Avancé", "#546e7a").field("code", "instruction", "Console.WriteLine(\"salut\");")
               .help("Écris n'importe quelle instruction C# (plusieurs lignes possibles). Échappatoire pour tout ce que les blocs ne font pas.").d;
    cat << B("csexpr", "Expression C#", "Avancé", "#546e7a").value().out(T::Any)
               .field("code", "expression", "1 + 1").field("type", "type", "nombre", types)
               .help("Une expression C# libre (ex. DateTime.Now.Hour). Indique le type du résultat.").d;
    cat << B("csclass", "Code C# global", "Avancé", "#546e7a").noFlowIn().outs({})
               .field("code", "code", "static double Double(double x) { return x * 2; }")
               .help("Code placé dans la classe du programme : méthodes, classes, structures… utilisables ensuite via « Expression C# » / « Code C# ».").d;
    return cat;
}

const NodeDef* findNodeDef(const QString& id) {
    for (const NodeDef& d : nodeCatalog())
        if (d.id == id) return &d;
    return nullptr;
}

// ===========================================================================
//  PortItem
// ===========================================================================
PortItem::PortItem(NodeItem* node, PortKind kind, PortDir dir, const QString& id, QPointF pos, DataType dtype)
    : QGraphicsItem(node), node_(node), kind_(kind), dir_(dir), dtype_(dtype), id_(id) {
    setPos(pos);
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::CrossCursor));
}

PortItem::~PortItem() {
    const auto copy = conns_;          // le destructeur d'un fil modifie conns_
    for (ConnectionItem* c : copy) delete c;
}

void PortItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    p->setRenderHint(QPainter::Antialiasing);
    const QColor col = (kind_ == PortKind::Flow) ? kFlowColor : typeColor(dtype_);
    p->setPen(QPen(col, 2));
    p->setBrush((!conns_.isEmpty() || hovered_) ? col : QColor("#2d2f3b"));
    if (kind_ == PortKind::Flow) {
        const QPointF tri[3] = {{-6, -7}, {7, 0}, {-6, 7}};
        p->drawPolygon(tri, 3);
    } else {
        p->drawEllipse(QPointF(0, 0), 6, 6);
    }
}

void PortItem::updateConnections() {
    for (ConnectionItem* c : std::as_const(conns_)) c->updatePath();
}

// ===========================================================================
//  ConnectionItem
// ===========================================================================
ConnectionItem::ConnectionItem(PortItem* from, PortItem* to) : from_(from), to_(to) {
    setFlag(ItemIsSelectable);
    setZValue(-1);
    from_->addConnection(this);
    to_->addConnection(this);
    updatePath();
}

ConnectionItem::~ConnectionItem() {
    if (from_) from_->removeConnection(this);
    if (to_) to_->removeConnection(this);
}

QPainterPath ConnectionItem::makePath(QPointF a, QPointF b) {
    const qreal dx = std::max<qreal>(50, qAbs(b.x() - a.x()) * 0.5);
    QPainterPath path(a);
    path.cubicTo(a + QPointF(dx, 0), b - QPointF(dx, 0), b);
    return path;
}

void ConnectionItem::updatePath() { setPath(makePath(from_->scenePos(), to_->scenePos())); }

QRectF ConnectionItem::boundingRect() const { return path().controlPointRect().adjusted(-8, -8, 8, 8); }

QPainterPath ConnectionItem::shape() const {
    QPainterPathStroker s;
    s.setWidth(12);
    return s.createStroke(path());
}

void ConnectionItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    p->setRenderHint(QPainter::Antialiasing);
    const QColor base = (from_->kind() == PortKind::Flow) ? kFlowColor : typeColor(from_->dataType());
    QPen pen(isSelected() ? kSelColor : base, isSelected() ? 3.5 : 2.5);
    pen.setCapStyle(Qt::RoundCap);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawPath(path());
}

// ===========================================================================
//  NodeItem
// ===========================================================================
NodeItem::NodeItem(const NodeDef& def) : def_(def) {
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);

    for (const DataIn& d : def_.dataIns) params_[d.id] = d.def;
    for (const FieldDef& f : def_.fields) params_[f.id] = f.def;

    if (def_.flowIn) new PortItem(this, PortKind::Flow, PortDir::In, "in", {0, kHead / 2});
    if (def_.dataOut)
        new PortItem(this, PortKind::Data, PortDir::Out, "out", {kW, kHead / 2}, def_.outType);
    for (int i = 0; i < def_.flowOuts.size(); ++i)
        new PortItem(this, PortKind::Flow, PortDir::Out, def_.flowOuts[i].first,
                     {kW, kHead + i * kRow + kRow / 2});
    for (int i = 0; i < def_.dataIns.size(); ++i)
        new PortItem(this, PortKind::Data, PortDir::In, def_.dataIns[i].id,
                     {0, kHead + i * kRow + kRow / 2}, def_.dataIns[i].type);
}

qreal NodeItem::height() const {
    const int rows = std::max<int>(def_.flowOuts.size(), def_.dataIns.size());
    return kHead + rows * kRow + def_.fields.size() * kRow + 10;
}

QRectF NodeItem::boundingRect() const { return QRectF(-2, -2, kW + 10, height() + 10); }

QList<PortItem*> NodeItem::ports() const {
    QList<PortItem*> res;
    for (QGraphicsItem* c : childItems())
        if (auto* p = qgraphicsitem_cast<PortItem*>(c)) res.append(p);
    return res;
}

PortItem* NodeItem::port(PortDir dir, const QString& id) const {
    for (PortItem* p : ports())
        if (p->dir() == dir && p->id() == id) return p;
    return nullptr;
}

void NodeItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    p->setRenderHint(QPainter::Antialiasing);
    const qreal h = height();

    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0, 0, 0, 70));
    p->drawRoundedRect(QRectF(3, 4, kW, h), 8, 8);

    QPainterPath body;
    body.addRoundedRect(QRectF(0, 0, kW, h), 8, 8);
    p->setPen(QPen(isSelected() ? kSelColor : QColor("#454859"), isSelected() ? 2 : 1));
    p->setBrush(QColor("#2d2f3b"));
    p->drawPath(body);

    p->save();
    p->setClipPath(body);
    p->fillRect(QRectF(0, 0, kW, kHead), def_.color);
    p->restore();

    QFont f = p->font();
    f.setBold(true);
    f.setPointSizeF(10);
    p->setFont(f);
    p->setPen(Qt::white);
    p->drawText(QRectF(0, 0, kW, kHead), Qt::AlignCenter, def_.title);

    f.setBold(false);
    f.setPointSizeF(9);
    p->setFont(f);
    const QFontMetrics fm(f);

    // sorties ▶ (libellés à droite)
    p->setPen(QColor("#b4b9cc"));
    for (int i = 0; i < def_.flowOuts.size(); ++i)
        p->drawText(QRectF(0, kHead + i * kRow, kW - 16, kRow), Qt::AlignVCenter | Qt::AlignRight,
                    def_.flowOuts[i].second);

    // entrées ● (libellé, et la valeur par défaut tant que rien n'est relié)
    for (int i = 0; i < def_.dataIns.size(); ++i) {
        const DataIn& d = def_.dataIns[i];
        const PortItem* pt = port(PortDir::In, d.id);
        const bool linked = pt && pt->isConnected();
        QString txt = linked ? d.label : QString("%1: %2").arg(d.label, params_.value(d.id).simplified());
        // on laisse de la place à droite si une sortie ▶ occupe la même ligne
        const qreal maxW = (i < def_.flowOuts.size() && !def_.flowOuts[i].second.isEmpty()) ? kW - 100 : kW - 32;
        p->setPen(linked ? QColor("#e8e8ee") : QColor("#9aa0b4"));
        p->drawText(QRectF(16, kHead + i * kRow, maxW, kRow), Qt::AlignVCenter | Qt::AlignLeft,
                    fm.elidedText(txt, Qt::ElideRight, int(maxW)));
    }

    // champs (réglages) sous les ports
    const int rows = std::max<int>(def_.flowOuts.size(), def_.dataIns.size());
    for (int j = 0; j < def_.fields.size(); ++j) {
        const qreal top = kHead + rows * kRow + j * kRow;
        p->setPen(Qt::NoPen);
        p->setBrush(QColor("#3a3c4d"));
        p->drawRoundedRect(QRectF(8, top + 2, kW - 16, kRow - 4), 4, 4);
        const QString txt = QString("%1: %2").arg(def_.fields[j].label, params_.value(def_.fields[j].id).simplified());
        p->setPen(QColor("#e8e8ee"));
        p->drawText(QRectF(14, top, kW - 28, kRow), Qt::AlignVCenter | Qt::AlignLeft,
                    fm.elidedText(txt, Qt::ElideRight, int(kW - 28)));
    }
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene()) {            // aimantation à la grille
        const QPointF pt = value.toPointF();
        return QPointF(qRound(pt.x() / kGrid) * kGrid, qRound(pt.y() / kGrid) * kGrid);
    }
    if (change == ItemPositionHasChanged) {                   // les fils suivent le bloc
        for (PortItem* p : ports()) p->updateConnections();
        if (auto* s = qobject_cast<NodeScene*>(scene())) emit s->layoutChanged();
    }
    return QGraphicsItem::itemChange(change, value);
}
