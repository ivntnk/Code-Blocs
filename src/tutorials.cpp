#include "tutorials.h"

namespace {
TutorialStep S(const QString& title, const QString& html, int example = -1, const QString& label = QString()) {
    return {title, html, example, label};
}
}  // namespace

const QVector<Tutorial>& tutorials() {
    static QVector<Tutorial> t;
    if (!t.isEmpty()) return t;

    // ------------------------------------------------------------------ 1
    t << Tutorial{"1. Bienvenue : comprendre Blocs", "Le principe, ajouter et relier des blocs, exécuter.", {
        S("Le principe", R"HTML(
<h2>Programmer en reliant des cases</h2>
<p>Chaque <b>bloc</b> fait une petite chose (afficher, calculer, comparer…). En les <b>reliant</b>, tu écris un programme.
À droite, Blocs écrit pour toi le code <b>C#</b> correspondant, en direct.</p>
<h3>Deux sortes de fils</h3>
<ul>
<li><b>▶ Le flux</b> (fils blancs) : l'<i>ordre</i> d'exécution. Tout part du bloc vert <b>▶ Début</b>, puis on suit les flèches.</li>
<li><b>● Les données</b> (fils colorés) : une <i>valeur</i> qui passe d'un bloc à l'autre.</li>
</ul>
<h3>Les couleurs des ronds ●</h3>
<p><span style="color:#ffb74d">orange = nombre</span> · <span style="color:#81c784">vert = texte</span> ·
<span style="color:#e57373">rouge = booléen (vrai/faux)</span> · <span style="color:#ba68c8">violet = liste</span> ·
<span style="color:#b0bec5">gris = s'adapte</span>.<br>Deux ronds de couleurs différentes ne se relient pas : Blocs te le dit.</p>
)HTML"),
        S("Ajouter, relier, supprimer", R"HTML(
<h2>Les gestes de base</h2>
<ul>
<li><b>Ajouter un bloc</b> : clique sur son nom dans la liste de gauche, ou fais un <b>clic droit</b> (ou double-clic) sur le fond → « Ajouter un bloc ».</li>
<li><b>Relier</b> : glisse d'un rond vers un autre. Les ▶ avec les ▶, les ● avec les ●.</li>
<li><b>Régler un bloc</b> : clique dessus, puis modifie ses valeurs dans <b>Propriétés</b> (à droite). L'aide du bloc s'y affiche.</li>
<li><b>Déplacer</b> : glisse le bloc. Clic droit (ou molette) + glisser : déplace toute la vue. Molette : zoom.</li>
<li><b>Supprimer</b> : sélectionne puis <code>Suppr</code>, ou clic droit → Supprimer. Clic droit sur un rond : le déconnecte.</li>
<li><b>Copier / coller</b> : <code>Ctrl+C</code>, <code>Ctrl+V</code>. <b>Annuler</b> : <code>Ctrl+Z</code>.</li>
</ul>
<p>💡 Si un rond ● n'est relié à rien, sa <i>valeur par défaut</i> (visible dans le bloc) est utilisée.</p>
)HTML"),
        S("Exécuter et déboguer", R"HTML(
<h2>Lancer ton programme</h2>
<ul>
<li><code>F5</code> (ou ▶ Exécuter) compile et lance. Le résultat s'affiche dans la <b>Console</b> en bas.</li>
<li>Quand le programme pose une question, tape ta réponse dans le champ sous la console puis <code>Entrée</code>.</li>
<li><code>Maj+F5</code> (■ Arrêter) stoppe un programme qui tourne trop longtemps.</li>
<li>Le panneau <b>Code C# généré</b> montre le code. <code>Ctrl+E</code> l'exporte en fichier <code>.cs</code>.</li>
</ul>
<h3>Si rien ne se lance</h3>
<p>Un cadre <span style="color:#ff9e9e">rouge</span> sous le code liste les <b>problèmes</b> trouvés (« variable jamais définie », « type incorrect »…). Corrige-les puis relance.
Il faut aussi avoir installé le <b>SDK .NET</b>.</p>
)HTML")}};

    // ------------------------------------------------------------------ 2
    t << Tutorial{"2. Ton premier programme", "Afficher, demander, utiliser une variable.", {
        S("Afficher un message", R"HTML(
<h2>Étape 1 : « Bonjour »</h2>
<ol>
<li>Ajoute <b>▶ Début</b> (Flux).</li>
<li>Ajoute <b>Afficher</b> (Entrées / Sorties) et relie le ▶ de Début au ▶ d'Afficher.</li>
<li>Clique sur Afficher et écris ton message dans « valeur ».</li>
<li>Appuie sur <code>F5</code> !</li>
</ol>
)HTML"),
        S("Demander son prénom", R"HTML(
<h2>Étape 2 : poser une question</h2>
<p>Le bloc <b>Demander (texte)</b> pose une question et <i>renvoie la réponse</i> (toujours du texte).</p>
<ol>
<li>Ajoute <b>Définir variable</b> : nom <code>prenom</code>, type <code>texte</code>.</li>
<li>Relie la sortie ● de <b>Demander</b> à l'entrée « valeur » de Définir variable.</li>
<li>Relie : Début → Définir variable → Afficher.</li>
</ol>
<p>💡 <b>Règle d'or :</b> « Demander » et « Nombre aléatoire » donnent une <i>nouvelle</i> valeur à chaque lecture.
Pour la <b>garder</b>, range-la dans une <b>variable</b> puis relis la variable.</p>
)HTML"),
        S("Assembler du texte", R"HTML(
<h2>Étape 3 : « Bonjour Marie ! »</h2>
<ol>
<li>Ajoute <b>Assembler texte</b> : texte A = <code>Bonjour </code> (avec l'espace).</li>
<li>Ajoute un bloc <b>Variable</b> (nom <code>prenom</code>) et relie-le à « texte B ».</li>
<li>Relie la sortie d'Assembler texte à la « valeur » d'Afficher.</li>
</ol>
<p>Charge l'exemple pour voir le résultat complet, puis lance-le avec <code>F5</code>.</p>
)HTML", 0, "Charger l'exemple « Bonjour »")}};

    // ------------------------------------------------------------------ 3
    t << Tutorial{"3. Variables, calculs et conditions", "Mémoriser, calculer, choisir avec Si / Sinon.", {
        S("Les variables", R"HTML(
<h2>Une variable = une boîte avec un nom</h2>
<ul>
<li><b>Définir variable</b> : crée ou modifie. Choisis son <b>type</b> : nombre, texte, booléen, liste.</li>
<li><b>Ajouter à variable</b> : <code>score + 10</code>. Mets une quantité négative pour retirer.</li>
<li><b>Variable</b> (bloc valeur) : lit sa valeur.</li>
</ul>
<p>Une variable <b>locale</b> n'existe que dans son bloc de départ. Mets <b>portée = globale</b> pour qu'elle soit aussi visible dans les <b>fonctions</b>.</p>
)HTML"),
        S("Calculer", R"HTML(
<h2>Faire des maths</h2>
<ul>
<li><b>Opération</b> : <code>+ − × / %</code> (le <code>%</code> donne le reste), <code>//</code> (division entière), puissance.</li>
<li><b>Fonction mathématique</b> : valeur absolue, racine, arrondis, sinus, cosinus.</li>
<li><b>Minimum / Maximum</b>, <b>Nombre aléatoire</b> (entier) et <b>Décimal aléatoire (0-1)</b>.</li>
</ul>
<p>Pour calculer plusieurs choses : chaîne les blocs. Exemple <code>(a + b) × 2</code> : un bloc « + » relié à l'entrée d'un bloc « × ».</p>
)HTML"),
        S("Si / Sinon", R"HTML(
<h2>Prendre une décision</h2>
<ol>
<li>Ajoute <b>Si</b>. Relie à son entrée « condition » un bloc <b>Comparaison</b> (ex. <code>age &gt;= 18</code>).</li>
<li>Ce qui est relié à <b>Alors</b> s'exécute si c'est vrai, <b>Sinon</b> si c'est faux, <b>Ensuite</b> dans tous les cas.</li>
<li>Pour combiner des conditions : <b>Logique (et / ou)</b> et <b>Non</b>.</li>
<li>Plusieurs cas ? Mets un <b>Si</b> dans le « Sinon » d'un autre Si (c'est ce que fait le jeu « Devine le nombre »).</li>
</ol>
<p>⚠️ On compare des choses du <b>même type</b> (nombre avec nombre, texte avec texte).</p>
)HTML")}};

    // ------------------------------------------------------------------ 4
    t << Tutorial{"4. Les boucles", "Répéter, compter, tant que, arrêter.", {
        S("Répéter N fois", R"HTML(
<h2>Répéter</h2>
<p><b>Répéter</b> exécute son « Répéter » N fois. Le <b>compteur</b> (par défaut <code>i</code>) vaut 0, 1, 2… N−1 ; lis-le avec un bloc <b>Variable</b> nommé <code>i</code>.</p>
<p>« Ensuite » continue <i>après</i> la boucle.</p>
)HTML"),
        S("Compter de … à …", R"HTML(
<h2>Compter</h2>
<p><b>Compter de … à …</b> : de, à (inclus), pas. Exemples :</p>
<ul>
<li>1 → 10, pas 1 : table de multiplication</li>
<li>10 → 1, pas <code>-1</code> : compte à rebours (ajoute <b>Attendre</b> 1 seconde dans la boucle)</li>
</ul>
)HTML"),
        S("Tant que, Arrêter, Tour suivant", R"HTML(
<h2>Boucle conditionnelle</h2>
<ul>
<li><b>Tant que</b> répète tant que la condition est vraie. Il faut qu'<i>quelque chose change</i> dans la boucle, sinon elle ne s'arrête jamais (■ Stop te sauve).</li>
<li><b>Arrêter la boucle</b> : sort tout de suite.</li>
<li><b>Tour suivant</b> : saute à l'itération suivante.</li>
</ul>
<p>Ces deux blocs doivent être placés <i>dans</i> une boucle, sinon Blocs te le signale.</p>
)HTML")}};

    // ------------------------------------------------------------------ 5
    t << Tutorial{"5. Jeu : Devine le nombre", "Un vrai jeu, construit en 3 étapes.", {
        S("L'idée du jeu", R"HTML(
<h2>Devine le nombre</h2>
<p>L'ordinateur choisit un nombre secret entre 1 et 100. Tu proposes un nombre, il répond <i>« Trop petit »</i> ou <i>« Trop grand »</i>, jusqu'à ce que tu trouves. À la fin, il affiche ton nombre d'essais.</p>
<p>On le construit en <b>3 étapes</b> ; à chacune tu peux charger le projet correspondant pour comparer avec le tien.</p>
)HTML"),
        S("Étape 1 : choisir et lire", R"HTML(
<h2>Le nombre secret et la réponse</h2>
<ul>
<li><b>Définir variable</b> <code>secret</code> (nombre) ← <b>Nombre aléatoire</b> min 1, max 100.</li>
<li><b>Définir variable</b> <code>reponse</code> (nombre) ← <b>Texte → nombre</b> ← <b>Demander</b> « Ton choix ? ».</li>
</ul>
<p>💡 <b>Texte → nombre</b> a une valeur « <i>si invalide</i> » : si le joueur tape « abc », le programme ne plante pas.</p>
<p>💡 Le nombre aléatoire est rangé dans la variable <code>secret</code> : sinon il changerait à chaque lecture !</p>
)HTML", 100, "Charger l'étape 1"),
        S("Étape 2 : comparer", R"HTML(
<h2>Trop petit, trop grand, gagné</h2>
<p>Deux <b>Si</b> imbriqués :</p>
<ol>
<li>Si <code>reponse &lt; secret</code> → afficher « Trop petit ! ».</li>
<li>Sinon : Si <code>reponse &gt; secret</code> → « Trop grand ! », sinon → « Bravo ! ».</li>
</ol>
<p>Pour chaque comparaison : un bloc <b>Comparaison</b> avec deux blocs <b>Variable</b> (<code>reponse</code> et <code>secret</code>).</p>
)HTML", 101, "Charger l'étape 2"),
        S("Étape 3 : jouer jusqu'à trouver", R"HTML(
<h2>La boucle</h2>
<ul>
<li>Variable <code>essais</code> (nombre) = 0 et <code>trouve</code> (booléen) = false.</li>
<li><b>Tant que</b> <code>NON trouve</code> : demander, <b>Ajouter à variable</b> essais +1, comparer ; quand c'est gagné, mettre <code>trouve</code> à <code>true</code>.</li>
<li>Après la boucle : <b>Assembler texte</b> « Bravo ! Trouvé en » + essais + « essai(s) ».</li>
</ul>
)HTML", 3, "Charger le jeu complet"),
        S("Pour aller plus loin", R"HTML(
<h2>Idées d'améliorations</h2>
<ul>
<li><b>Limiter les essais</b> : condition du Tant que = <code>NON trouve ET essais &lt; 10</code>.</li>
<li><b>Niveaux</b> : demander le nombre maximum au début et l'utiliser dans le Nombre aléatoire.</li>
<li><b>Rejouer</b> : mets tout dans une boucle qui demande « Rejouer ? (o/n) ».</li>
<li><b>Indice</b> : afficher « Tu chauffes ! » si l'écart est petit (Fonction mathématique <i>abs</i>).</li>
</ul>
)HTML")}};

    // ------------------------------------------------------------------ 6
    t << Tutorial{"6. Jeu : Quiz", "Listes de questions et de réponses, score.", {
        S("Le principe", R"HTML(
<h2>Un quiz avec deux listes</h2>
<p>Une liste de <b>questions</b> et une liste de <b>réponses</b> dans le même ordre. On les parcourt avec un compteur.</p>
<ul>
<li>Variable <code>questions</code> de type <i>liste de textes</i>, valeur : questions séparées par des virgules.</li>
<li>Variable <code>reponses</code> idem, et <code>score</code> (nombre) = 0.</li>
</ul>
)HTML"),
        S("La boucle des questions", R"HTML(
<h2>Poser chaque question</h2>
<ol>
<li><b>Compter de … à …</b> : de 0 à <code>taille de la liste − 1</code> (bloc <b>Taille de la liste</b> puis <b>Opération</b> « − 1 »).</li>
<li>Dans la boucle : <b>Élément de la liste</b> (liste <code>questions</code>, position <code>i</code>) → <b>Demander</b> → <b>Majuscules / minuscules</b> (minuscules) → variable <code>reponse</code>.</li>
<li>Si <code>reponse == élément n° i de reponses</code> → « Correct ! » et <b>Ajouter à variable</b> score +1, sinon « Raté… ».</li>
</ol>
)HTML", 4, "Charger le jeu « Quiz »"),
        S("Idées pour l'améliorer", R"HTML(
<ul>
<li>Plusieurs réponses acceptées (utilise <b>La liste contient ?</b>).</li>
<li>Mélanger l'ordre des questions avec <b>Nombre aléatoire</b>.</li>
<li>Afficher un message selon le score (Si / Sinon).</li>
<li>Ignorer les espaces : <b>Majuscules / minuscules → sans espaces autour</b>.</li>
</ul>
)HTML")}};

    // ------------------------------------------------------------------ 7
    t << Tutorial{"7. Les fonctions", "Réutiliser un bout de programme.", {
        S("Définir une fonction", R"HTML(
<h2>Une fonction = un programme nommé</h2>
<ol>
<li>Ajoute <b>▶ Fonction</b> (c'est un point de départ, comme Début). Donne-lui un <b>nom</b>.</li>
<li><b>Paramètres</b> (les « ingrédients ») : <code>nom:type</code> séparés par des virgules, 6 maximum. Ex. <code>a:nombre, titre:texte</code>.</li>
<li><b>Retour</b> : le type de ce qu'elle renvoie, ou « aucun ».</li>
<li>Relie le corps à sa sortie ▶. Dans le corps, un bloc <b>Variable</b> avec le nom du paramètre donne sa valeur.</li>
<li>Termine par <b>Retourner</b> si la fonction renvoie quelque chose.</li>
</ol>
)HTML"),
        S("Appeler une fonction", R"HTML(
<h2>Utiliser la fonction</h2>
<ul>
<li><b>Appeler fonction</b> : l'exécute (quand on n'a pas besoin du résultat).</li>
<li><b>Appeler (valeur)</b> : utilise le résultat dans un calcul. Les <b>arguments</b> se donnent dans l'ordre des paramètres.</li>
</ul>
<p>⚠️ Les variables « locales » de Début ne sont pas visibles dans une fonction. Pour partager : <b>portée = globale</b>.</p>
<p>Charge l'exemple <b>factorielle</b> : une fonction qui s'appelle elle-même (récursion) !</p>
)HTML", 1, "Charger l'exemple « factorielle »")}};

    // ------------------------------------------------------------------ 8
    t << Tutorial{"8. Listes et texte", "Ranger plusieurs valeurs, manipuler du texte.", {
        S("Les listes", R"HTML(
<h2>Une liste = plusieurs valeurs rangées</h2>
<ul>
<li>Crée-la avec <b>Définir variable</b> (type liste), valeur initiale : <code>12, 8, 15</code>.</li>
<li>Les autres blocs désignent la liste par le <b>nom</b> de la variable, dans leur champ « liste ».</li>
<li><b>Ajouter, Retirer, Remplacer, Vider, Trier / inverser, Taille, Position, La liste contient ?, Liste → texte</b>.</li>
<li><b>Pour chaque élément</b> : parcourt la liste.</li>
<li>Les positions commencent à <b>0</b> (le premier élément est n° 0).</li>
</ul>
)HTML", 2, "Charger l'exemple « Listes »"),
        S("Manipuler du texte", R"HTML(
<h2>La catégorie « Texte »</h2>
<ul>
<li><b>Longueur</b>, <b>Majuscules / minuscules</b>, <b>Extraire</b>, <b>Caractère n°</b>, <b>Remplacer</b>.</li>
<li><b>Le texte contient ?</b> (contient / commence par / finit par).</li>
<li><b>Découper en liste</b> : <code>"a,b,c"</code> avec le séparateur « , » donne une liste de 3 textes.</li>
<li><b>Texte → nombre</b> et <b>Est un nombre ?</b> pour vérifier ce que tape le joueur.</li>
</ul>
)HTML")}};

    // ------------------------------------------------------------------ 9
    t << Tutorial{"9. Aller plus loin : écrire du C#", "Le code libre, quand les blocs ne suffisent pas.", {
        S("Les blocs de code libre", R"HTML(
<h2>Aucune limite grâce au C#</h2>
<ul>
<li><b>Code C#</b> : une ou plusieurs instructions C#, à n'importe quel endroit du flux.</li>
<li><b>Expression C#</b> : une valeur calculée en C# (ex. <code>DateTime.Now.Hour</code>), avec son type.</li>
<li><b>Code C# global</b> : méthodes, classes, structures… placées dans le programme et utilisables ensuite par les deux blocs ci-dessus.</li>
</ul>
<p>Exemple : un bloc <i>Code C# global</i> <code>static double Triple(double x) { return x * 3; }</code>, puis une <i>Expression C#</i> <code>Triple(4)</code> (type nombre) → 12.</p>
)HTML"),
        S("Les limites actuelles", R"HTML(
<h2>Ce que Blocs ne fait pas (encore) avec des blocs</h2>
<ul>
<li>Programmes en <b>mode texte</b> (console) : pas de fenêtre graphique.</li>
<li>Listes de nombres ou de textes seulement ; pas de dictionnaires ni de classes <i>en blocs</i> (mais possible avec « Code C# global »).</li>
<li>6 paramètres maximum par fonction.</li>
<li>Pas de lecture/écriture de fichiers en blocs (utilise « Code C# » : <code>System.IO.File</code>).</li>
</ul>
<p>Le code est du <b>vrai C#</b> : tu peux l'exporter (<code>Ctrl+E</code>) et le continuer dans Visual Studio.</p>
)HTML")}};

    // ------------------------------------------------------------------ 10
    t << Tutorial{"10. Recettes et idées de jeux", "Comment faire… (pile ou face, dé, rejouer…).", {
        S("Recettes rapides", R"HTML(
<h2>Comment faire…</h2>
<ul>
<li><b>Un dé à 6 faces</b> : <i>Nombre aléatoire</i> min 1, max 6.</li>
<li><b>Pile ou face</b> : <i>Si</i> <code>Décimal aléatoire &lt; 0,5</code> → « Pile », sinon « Face ».</li>
<li><b>Compte à rebours</b> : <i>Compter</i> de 10 à 1, pas −1, avec <i>Attendre</i> 1 seconde.</li>
<li><b>Rejouer ?</b> : boucle <i>Tant que</i> dont la condition dépend d'une réponse « o/n » (Comparaison de textes).</li>
<li><b>Choisir au hasard dans une liste</b> : <i>Élément de la liste</i> avec position = <i>Nombre aléatoire</i> de 0 à <i>taille − 1</i>.</li>
<li><b>Compter des occurrences</b> : <i>Pour chaque</i> + <i>Si</i> + <i>Ajouter à variable</i>.</li>
<li><b>Effacer l'écran</b> : bloc <i>Effacer l'écran</i>.</li>
</ul>
)HTML"),
        S("Idées de jeux à construire", R"HTML(
<h2>À toi de jouer</h2>
<ul>
<li><b>Pierre-feuille-ciseaux</b> : liste de 3 textes, choix de l'ordinateur au hasard, comparaisons avec <i>Logique (et / ou)</i>. Une <i>fonction</i> « gagne(a, b) » simplifie tout.</li>
<li><b>Le pendu</b> : un mot (texte), une liste des lettres proposées, <i>Caractère n°</i> pour tester chaque lettre, nombre d'erreurs.</li>
<li><b>Morpion (tic-tac-toe)</b> : une liste de 9 textes pour les cases, une fonction pour afficher la grille.</li>
<li><b>Calculatrice</b> : demander 2 nombres et un opérateur, puis des <i>Si</i> pour choisir l'opération.</li>
<li><b>Aventure textuelle</b> : des <i>Si</i> / <i>Sinon</i> en cascade selon les choix du joueur.</li>
</ul>
)HTML"),
        S("Quand ça ne marche pas", R"HTML(
<h2>Résoudre les problèmes</h2>
<ul>
<li><b>« la variable … n'est jamais définie »</b> : ajoute un bloc <i>Définir variable</i> avant, avec le même nom.</li>
<li><b>« attend un nombre mais reçoit un texte »</b> : convertis avec <i>Texte → nombre</i>, ou assemble avec <i>Assembler texte</i>.</li>
<li><b>La réponse est redemandée plusieurs fois</b> : un bloc <i>Demander</i> relié à plusieurs endroits est relu à chaque fois → range-le dans une variable.</li>
<li><b>Le programme ne s'arrête pas</b> : vérifie la condition de ta boucle <i>Tant que</i> ; ■ Arrêter pour stopper.</li>
<li><b>« doit être placé dans une boucle »</b> : <i>Arrêter la boucle</i> / <i>Tour suivant</i> doivent être dans le corps d'une boucle.</li>
</ul>
)HTML")}};

    // ------------------------------------------------------------------ 11
    t << Tutorial{"11. Jeux graphiques (fenêtre, clavier)", "Dessiner, animer, jouer avec le clavier et la souris.", {
        S("La fenêtre et la boucle de jeu", R"HTML(
<h2>Une vraie fenêtre de jeu</h2>
<p>Dans la catégorie <b style="color:#fb8c00">Jeu graphique</b> :</p>
<ol>
<li><b>Ouvrir la fenêtre de jeu</b> (largeur, hauteur, titre).</li>
<li><b>Boucle de jeu</b> : se répète ~60 fois par seconde. À chaque tour elle <i>efface l'écran</i> avec la couleur de fond, exécute ce qui est relié à « À chaque image », puis <i>affiche</i> le résultat.</li>
</ol>
<h3>Les coordonnées</h3>
<p>Le point <code>(0, 0)</code> est en <b>haut à gauche</b>. <code>x</code> augmente vers la <b>droite</b>, <code>y</code> augmente vers le <b>bas</b>. Une fenêtre 640 × 480 va donc de (0,0) à (640,480).</p>
<p>La fenêtre de jeu s'ouvre à part quand tu lances le programme (<code>F5</code>). <b>Clique dedans</b> pour que le clavier lui soit envoyé.</p>
)HTML"),
        S("Dessiner et animer", R"HTML(
<h2>Rectangle, cercle, ligne, texte</h2>
<ul>
<li><b>Rectangle</b> : x, y (coin haut-gauche), largeur, hauteur, couleur, plein ou contour.</li>
<li><b>Cercle</b> : x, y (centre), rayon, couleur.</li>
<li><b>Ligne</b> et <b>Texte à l'écran</b> (les emojis ⭐ 🚀 👾 marchent).</li>
<li><b>Couleurs</b> : noir, blanc, rouge, vert, bleu, jaune, orange, violet, rose, marron, gris, cyan, ou <code>#RRGGBB</code>.</li>
</ul>
<h3>Animer</h3>
<p>Pour qu'un dessin bouge, ses coordonnées doivent <b>changer à chaque image</b> : soit avec une variable (<code>x</code> augmente de 3 à chaque tour), soit avec le bloc <b>Temps écoulé</b> combiné à sinus/cosinus.</p>
<p>L'exemple charge une <b>planète en orbite</b> : <code>x = 320 + 200 × cos(temps)</code> et <code>y = 240 + 200 × sin(temps)</code>.</p>
)HTML", 5, "Charger l'animation « Orbite »"),
        S("Clavier et souris", R"HTML(
<h2>Faire réagir le jeu</h2>
<ul>
<li><b>Touche enfoncée ?</b> : vrai <i>tant que</i> la touche est maintenue. Idéal pour se déplacer : <i>Si</i> touche « Gauche » enfoncée → <i>Ajouter à variable</i> <code>x</code> de −5.</li>
<li><b>Touche appuyée (une fois)</b> : vrai <i>une seule fois</i> par appui. Pour tirer, sauter, changer d'arme…</li>
<li><b>Souris : x</b>, <b>Souris : y</b>, <b>Clic souris enfoncé ?</b>.</li>
</ul>
<p>Touches disponibles : flèches, Espace, Entrée, Échap, lettres A–Z, chiffres 0–9. Les lettres correspondent à la lettre écrite sur la touche (ZQSD sur un clavier AZERTY).</p>
)HTML"),
        S("Jeu : Attrape les étoiles", R"HTML(
<h2>Un jeu complet</h2>
<p>Une raquette bleue en bas, une étoile ⭐ qui tombe. Les flèches ← → déplacent la raquette ; chaque étoile attrapée donne un point.</p>
<h3>Comment c'est construit</h3>
<ol>
<li>Variables : <code>px</code> (position de la raquette), <code>coinx</code> / <code>coiny</code> (étoile), <code>score</code>.</li>
<li>Dans la boucle : deux <b>Si</b> avec <i>Touche enfoncée ?</i> modifient <code>px</code>.</li>
<li>L'étoile descend : <i>Ajouter à variable</i> <code>coiny</code> +4.</li>
<li><b>Rectangles qui se touchent ?</b> compare la raquette et l'étoile : si oui, score +1 et l'étoile est envoyée hors de l'écran.</li>
<li>Si <code>coiny &gt; 480</code> : l'étoile repart d'en haut, à un <code>x</code> aléatoire.</li>
<li>Pour finir, on <b>dessine</b> : raquette, score, étoile.</li>
</ol>
<p>💡 Le dessin se fait <i>après</i> les calculs, pour afficher les positions à jour.</p>
)HTML", 6, "Charger le jeu « Attrape les étoiles »"),
        S("Pour l'améliorer", R"HTML(
<h2>À toi d'ajouter…</h2>
<ul>
<li><b>Garder la raquette dans l'écran</b> : après le déplacement, <i>Définir px</i> = <i>Minimum/Maximum</i> (max 0, puis min 560).</li>
<li><b>Difficulté croissante</b> : la vitesse de chute dépend du score (<code>4 + score ÷ 5</code>).</li>
<li><b>Vies et « Game over »</b> : une variable <code>vies</code> qui baisse quand l'étoile est ratée ; à 0, affiche « GAME OVER » puis <b>Arrêter la boucle</b>.</li>
<li><b>Bip sonore</b> quand on attrape une étoile.</li>
<li><b>Pong</b> : une balle (x, y, vitesses), des rebonds avec <i>Si x &lt; 0 ou x &gt; 640</i> → inverser la vitesse (× −1), deux raquettes.</li>
<li><b>Mouvement régulier</b> : multiplie les vitesses par <b>Durée de l'image</b> pour que le jeu aille à la même vitesse sur tous les ordinateurs.</li>
</ul>
)HTML"),
        S("Bon à savoir", R"HTML(
<h2>Limites et astuces</h2>
<ul>
<li>Pendant que tu le construis, le jeu s'affiche dans la <b>fenêtre de jeu de Blocs</b>. Pour le <b>partager</b> avec une vraie fenêtre Windows : <b>Fichier &gt; Exporter l'application (.exe)</b> (voir le tutoriel 12).</li>
<li>Pas de « Demander » dans un jeu graphique : le clavier est utilisé par la fenêtre.</li>
<li>Fermer la fenêtre de jeu arrête le programme.</li>
<li>Si les touches ne répondent pas : <b>clique dans la fenêtre de jeu</b> pour lui donner le focus.</li>
<li>Un dessin fixe (sans animation) : ouvre la fenêtre, dessine, puis <b>Afficher l'image maintenant</b>.</li>
</ul>
)HTML")}};

    // ------------------------------------------------------------------ 12
    t << Tutorial{"12. Images, sons et partage de ton jeu", "Sprites, sons WAV, et export en application Windows (.exe).", {
        S("Importer des images (sprites)", R"HTML(
<h2>Des vraies images dans ton jeu</h2>
<ol>
<li>Menu <b>Jeu &gt; Importer une image…</b> : choisis un fichier PNG, JPG, GIF ou BMP (8 Mo maximum). Un <b>PNG transparent</b> est idéal pour un personnage.</li>
<li>Donne-lui un <b>nom</b> (ex. <code>fusee</code>). Il est enregistré <i>dans ton fichier .blocs</i> : tu peux l'envoyer à un ami sans fichiers à part.</li>
<li>Dans la boucle de jeu, utilise <b>Dessiner une image (sprite)</b> : écris le nom dans « image » (ou choisis-le dans la liste sous le champ), puis x, y, largeur, hauteur et <b>angle</b> (en degrés, autour du centre).</li>
</ol>
<p>💡 <b>Ressources du projet</b> (menu Jeu) liste tout ce que tu as importé, avec des aperçus.</p>
<p>L'exemple génère lui-même une fusée et un astéroïde qui tourne : charge-le pour voir les blocs.</p>
)HTML", 7, "Charger « Esquive les astéroïdes »"),
        S("Ajouter des sons", R"HTML(
<h2>Jouer un son</h2>
<ol>
<li><b>Jeu &gt; Importer un son (WAV)…</b> : fichiers <b>.wav</b> uniquement (formats .mp3 non pris en charge ; tu peux convertir avec un logiciel comme Audacity : « Exporter en WAV 16 bits »).</li>
<li>Bloc <b>Jouer un son</b> : écris le nom du son. Utilise-le quand quelque chose se passe : un point gagné, une collision…</li>
<li>Le bloc <b>Bip sonore</b> joue le bip du système, sans rien à importer.</li>
</ol>
<p>Un seul son joue à la fois : si un nouveau son démarre, il remplace le précédent. Garde les sons <b>courts</b>.</p>
)HTML"),
        S("Partager : exporter en .exe", R"HTML(
<h2>Fichier &gt; Exporter l'application (.exe)…</h2>
<p>Ton projet devient un vrai programme Windows, <b>sans Blocs</b> : un jeu graphique ouvre sa propre fenêtre. Trois choix :</p>
<ul>
<li><b>.exe léger</b> : un petit fichier. L'autre PC doit avoir le « <i>.NET Desktop Runtime</i> » (gratuit, installé sur beaucoup de PC).</li>
<li><b>.exe autonome</b> : tout est inclus (gros fichier, ~150 Mo). Il marche sur <i>n'importe quel</i> Windows. Il faut Internet <i>pour le fabriquer</i> (téléchargement de .NET).</li>
<li><b>Projet Visual Studio</b> : un dossier à ouvrir et modifier dans Visual Studio.</li>
</ul>
<p>Choisis le nom puis le dossier. Blocs fabrique l'application, puis ouvre le dossier. Tes images et sons sont <b>dedans</b> (un seul fichier .exe à envoyer).</p>
)HTML"),
        S("Bon à savoir sur l'export", R"HTML(
<h2>Avant d'envoyer ton jeu</h2>
<ul>
<li><b>Windows peut afficher « Windows a protégé votre ordinateur »</b> (SmartScreen) : c'est normal pour un programme créé par quelqu'un qui n'est pas un éditeur connu. Il suffit de cliquer sur <i>Informations complémentaires</i> puis <i>Exécuter quand même</i>.</li>
<li>Un jeu graphique n'a <b>pas de console</b> : les blocs « Afficher » ne montrent rien. Utilise « Texte à l'écran ».</li>
<li>Les <b>sons</b> : WAV seulement ; la fenêtre des « Ressources » signale ce qui manque.</li>
<li>Une application <b>console</b> (sans blocs de jeu graphique) est exportée comme programme en ligne de commande.</li>
<li>Si l'export échoue, la console de Blocs affiche la raison (souvent : Internet pour la version « autonome »).</li>
</ul>
)HTML")}};

    // ------------------------------------------------------------------ 13
    t << Tutorial{"13. Organiser ton projet (explorateur)", "Dossier de projet, plusieurs programmes, fichiers.", {
        S("Ouvrir un dossier de projet", R"HTML(
<h2>Un dossier = un projet</h2>
<p>Comme dans un éditeur de code, Blocs peut afficher <b>le dossier de ton projet</b> dans l'onglet <b>Explorateur</b> (à côté de l'onglet « Blocs », à gauche).</p>
<ul>
<li><b>Fichier &gt; Ouvrir un dossier…</b> (<code>Ctrl+Maj+O</code>) : choisis un dossier existant.</li>
<li><b>Fichier &gt; Nouveau projet (dossier)…</b> : crée un dossier tout neuf avec un premier programme, prêt à l'emploi.</li>
<li><b>Dossiers récents</b> : retrouve tes projets en un clic. Blocs se souvient du dernier dossier ouvert.</li>
</ul>
<p>Quand tu enregistres un programme pour la première fois, il est proposé <i>dans ce dossier</i>. Si aucun dossier n'est ouvert, c'est celui de ton programme qui devient le dossier du projet.</p>
)HTML"),
        S("Créer, renommer, ranger", R"HTML(
<h2>Gérer tes fichiers</h2>
<ul>
<li>Le bouton <b>＋</b> (ou un <b>clic droit</b>) : <i>Nouveau programme Blocs</i>, <i>Nouveau dossier</i>, <i>Nouveau fichier texte</i>, <i>Importer des fichiers</i>.</li>
<li><b>Double-clic</b> sur un programme <code>.blocs</code> : il s'ouvre dans l'éditeur. Le programme ouvert est <b style="color:#4fc3f7">en gras bleu</b> dans la liste.</li>
<li><code>F2</code> : renommer. <code>Suppr</code> : supprimer (le fichier va à la <b>corbeille</b>, rien n'est perdu pour de bon).</li>
<li><code>Ctrl+C</code>, <code>Ctrl+X</code>, <code>Ctrl+V</code> : copier, couper, coller des fichiers et des dossiers. <i>Dupliquer</i> fait une copie à côté.</li>
<li><b>Glisser-déposer</b> : déplace un fichier dans un dossier, ou dépose-y des fichiers venant de l'Explorateur Windows.</li>
</ul>
<p>💡 Si tu renommes le programme ouvert, Blocs le suit : pas besoin de le rouvrir.</p>
)HTML"),
        S("Images et sons du projet", R"HTML(
<h2>Du dossier au jeu</h2>
<p>Dépose tes images (PNG, JPG…) et sons (WAV) dans le dossier du projet, par exemple dans un sous-dossier <code>images</code>.</p>
<ul>
<li><b>Double-clic sur une image</b> : un aperçu s'affiche, avec un bouton <i>Importer comme ressource du projet</i>.</li>
<li><b>Clic droit &gt; Importer comme ressource du projet</b> (images et WAV) : le fichier est ajouté au programme ouvert, avec le nom que tu choisis. Tu peux ensuite l'utiliser dans « Dessiner une image » ou « Jouer un son ».</li>
<li>Les autres fichiers (texte, etc.) s'ouvrent avec le programme habituel de Windows.</li>
</ul>
<p>Les ressources importées sont enregistrées <i>dans</i> le fichier <code>.blocs</code> : tu peux garder le dossier d'origine pour tes sources, sans que le jeu en dépende.</p>
)HTML")}};

    return t;
}
