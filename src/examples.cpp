#include "examples.h"

#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <QtMath>
#include <functional>

#include "editor.h"

namespace {

using P = QMap<QString, QString>;

NodeItem* N(NodeScene& s, const char* type, double x, double y, const P& params = {}) {
    NodeItem* n = s.addNode(type, {x, y}, false);
    for (auto it = params.begin(); it != params.end(); ++it) n->setParam(it.key(), it.value());
    return n;
}

void hello(NodeScene& s) {
    auto* start = N(s, "start", -400, -120);
    auto* setv = N(s, "setvar", -130, -120, {{"name", "prenom"}, {"type", "texte"}});
    auto* ask = N(s, "input", -130, 40, {{"prompt", "Ton prénom ? "}});
    auto* print1 = N(s, "print", 150, -120);
    auto* join1 = N(s, "join", 150, 20, {{"a", "Bonjour "}});
    auto* var1 = N(s, "getvar", 150, 130, {{"name", "prenom"}});
    auto* rep = N(s, "repeat", 430, -120, {{"count", "3"}});
    auto* print2 = N(s, "print", 700, -120);
    auto* join2 = N(s, "join", 700, 20, {{"a", "Compteur : "}});
    auto* var2 = N(s, "getvar", 700, 130, {{"name", "i"}});
    s.connectNodes(start, "next", setv, "in");
    s.connectNodes(setv, "next", print1, "in");
    s.connectNodes(print1, "next", rep, "in");
    s.connectNodes(rep, "body", print2, "in");
    s.connectNodes(ask, "out", setv, "value");
    s.connectNodes(join1, "out", print1, "value");
    s.connectNodes(var1, "out", join1, "b");
    s.connectNodes(join2, "out", print2, "value");
    s.connectNodes(var2, "out", join2, "b");
}

// factorielle(n) = 1 si n <= 1, sinon n * factorielle(n - 1)
void factorial(NodeScene& s) {
    auto* fd = N(s, "funcdef", -520, -400, {{"name", "factorielle"}, {"params", "n:nombre"}, {"ret", "nombre"}});
    auto* iff = N(s, "if", -240, -400);
    auto* cmp = N(s, "compare", -240, -260, {{"op", "<="}, {"b", "1"}});
    auto* gv1 = N(s, "getvar", -500, -250, {{"name", "n"}});
    auto* r1 = N(s, "return", 60, -400, {{"value", "1"}});
    auto* r2 = N(s, "return", 60, -300);
    auto* mul = N(s, "math", 60, -200, {{"op", "*"}});
    auto* gv2 = N(s, "getvar", -240, -120, {{"name", "n"}});
    auto* call = N(s, "callv", 60, -70, {{"name", "factorielle"}});
    auto* sub = N(s, "math", -240, 20, {{"op", "-"}, {"b", "1"}});
    auto* gv3 = N(s, "getvar", -500, 20, {{"name", "n"}});
    s.connectNodes(fd, "next", iff, "in");
    s.connectNodes(cmp, "out", iff, "cond");
    s.connectNodes(gv1, "out", cmp, "a");
    s.connectNodes(iff, "then", r1, "in");
    s.connectNodes(iff, "else", r2, "in");
    s.connectNodes(mul, "out", r2, "value");
    s.connectNodes(gv2, "out", mul, "a");
    s.connectNodes(call, "out", mul, "b");
    s.connectNodes(sub, "out", call, "a1");
    s.connectNodes(gv3, "out", sub, "a");

    auto* start = N(s, "start", -520, 200);
    auto* pr = N(s, "print", -240, 200);
    auto* join = N(s, "join", -240, 300, {{"a", "5! = "}});
    auto* call2 = N(s, "callv", -240, 420, {{"name", "factorielle"}, {"a1", "5"}});
    s.connectNodes(start, "next", pr, "in");
    s.connectNodes(join, "out", pr, "value");
    s.connectNodes(call2, "out", join, "b");
}

// notes = [12, 8, 15, 10] ; total = somme ; affichage ; ajout d'un élément
void lists(NodeScene& s) {
    auto* start = N(s, "start", -500, -200);
    auto* sv1 = N(s, "setvar", -240, -200, {{"name", "notes"}, {"type", "liste de nombres"}, {"value", "12, 8, 15, 10"}});
    auto* sv2 = N(s, "setvar", 60, -200, {{"name", "total"}, {"type", "nombre"}, {"value", "0"}});
    auto* fe = N(s, "foreach", 360, -200, {{"liste", "notes"}, {"element", "note"}});
    auto* add = N(s, "addvar", 660, -200, {{"name", "total"}});
    auto* gvn = N(s, "getvar", 660, -80, {{"name", "note"}});
    s.connectNodes(start, "next", sv1, "in");
    s.connectNodes(sv1, "next", sv2, "in");
    s.connectNodes(sv2, "next", fe, "in");
    s.connectNodes(fe, "body", add, "in");
    s.connectNodes(gvn, "out", add, "amount");

    auto* p1 = N(s, "print", -500, 100);
    auto* j1 = N(s, "join", -500, 200, {{"a", "Total = "}});
    auto* gvt = N(s, "getvar", -500, 310, {{"name", "total"}});
    auto* p2 = N(s, "print", -240, 100);
    auto* j2 = N(s, "join", -240, 200, {{"a", "Nombre de notes : "}});
    auto* cnt = N(s, "listcount", -240, 310, {{"liste", "notes"}});
    auto* ladd = N(s, "listadd", 60, 100, {{"liste", "notes"}, {"value", "20"}});
    auto* p3 = N(s, "print", 360, 100);
    auto* gvl = N(s, "getvar", 360, 200, {{"name", "notes"}});
    s.connectNodes(fe, "next", p1, "in");
    s.connectNodes(p1, "next", p2, "in");
    s.connectNodes(p2, "next", ladd, "in");
    s.connectNodes(ladd, "next", p3, "in");
    s.connectNodes(j1, "out", p1, "value");
    s.connectNodes(gvt, "out", j1, "b");
    s.connectNodes(j2, "out", p2, "value");
    s.connectNodes(cnt, "out", j2, "b");
    s.connectNodes(gvl, "out", p3, "value");
}

// ---------------------------------------------------------------------------
//  Jeu « Devine le nombre »
// ---------------------------------------------------------------------------
struct Cmp { NodeItem* if1; NodeItem* win; };

// « Trop petit / Trop grand / Gagné » : deux Si imbriqués. loopWin : à la fin, trouve = true (sinon on affiche Bravo)
Cmp compareChain(NodeScene& s, double bx, double by, bool loopWin) {
    auto* if1 = N(s, "if", bx, by);
    auto* c1 = N(s, "compare", bx, by + 140, {{"op", "<"}});
    auto* g1 = N(s, "getvar", bx - 280, by + 140, {{"name", "reponse"}});
    auto* g2 = N(s, "getvar", bx - 280, by + 220, {{"name", "secret"}});
    auto* pS = N(s, "print", bx + 280, by - 40, {{"value", "Trop petit !"}});
    auto* if2 = N(s, "if", bx + 280, by + 80);
    auto* c2 = N(s, "compare", bx + 280, by + 260, {{"op", ">"}});
    auto* g3 = N(s, "getvar", bx, by + 300, {{"name", "reponse"}});
    auto* g4 = N(s, "getvar", bx, by + 380, {{"name", "secret"}});
    auto* pB = N(s, "print", bx + 560, by + 40, {{"value", "Trop grand !"}});
    NodeItem* win = loopWin ? N(s, "setvar", bx + 560, by + 160, {{"name", "trouve"}, {"type", "booléen"}, {"value", "true"}})
                            : N(s, "print", bx + 560, by + 160, {{"value", "Bravo, c'est le bon nombre !"}});
    s.connectNodes(c1, "out", if1, "cond"); s.connectNodes(g1, "out", c1, "a"); s.connectNodes(g2, "out", c1, "b");
    s.connectNodes(if1, "then", pS, "in");  s.connectNodes(if1, "else", if2, "in");
    s.connectNodes(c2, "out", if2, "cond"); s.connectNodes(g3, "out", c2, "a"); s.connectNodes(g4, "out", c2, "b");
    s.connectNodes(if2, "then", pB, "in");  s.connectNodes(if2, "else", win, "in");
    return {if1, win};
}

// Étape 1 : le programme choisit un nombre et lit la réponse
void guessA(NodeScene& s) {
    auto* st = N(s, "start", -600, -100);
    auto* p0 = N(s, "print", -320, -100, {{"value", "J'ai choisi un nombre entre 1 et 100."}});
    auto* svS = N(s, "setvar", -40, -100, {{"name", "secret"}, {"type", "nombre"}});
    auto* rnd = N(s, "random", -40, 60, {{"a", "1"}, {"b", "100"}});
    auto* svR = N(s, "setvar", 240, -100, {{"name", "reponse"}, {"type", "nombre"}});
    auto* tn = N(s, "tonumber", 240, 60, {{"def", "0"}});
    auto* inp = N(s, "input", 240, 170, {{"prompt", "Ton choix ? "}});
    auto* pY = N(s, "print", 520, -100);
    auto* j1 = N(s, "join", 520, 10, {{"a", "Tu as proposé "}});
    auto* gR = N(s, "getvar", 520, 120, {{"name", "reponse"}});
    auto* pS = N(s, "print", 800, -100);
    auto* j2 = N(s, "join", 800, 10, {{"a", "Le nombre secret était "}});
    auto* gS = N(s, "getvar", 800, 120, {{"name", "secret"}});
    s.connectNodes(st, "next", p0, "in"); s.connectNodes(p0, "next", svS, "in"); s.connectNodes(svS, "next", svR, "in");
    s.connectNodes(svR, "next", pY, "in"); s.connectNodes(pY, "next", pS, "in");
    s.connectNodes(rnd, "out", svS, "value"); s.connectNodes(inp, "out", tn, "x"); s.connectNodes(tn, "out", svR, "value");
    s.connectNodes(j1, "out", pY, "value"); s.connectNodes(gR, "out", j1, "b");
    s.connectNodes(j2, "out", pS, "value"); s.connectNodes(gS, "out", j2, "b");
}

// Étape 2 : on compare la réponse au nombre secret (une seule tentative)
void guessB(NodeScene& s) {
    auto* st = N(s, "start", -640, -300);
    auto* p0 = N(s, "print", -360, -300, {{"value", "J'ai choisi un nombre entre 1 et 100."}});
    auto* svS = N(s, "setvar", -80, -300, {{"name", "secret"}, {"type", "nombre"}});
    auto* rnd = N(s, "random", -80, -160, {{"a", "1"}, {"b", "100"}});
    auto* svR = N(s, "setvar", 200, -300, {{"name", "reponse"}, {"type", "nombre"}});
    auto* tn = N(s, "tonumber", 200, -150, {{"def", "0"}});
    auto* inp = N(s, "input", 200, -50, {{"prompt", "Ton choix ? "}});
    s.connectNodes(st, "next", p0, "in"); s.connectNodes(p0, "next", svS, "in"); s.connectNodes(svS, "next", svR, "in");
    s.connectNodes(rnd, "out", svS, "value"); s.connectNodes(inp, "out", tn, "x"); s.connectNodes(tn, "out", svR, "value");
    const Cmp k = compareChain(s, 760, -300, false);
    s.connectNodes(svR, "next", k.if1, "in");
}

// Programme complet : on rejoue jusqu'à trouver, en comptant les essais
void guessFinal(NodeScene& s) {
    auto* st = N(s, "start", -640, -420);
    auto* svS = N(s, "setvar", -360, -420, {{"name", "secret"}, {"type", "nombre"}});
    auto* rnd = N(s, "random", -360, -270, {{"a", "1"}, {"b", "100"}});
    auto* svE = N(s, "setvar", -80, -420, {{"name", "essais"}, {"type", "nombre"}, {"value", "0"}});
    auto* svT = N(s, "setvar", 200, -420, {{"name", "trouve"}, {"type", "booléen"}, {"value", "false"}});
    auto* p0 = N(s, "print", 480, -420, {{"value", "J'ai choisi un nombre entre 1 et 100."}});
    auto* wh = N(s, "while", 760, -420);
    auto* nt = N(s, "not", 760, -300);
    auto* gT = N(s, "getvar", 760, -210, {{"name", "trouve"}});
    auto* pEnd = N(s, "print", 1040, -420);
    auto* j2 = N(s, "join", 1040, -300, {{"b", " essai(s) !"}});
    auto* j1 = N(s, "join", 1040, -190, {{"a", "Bravo ! Trouvé en "}});
    auto* gE = N(s, "getvar", 1040, -90, {{"name", "essais"}});
    s.connectNodes(st, "next", svS, "in"); s.connectNodes(svS, "next", svE, "in"); s.connectNodes(svE, "next", svT, "in");
    s.connectNodes(svT, "next", p0, "in"); s.connectNodes(p0, "next", wh, "in"); s.connectNodes(wh, "next", pEnd, "in");
    s.connectNodes(rnd, "out", svS, "value"); s.connectNodes(gT, "out", nt, "a"); s.connectNodes(nt, "out", wh, "cond");
    s.connectNodes(j1, "out", j2, "a"); s.connectNodes(gE, "out", j1, "b"); s.connectNodes(j2, "out", pEnd, "value");

    // corps de la boucle
    auto* svR = N(s, "setvar", 200, 60, {{"name", "reponse"}, {"type", "nombre"}});
    auto* tn = N(s, "tonumber", 200, 210, {{"def", "0"}});
    auto* inp = N(s, "input", 200, 310, {{"prompt", "Ton choix ? "}});
    auto* add = N(s, "addvar", 480, 60, {{"name", "essais"}});
    s.connectNodes(wh, "body", svR, "in"); s.connectNodes(svR, "next", add, "in");
    s.connectNodes(inp, "out", tn, "x"); s.connectNodes(tn, "out", svR, "value");
    const Cmp k = compareChain(s, 760, 60, true);
    s.connectNodes(add, "next", k.if1, "in");
}

// ---------------------------------------------------------------------------
//  Jeu « Quiz » : questions et réponses dans deux listes
// ---------------------------------------------------------------------------
void quiz(NodeScene& s) {
    auto* st = N(s, "start", -640, -420);
    auto* svQ = N(s, "setvar", -360, -420, {{"name", "questions"}, {"type", "liste de textes"},
        {"value", "Capitale de la France ?, Combien font 3 x 4 ?, Couleur du ciel par beau temps ?"}});
    auto* svA = N(s, "setvar", -80, -420, {{"name", "reponses"}, {"type", "liste de textes"}, {"value", "paris, 12, bleu"}});
    auto* svS = N(s, "setvar", 200, -420, {{"name", "score"}, {"type", "nombre"}, {"value", "0"}});
    auto* fr = N(s, "forrange", 480, -420, {{"from", "0"}, {"step", "1"}, {"var", "i"}});
    auto* mn = N(s, "math", 480, -260, {{"op", "-"}, {"b", "1"}});
    auto* cq = N(s, "listcount", 480, -130, {{"liste", "questions"}});
    auto* pEnd = N(s, "print", 760, -420);
    auto* j2 = N(s, "join", 760, -300, {{"b", " bonne(s) réponse(s)"}});
    auto* j1 = N(s, "join", 760, -190, {{"a", "Score final : "}});
    auto* gSc = N(s, "getvar", 760, -90, {{"name", "score"}});
    s.connectNodes(st, "next", svQ, "in"); s.connectNodes(svQ, "next", svA, "in"); s.connectNodes(svA, "next", svS, "in");
    s.connectNodes(svS, "next", fr, "in"); s.connectNodes(fr, "next", pEnd, "in");
    s.connectNodes(cq, "out", mn, "a"); s.connectNodes(mn, "out", fr, "to");
    s.connectNodes(j1, "out", j2, "a"); s.connectNodes(gSc, "out", j1, "b"); s.connectNodes(j2, "out", pEnd, "value");

    // corps : on pose la question i, on compare à la réponse i
    auto* svR = N(s, "setvar", 200, 60, {{"name", "reponse"}, {"type", "texte"}});
    auto* lc = N(s, "textcase", 200, 210, {{"op", "minuscules"}});
    auto* inp = N(s, "input", 200, 310);
    auto* lq = N(s, "listget", 200, 390, {{"liste", "questions"}});
    auto* gi1 = N(s, "getvar", 200, 500, {{"name", "i"}});
    auto* iff = N(s, "if", 480, 60);
    auto* cmp = N(s, "compare", 480, 200, {{"op", "=="}});
    auto* gR = N(s, "getvar", 480, 330, {{"name", "reponse"}});
    auto* la = N(s, "listget", 480, 410, {{"liste", "reponses"}});
    auto* gi2 = N(s, "getvar", 480, 510, {{"name", "i"}});
    auto* pOk = N(s, "print", 760, 20, {{"value", "Correct !"}});
    auto* addS = N(s, "addvar", 1040, 20, {{"name", "score"}});
    auto* pKo = N(s, "print", 760, 160, {{"value", "Raté…"}});
    s.connectNodes(fr, "body", svR, "in"); s.connectNodes(svR, "next", iff, "in");
    s.connectNodes(lc, "out", svR, "value"); s.connectNodes(inp, "out", lc, "text"); s.connectNodes(lq, "out", inp, "prompt");
    s.connectNodes(gi1, "out", lq, "index");
    s.connectNodes(cmp, "out", iff, "cond"); s.connectNodes(gR, "out", cmp, "a"); s.connectNodes(la, "out", cmp, "b");
    s.connectNodes(gi2, "out", la, "index");
    s.connectNodes(iff, "then", pOk, "in"); s.connectNodes(pOk, "next", addS, "in"); s.connectNodes(iff, "else", pKo, "in");
}

// ---------------------------------------------------------------------------
//  Jeux graphiques
// ---------------------------------------------------------------------------
// Animation : une planète tourne autour du soleil  (x = 320 + 200·cos(t), y = 240 + 200·sin(t))
void orbit(NodeScene& s) {
    auto* st = N(s, "start", -520, -100);
    auto* op = N(s, "g_open", -240, -100, {{"w", "640"}, {"h", "480"}, {"title", "Orbite"}});
    auto* lp = N(s, "g_loop", 40, -100, {{"bg", "noir"}});
    auto* sun = N(s, "g_circle", 340, -100, {{"x", "320"}, {"y", "240"}, {"r", "30"}, {"color", "jaune"}});
    auto* pl = N(s, "g_circle", 620, -100, {{"r", "12"}, {"color", "cyan"}});
    auto* tx = N(s, "g_text", 900, -100, {{"x", "10"}, {"y", "10"}, {"text", "Orbite"}});
    s.connectNodes(st, "next", op, "in"); s.connectNodes(op, "next", lp, "in");
    s.connectNodes(lp, "body", sun, "in"); s.connectNodes(sun, "next", pl, "in"); s.connectNodes(pl, "next", tx, "in");

    auto* t1 = N(s, "g_time", 340, 150);
    auto* cs = N(s, "mathfn", 620, 150, {{"fn", "cosinus"}});
    auto* m1 = N(s, "math", 900, 150, {{"op", "*"}, {"b", "200"}});
    auto* a1 = N(s, "math", 1180, 150, {{"op", "+"}, {"a", "320"}});
    s.connectNodes(t1, "out", cs, "x"); s.connectNodes(cs, "out", m1, "a"); s.connectNodes(m1, "out", a1, "b");
    s.connectNodes(a1, "out", pl, "x");
    auto* t2 = N(s, "g_time", 340, 320);
    auto* sn = N(s, "mathfn", 620, 320, {{"fn", "sinus"}});
    auto* m2 = N(s, "math", 900, 320, {{"op", "*"}, {"b", "200"}});
    auto* a2 = N(s, "math", 1180, 320, {{"op", "+"}, {"a", "240"}});
    s.connectNodes(t2, "out", sn, "x"); s.connectNodes(sn, "out", m2, "a"); s.connectNodes(m2, "out", a2, "b");
    s.connectNodes(a2, "out", pl, "y");
}

// Jeu : on déplace la raquette bleue avec ← → pour attraper l'étoile qui tombe
void catchStars(NodeScene& s) {
    auto* st = N(s, "start", -700, -420);
    auto* op = N(s, "g_open", -420, -420, {{"w", "640"}, {"h", "480"}, {"title", "Attrape les étoiles"}});
    auto* svP = N(s, "setvar", -140, -420, {{"name", "px"}, {"type", "nombre"}, {"value", "300"}});
    auto* svX = N(s, "setvar", 140, -420, {{"name", "coinx"}, {"type", "nombre"}, {"value", "200"}});
    auto* svY = N(s, "setvar", 420, -420, {{"name", "coiny"}, {"type", "nombre"}, {"value", "0"}});
    auto* svS = N(s, "setvar", 700, -420, {{"name", "score"}, {"type", "nombre"}, {"value", "0"}});
    auto* lp = N(s, "g_loop", 980, -420, {{"bg", "noir"}});
    s.connectNodes(st, "next", op, "in"); s.connectNodes(op, "next", svP, "in"); s.connectNodes(svP, "next", svX, "in");
    s.connectNodes(svX, "next", svY, "in"); s.connectNodes(svY, "next", svS, "in"); s.connectNodes(svS, "next", lp, "in");

    auto V = [&](const char* name, double x, double y) { return N(s, "getvar", x, y, {{"name", name}}); };

    // 1-2) déplacements avec les flèches
    auto* if1 = N(s, "if", -700, 0);
    auto* k1 = N(s, "g_key", -700, -120, {{"key", "Gauche"}});
    auto* l1 = N(s, "addvar", -700, 150, {{"name", "px"}, {"amount", "-6"}});
    auto* if2 = N(s, "if", -400, 0);
    auto* k2 = N(s, "g_key", -400, -120, {{"key", "Droite"}});
    auto* l2 = N(s, "addvar", -400, 150, {{"name", "px"}, {"amount", "6"}});
    s.connectNodes(lp, "body", if1, "in"); s.connectNodes(k1, "out", if1, "cond"); s.connectNodes(if1, "then", l1, "in");
    s.connectNodes(if1, "next", if2, "in"); s.connectNodes(k2, "out", if2, "cond"); s.connectNodes(if2, "then", l2, "in");
    // 3) l'étoile tombe
    auto* fall = N(s, "addvar", -100, 0, {{"name", "coiny"}, {"amount", "4"}});
    s.connectNodes(if2, "next", fall, "in");
    // 4) collision raquette / étoile
    auto* if4 = N(s, "if", 200, 0);
    auto* ov = N(s, "g_overlap", 200, -300, {{"y1", "440"}, {"w1", "80"}, {"h1", "20"}, {"w2", "28"}, {"h2", "28"}});
    auto* gp = V("px", -100, -340); auto* gx = V("coinx", -100, -260); auto* gy = V("coiny", -100, -180);
    s.connectNodes(fall, "next", if4, "in"); s.connectNodes(ov, "out", if4, "cond");
    s.connectNodes(gp, "out", ov, "x1"); s.connectNodes(gx, "out", ov, "x2"); s.connectNodes(gy, "out", ov, "y2");
    auto* addS = N(s, "addvar", 200, 150, {{"name", "score"}, {"amount", "1"}});
    auto* push = N(s, "setvar", 200, 260, {{"name", "coiny"}, {"type", "nombre"}, {"value", "500"}});
    s.connectNodes(if4, "then", addS, "in"); s.connectNodes(addS, "next", push, "in");
    // 5) l'étoile est sortie de l'écran : elle repart d'en haut, à une position au hasard
    auto* if5 = N(s, "if", 500, 0);
    auto* cmp = N(s, "compare", 500, -120, {{"op", ">"}, {"b", "480"}});
    auto* gy2 = V("coiny", 500, -250);
    s.connectNodes(if4, "next", if5, "in"); s.connectNodes(cmp, "out", if5, "cond"); s.connectNodes(gy2, "out", cmp, "a");
    auto* top = N(s, "setvar", 500, 150, {{"name", "coiny"}, {"type", "nombre"}, {"value", "0"}});
    auto* nx = N(s, "setvar", 500, 300, {{"name", "coinx"}, {"type", "nombre"}});
    auto* rnd = N(s, "random", 500, 460, {{"a", "20"}, {"b", "600"}});
    s.connectNodes(if5, "then", top, "in"); s.connectNodes(top, "next", nx, "in"); s.connectNodes(rnd, "out", nx, "value");
    // 6) dessin : raquette, score, étoile
    auto* rect = N(s, "g_rect", 800, 0, {{"y", "440"}, {"w", "80"}, {"h", "20"}, {"color", "bleu"}});
    auto* gp2 = V("px", 800, -120);
    s.connectNodes(if5, "next", rect, "in"); s.connectNodes(gp2, "out", rect, "x");
    auto* txt = N(s, "g_text", 1100, 0, {{"x", "10"}, {"y", "10"}, {"size", "24"}, {"color", "blanc"}});
    auto* jn = N(s, "join", 1100, -250, {{"a", "Score : "}});
    auto* gs = V("score", 1100, -350);
    s.connectNodes(rect, "next", txt, "in"); s.connectNodes(jn, "out", txt, "text"); s.connectNodes(gs, "out", jn, "b");
    auto* star = N(s, "g_text", 1400, 0, {{"text", "⭐"}, {"size", "28"}, {"color", "jaune"}});
    auto* gx3 = V("coinx", 1400, -250); auto* gy3 = V("coiny", 1400, -170);
    s.connectNodes(txt, "next", star, "in"); s.connectNodes(gx3, "out", star, "x"); s.connectNodes(gy3, "out", star, "y");
}

// ---------------------------------------------------------------------------
//  Ressources de démonstration (dessinées / synthétisées par le programme)
// ---------------------------------------------------------------------------
QByteArray pngBytes(const QImage& im) {
    QByteArray b;
    QBuffer buf(&b);
    buf.open(QIODevice::WriteOnly);
    im.save(&buf, "PNG");
    return b;
}

QImage makeRocket() {
    QImage im(64, 64, QImage::Format_ARGB32);
    im.fill(Qt::transparent);
    QPainter p(&im);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ff9800")); p.drawPolygon(QPolygonF({{26, 50}, {38, 50}, {32, 63}}));          // flamme
    p.setBrush(QColor("#e53935"));
    p.drawPolygon(QPolygonF({{20, 38}, {6, 58}, {20, 52}}));                                           // ailerons
    p.drawPolygon(QPolygonF({{44, 38}, {58, 58}, {44, 52}}));
    p.setBrush(QColor("#eceff1")); p.drawPolygon(QPolygonF({{32, 3}, {45, 24}, {45, 51}, {19, 51}, {19, 24}}));  // corps
    p.setBrush(QColor("#e53935")); p.drawPolygon(QPolygonF({{32, 3}, {40, 16}, {24, 16}}));            // nez
    p.setBrush(QColor("#29b6f6")); p.drawEllipse(QPointF(32, 31), 7, 7);                               // hublot
    p.setBrush(QColor("#b0bec5")); p.drawRect(QRectF(19, 44, 26, 5));
    return im;
}

QImage makeAsteroid() {
    QImage im(64, 64, QImage::Format_ARGB32);
    im.fill(Qt::transparent);
    QPainter p(&im);
    p.setRenderHint(QPainter::Antialiasing);
    static const double rad[] = {28, 24, 29, 22, 28, 25, 29, 23, 27, 25, 29, 24};
    QPolygonF poly;
    for (int i = 0; i < 12; ++i) poly << QPointF(32 + rad[i] * qCos(i * M_PI / 6), 32 + rad[i] * qSin(i * M_PI / 6));
    p.setPen(QPen(QColor("#5d4037"), 2));
    p.setBrush(QColor("#a0703f"));
    p.drawPolygon(poly);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#7b5230"));
    p.drawEllipse(QPointF(22, 24), 7, 6); p.drawEllipse(QPointF(40, 38), 8, 7); p.drawEllipse(QPointF(24, 44), 4, 4);
    return im;
}

// Son WAV 16 bits mono : f(t) renvoie une valeur entre -1 et 1
QByteArray makeWav(int ms, const std::function<double(double)>& f) {
    const int rate = 22050, n = rate * ms / 1000;
    QByteArray pcm(n * 2, 0);
    for (int i = 0; i < n; ++i) {
        const qint16 s = qint16(qBound(-1.0, f(double(i) / rate), 1.0) * 32767 * 0.6);
        pcm[2 * i] = char(s & 0xff);
        pcm[2 * i + 1] = char((s >> 8) & 0xff);
    }
    QByteArray w;
    auto le32 = [&](quint32 v) { for (int k = 0; k < 4; ++k) w.append(char((v >> (8 * k)) & 0xff)); };
    auto le16 = [&](quint16 v) { w.append(char(v & 0xff)); w.append(char(v >> 8)); };
    w.append("RIFF"); le32(36 + pcm.size()); w.append("WAVE");
    w.append("fmt "); le32(16); le16(1); le16(1); le32(rate); le32(rate * 2); le16(2); le16(16);
    w.append("data"); le32(pcm.size()); w.append(pcm);
    return w;
}

// ---------------------------------------------------------------------------
//  Jeu avec images et sons : on esquive les astéroïdes avec la fusée
// ---------------------------------------------------------------------------
void asteroids(NodeScene& s) {
    s.setAsset("fusee", Asset{"png", pngBytes(makeRocket())});
    s.setAsset("asteroide", Asset{"png", pngBytes(makeAsteroid())});
    s.setAsset("bip", Asset{"wav", makeWav(120, [](double t) { return qSin(2 * M_PI * 880 * t) * qExp(-t * 14); })});
    s.setAsset("boum", Asset{"wav", makeWav(450, [](double t) {
        const double noise = double((qint64(t * 22050) * 1103515245 + 12345) % 2001 - 1000) / 1000.0;
        return (noise * 0.7 + qSin(2 * M_PI * 70 * t)) * qExp(-t * 7);
    })});

    auto V = [&](const char* name, double x, double y) { return N(s, "getvar", x, y, {{"name", name}}); };
    auto* st = N(s, "start", -700, -420);
    auto* op = N(s, "g_open", -420, -420, {{"w", "640"}, {"h", "480"}, {"title", "Esquive les astéroïdes"}});
    auto* svP = N(s, "setvar", -140, -420, {{"name", "px"}, {"type", "nombre"}, {"value", "300"}});
    auto* svX = N(s, "setvar", 140, -420, {{"name", "ax"}, {"type", "nombre"}});
    auto* rnd0 = N(s, "random", 140, -270, {{"a", "20"}, {"b", "580"}});
    auto* svY = N(s, "setvar", 420, -420, {{"name", "ay"}, {"type", "nombre"}, {"value", "-40"}});
    auto* svS = N(s, "setvar", 700, -420, {{"name", "score"}, {"type", "nombre"}, {"value", "0"}});
    auto* lp = N(s, "g_loop", 980, -420, {{"bg", "noir"}});
    s.connectNodes(st, "next", op, "in"); s.connectNodes(op, "next", svP, "in"); s.connectNodes(svP, "next", svX, "in");
    s.connectNodes(svX, "next", svY, "in"); s.connectNodes(svY, "next", svS, "in"); s.connectNodes(svS, "next", lp, "in");
    s.connectNodes(rnd0, "out", svX, "value");

    // fin de partie (après la boucle)
    auto* over = N(s, "g_text", 1280, -420, {{"x", "190"}, {"y", "190"}, {"text", "GAME OVER"}, {"size", "56"}, {"color", "rouge"}});
    auto* fin = N(s, "g_text", 1560, -420, {{"x", "230"}, {"y", "270"}, {"size", "30"}, {"color", "blanc"}});
    auto* jf = N(s, "join", 1560, -300, {{"a", "Score : "}});
    auto* gsf = V("score", 1560, -190);
    auto* show = N(s, "g_show", 1840, -420);
    s.connectNodes(lp, "next", over, "in"); s.connectNodes(over, "next", fin, "in"); s.connectNodes(fin, "next", show, "in");
    s.connectNodes(jf, "out", fin, "text"); s.connectNodes(gsf, "out", jf, "b");

    // dans la boucle : déplacements
    auto* if1 = N(s, "if", -700, 0);
    auto* k1 = N(s, "g_key", -700, -120, {{"key", "Gauche"}});
    auto* l1 = N(s, "addvar", -700, 150, {{"name", "px"}, {"amount", "-7"}});
    auto* if2 = N(s, "if", -400, 0);
    auto* k2 = N(s, "g_key", -400, -120, {{"key", "Droite"}});
    auto* l2 = N(s, "addvar", -400, 150, {{"name", "px"}, {"amount", "7"}});
    s.connectNodes(lp, "body", if1, "in"); s.connectNodes(k1, "out", if1, "cond"); s.connectNodes(if1, "then", l1, "in");
    s.connectNodes(if1, "next", if2, "in"); s.connectNodes(k2, "out", if2, "cond"); s.connectNodes(if2, "then", l2, "in");
    // l'astéroïde tombe
    auto* fall = N(s, "addvar", -100, 0, {{"name", "ay"}, {"amount", "6"}});
    s.connectNodes(if2, "next", fall, "in");
    // collision : son « boum » puis fin de la boucle
    auto* ifHit = N(s, "if", 200, 0);
    auto* ov = N(s, "g_overlap", 200, -300, {{"y1", "420"}, {"w1", "40"}, {"h1", "44"}, {"w2", "36"}, {"h2", "36"}});
    auto* gp = V("px", -100, -340); auto* gx = V("ax", -100, -260); auto* gy = V("ay", -100, -180);
    s.connectNodes(fall, "next", ifHit, "in"); s.connectNodes(ov, "out", ifHit, "cond");
    s.connectNodes(gp, "out", ov, "x1"); s.connectNodes(gx, "out", ov, "x2"); s.connectNodes(gy, "out", ov, "y2");
    auto* boum = N(s, "g_sound", 200, 150, {{"snd", "boum"}});
    auto* brk = N(s, "break", 200, 260);
    s.connectNodes(ifHit, "then", boum, "in"); s.connectNodes(boum, "next", brk, "in");
    // l'astéroïde sort de l'écran : +1 point, « bip », nouvel astéroïde
    auto* ifOut = N(s, "if", 500, 0);
    auto* cmp = N(s, "compare", 500, -120, {{"op", ">"}, {"b", "480"}});
    auto* gy2 = V("ay", 500, -250);
    s.connectNodes(ifHit, "next", ifOut, "in"); s.connectNodes(cmp, "out", ifOut, "cond"); s.connectNodes(gy2, "out", cmp, "a");
    auto* addS = N(s, "addvar", 500, 150, {{"name", "score"}, {"amount", "1"}});
    auto* bip = N(s, "g_sound", 500, 260, {{"snd", "bip"}});
    auto* top = N(s, "setvar", 500, 370, {{"name", "ay"}, {"type", "nombre"}, {"value", "-40"}});
    auto* nx = N(s, "setvar", 500, 520, {{"name", "ax"}, {"type", "nombre"}});
    auto* rnd = N(s, "random", 500, 690, {{"a", "20"}, {"b", "580"}});
    s.connectNodes(ifOut, "then", addS, "in"); s.connectNodes(addS, "next", bip, "in"); s.connectNodes(bip, "next", top, "in");
    s.connectNodes(top, "next", nx, "in"); s.connectNodes(rnd, "out", nx, "value");
    // dessin : fusée, astéroïde qui tourne, score
    auto* ship = N(s, "g_image", 800, 0, {{"img", "fusee"}, {"y", "420"}, {"w", "48"}, {"h", "48"}, {"angle", "0"}});
    auto* gp2 = V("px", 800, -120);
    s.connectNodes(ifOut, "next", ship, "in"); s.connectNodes(gp2, "out", ship, "x");
    auto* ast = N(s, "g_image", 1100, 0, {{"img", "asteroide"}, {"w", "40"}, {"h", "40"}});
    auto* gx3 = V("ax", 1100, -250); auto* gy3 = V("ay", 1100, -170);
    auto* tm = N(s, "g_time", 1100, 230);
    auto* spin = N(s, "math", 1100, 300, {{"op", "*"}, {"b", "120"}});
    s.connectNodes(ship, "next", ast, "in"); s.connectNodes(gx3, "out", ast, "x"); s.connectNodes(gy3, "out", ast, "y");
    s.connectNodes(tm, "out", spin, "a"); s.connectNodes(spin, "out", ast, "angle");
    auto* txt = N(s, "g_text", 1400, 0, {{"x", "10"}, {"y", "10"}, {"size", "24"}, {"color", "blanc"}});
    auto* jn = N(s, "join", 1400, -250, {{"a", "Score : "}});
    auto* gs = V("score", 1400, -350);
    s.connectNodes(ast, "next", txt, "in"); s.connectNodes(jn, "out", txt, "text"); s.connectNodes(gs, "out", jn, "b");
}

}  // namespace

const QVector<ExampleInfo>& examples() {
    static const QVector<ExampleInfo> list = {
        {0, "Bonjour (saisie, variable, boucle)", true},
        {1, "Fonction : factorielle", true},
        {2, "Listes : somme des notes", true},
        {3, "Jeu : Devine le nombre (complet)", true},
        {4, "Jeu : Quiz", true},
        {5, "Animation : planète en orbite (graphique)", true},
        {6, "Jeu : Attrape les étoiles (graphique, clavier)", true},
        {7, "Jeu : Esquive les astéroïdes (images et sons)", true},
        {100, "Devine le nombre — étape 1 : choisir et lire", false},
        {101, "Devine le nombre — étape 2 : comparer", false},
    };
    return list;
}

const char* exampleName(int id) {
    for (const ExampleInfo& e : examples())
        if (e.id == id) return e.name;
    return "";
}

void buildExample(NodeScene& scene, int id) {
    scene.clearAll();
    switch (id) {
    case 0: hello(scene); break;
    case 1: factorial(scene); break;
    case 2: lists(scene); break;
    case 3: guessFinal(scene); break;
    case 4: quiz(scene); break;
    case 5: orbit(scene); break;
    case 6: catchStars(scene); break;
    case 7: asteroids(scene); break;
    case 100: guessA(scene); break;
    case 101: guessB(scene); break;
    default: break;
    }
    scene.clearSelection();
}
