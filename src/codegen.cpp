#include "codegen.h"

#include <QSet>
#include <algorithm>

#include "editor.h"

namespace {

struct Expr {                 // une expression C# et son type
    QString code;
    DataType t = DataType::Number;
};

struct Param { QString name; DataType type; };

struct FuncInfo {             // une fonction définie par un bloc « ▶ Fonction »
    QString name;
    QVector<Param> params;
    bool hasRet = false;
    DataType ret = DataType::Number;
    NodeItem* node = nullptr;
};

const QSet<QString>& csKeywords() {
    static const QSet<QString> k = {
        "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked", "class", "const",
        "continue", "decimal", "default", "delegate", "do", "double", "else", "enum", "event", "explicit",
        "extern", "false", "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit", "in", "int",
        "interface", "internal", "is", "lock", "long", "namespace", "new", "null", "object", "operator", "out",
        "override", "params", "private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed",
        "short", "sizeof", "stackalloc", "static", "string", "struct", "switch", "this", "throw", "true", "try",
        "typeof", "uint", "ulong", "unchecked", "unsafe", "ushort", "using", "virtual", "void", "volatile", "while"};
    return k;
}

QString csName(const QString& raw) {
    QString s;
    for (QChar c : raw.trimmed()) s += (c.isLetterOrNumber() || c == '_') ? c : QChar('_');
    if (s.isEmpty()) s = "x";
    if (s[0].isDigit()) s.prepend('_');
    if (csKeywords().contains(s)) s.prepend('@');
    return s;
}

QString csType(DataType t) {
    switch (t) {
    case DataType::Text: return "string";
    case DataType::Bool: return "bool";
    case DataType::NumList: return "List<double>";
    case DataType::TextList: return "List<string>";
    default: return "double";
    }
}

QString initValue(DataType t) {
    switch (t) {
    case DataType::Text: return "\"\"";
    case DataType::Bool: return "false";
    case DataType::NumList: return "new List<double>()";
    case DataType::TextList: return "new List<string>()";
    default: return "0.0";
    }
}

QString stringLit(const QString& raw) {
    QString s = "\"";
    for (QChar c : raw) {
        if (c == '\\') s += "\\\\";
        else if (c == '"') s += "\\\"";
        else if (c == '\n') s += "\\n";
        else if (c == '\r') s += "\\r";
        else if (c == '\t') s += "\\t";
        else s += c;
    }
    return s + "\"";
}

QString numLit(double v) {
    QString s = QString::number(v, 'g', 15);
    if (!s.contains('.') && !s.contains('e') && !s.contains('E')) s += ".0";   // reste un double
    return v < 0 ? "(" + s + ")" : s;
}

bool parseNumber(QString s, double* out) {
    bool ok = false;
    *out = s.trimmed().replace(',', '.').toDouble(&ok);
    return ok;
}

// Texte « affichable » : les listes sont affichées sous la forme [1, 2, 3]
QString listAsText(const QString& code) { return "(\"[\" + string.Join(\", \", " + code + ") + \"]\")"; }

Expr asTextExpr(const Expr& e) {
    if (isList(e.t)) return {listAsText(e.code), DataType::Text};
    return e.t == DataType::Text ? e : Expr{"Convert.ToString(" + e.code + ")", DataType::Text};
}

// Petites méthodes ajoutées au programme seulement si un bloc en a besoin
QStringList L(const char* text) { return QString::fromUtf8(text).split('\n'); }

const char* kJeuClass = R"CS(static class Jeu
{
    static readonly System.Text.StringBuilder img = new System.Text.StringBuilder();
    static readonly object verrou = new object();
    static readonly HashSet<string> touches = new HashSet<string>();
    static readonly HashSet<string> frappes = new HashSet<string>();
    static readonly System.Diagnostics.Stopwatch chrono = System.Diagnostics.Stopwatch.StartNew();
    static double sx, sy, dernier;
    static bool clic, ouvert, enCours;
    public static double Dt = 1.0 / 60;
    public static double Temps { get { return chrono.Elapsed.TotalSeconds; } }
    public static double SourisX { get { lock (verrou) return sx; } }
    public static double SourisY { get { lock (verrou) return sy; } }
    public static bool Clic { get { lock (verrou) return clic; } }
    static string N(double v) { return v.ToString("0.##", CultureInfo.InvariantCulture); }
    static string T(string s) { return (s ?? "").Replace('\n', ' ').Replace('\r', ' '); }
    static string C(string s) { return T(s).Replace(" ", ""); }
    static void Envoyer(string lignes)
    {
        Console.Out.Write("\u0001" + lignes + "\u0002");
        Console.Out.Flush();
    }
    public static void Ouvrir(double largeur, double hauteur, string titre)
    {
        if (!ouvert)
        {
            ouvert = true;
            Envoyer("size " + N(largeur) + " " + N(hauteur) + "\ntitle " + T(titre) + "\n");
            var lecteur = new Thread(Lire) { IsBackground = true };
            lecteur.Start();
        }
        Debut("noir");
        dernier = chrono.Elapsed.TotalSeconds;
    }
    static void Lire()
    {
        string ligne;
        while ((ligne = Console.ReadLine()) != null)
        {
            string[] p = ligne.Split(' ');
            lock (verrou)
            {
                if (p[0] == "kd" && p.Length > 1) { if (touches.Add(p[1])) frappes.Add(p[1]); }
                else if (p[0] == "ku" && p.Length > 1) touches.Remove(p[1]);
                else if (p.Length > 2 && (p[0] == "mm" || p[0] == "md" || p[0] == "mu"))
                {
                    double.TryParse(p[1], NumberStyles.Float, CultureInfo.InvariantCulture, out sx);
                    double.TryParse(p[2], NumberStyles.Float, CultureInfo.InvariantCulture, out sy);
                    if (p[0] == "md") clic = true;
                    if (p[0] == "mu") clic = false;
                }
            }
        }
    }
    static void Debut(string fond) { img.Clear(); img.Append("clear " + C(fond) + "\n"); }
    public static bool Boucle(string fond)
    {
        if (enCours) Afficher();
        enCours = true;
        Debut(fond);
        return true;
    }
    public static void Afficher()
    {
        img.Append("flip\n");
        Envoyer(img.ToString());
        lock (verrou) frappes.Clear();
        double reste = 1.0 / 60 - (chrono.Elapsed.TotalSeconds - dernier);
        if (reste > 0) Thread.Sleep((int)(reste * 1000));
        double fin = chrono.Elapsed.TotalSeconds;
        Dt = Math.Min(fin - dernier, 0.1);
        dernier = fin;
        Debut("noir");
    }
    public static void Rect(double x, double y, double l, double h, string c, bool plein)
    { img.Append("rect " + N(x) + " " + N(y) + " " + N(l) + " " + N(h) + " " + C(c) + " " + (plein ? 1 : 0) + "\n"); }
    public static void Cercle(double x, double y, double r, string c, bool plein)
    { img.Append("circle " + N(x) + " " + N(y) + " " + N(r) + " " + C(c) + " " + (plein ? 1 : 0) + "\n"); }
    public static void Ligne(double x1, double y1, double x2, double y2, string c, double ep)
    { img.Append("line " + N(x1) + " " + N(y1) + " " + N(x2) + " " + N(y2) + " " + C(c) + " " + N(ep) + "\n"); }
    public static void Texte(double x, double y, double taille, string c, string texte)
    { img.Append("text " + N(x) + " " + N(y) + " " + N(taille) + " " + C(c) + " " + T(texte) + "\n"); }
    public static void Sprite(string nom, double x, double y, double l, double h, double angle)
    { img.Append("sprite " + N(x) + " " + N(y) + " " + N(l) + " " + N(h) + " " + N(angle) + " " + C(nom) + "\n"); }
    public static void Son(string nom) { img.Append("sound " + C(nom) + "\n"); }
    public static void Bip() { img.Append("beep\n"); }
    public static bool Touche(string nom) { lock (verrou) return touches.Contains(nom); }
    public static bool Frappe(string nom) { lock (verrou) return frappes.Contains(nom); }
    public static bool Touchent(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2)
    { return x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2; }
})CS";


// Version « jeu autonome » : une vraie fenêtre Windows (WinForms), sans Blocs.
// Le programme du joueur tourne dans le fil principal, la fenêtre dans un fil séparé.
const char* kJeuStandalone = R"CS(static class Jeu
{
    static readonly object verrou = new object();
    static readonly HashSet<string> touches = new HashSet<string>();
    static readonly HashSet<string> frappes = new HashSet<string>();
    static readonly Dictionary<string, System.Drawing.Image> images = new Dictionary<string, System.Drawing.Image>();
    static readonly System.Diagnostics.Stopwatch chrono = System.Diagnostics.Stopwatch.StartNew();
    static Bitmap arriere, devant;
    static Graphics g;
    static JeuFenetre fenetre;
    static Thread fil;
    static System.Media.SoundPlayer dernierSon;
    static double sx, sy, dernier;
    static bool clic, ouvert, enCours;
    public static double Dt = 1.0 / 60;
    public static double Temps { get { return chrono.Elapsed.TotalSeconds; } }
    public static double SourisX { get { lock (verrou) return sx; } }
    public static double SourisY { get { lock (verrou) return sy; } }
    public static bool Clic { get { lock (verrou) return clic; } }

    public static void Ouvrir(double largeur, double hauteur, string titre)
    {
        if (!ouvert)
        {
            ouvert = true;
            int w = (int)largeur, h = (int)hauteur;
            arriere = new Bitmap(w, h);
            devant = new Bitmap(w, h);
            using (Graphics gg = Graphics.FromImage(devant)) gg.Clear(Col("noir"));
            ManualResetEvent pret = new ManualResetEvent(false);
            fil = new Thread(delegate ()
            {
                Application.EnableVisualStyles();
                fenetre = new JeuFenetre(w, h, titre);
                fenetre.Shown += delegate (object s, EventArgs e) { pret.Set(); };
                Application.Run(fenetre);
                Environment.Exit(0);
            });
            fil.SetApartmentState(ApartmentState.STA);
            fil.Start();
            pret.WaitOne();
        }
        Debut("noir");
        dernier = chrono.Elapsed.TotalSeconds;
    }
    public static void AttendreFermeture() { if (fil != null) fil.Join(); }

    static Color Col(string nom)
    {
        switch ((nom ?? "").Trim().ToLowerInvariant())
        {
            case "noir": return Color.FromArgb(16, 16, 24);
            case "blanc": return Color.FromArgb(245, 245, 245);
            case "rouge": return Color.FromArgb(229, 57, 53);
            case "vert": return Color.FromArgb(67, 160, 71);
            case "bleu": return Color.FromArgb(30, 136, 229);
            case "jaune": return Color.FromArgb(253, 216, 53);
            case "orange": return Color.FromArgb(251, 140, 0);
            case "violet": return Color.FromArgb(142, 36, 170);
            case "rose": return Color.FromArgb(236, 64, 122);
            case "marron": return Color.FromArgb(109, 76, 65);
            case "gris": return Color.FromArgb(158, 158, 158);
            case "cyan": return Color.FromArgb(38, 198, 218);
        }
        try { return ColorTranslator.FromHtml(nom.Trim()); } catch (Exception) { return Color.FromArgb(245, 245, 245); }
    }
    static void Debut(string nomFond)
    {
        if (g != null) g.Dispose();
        g = Graphics.FromImage(arriere);
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;
        g.Clear(Col(nomFond));
    }
    public static bool Boucle(string fond)
    {
        if (enCours) Afficher();
        enCours = true;
        Debut(fond);
        return true;
    }
    public static void Afficher()
    {
        if (g != null) { g.Dispose(); g = null; }
        lock (verrou) { Bitmap t = devant; devant = arriere; arriere = t; frappes.Clear(); }
        Rafraichir();
        double reste = 1.0 / 60 - (chrono.Elapsed.TotalSeconds - dernier);
        if (reste > 0) Thread.Sleep((int)(reste * 1000));
        double fin = chrono.Elapsed.TotalSeconds;
        Dt = Math.Min(fin - dernier, 0.1);
        dernier = fin;
        Debut("noir");
    }
    static void Rafraichir()
    {
        JeuFenetre f = fenetre;
        if (f != null && f.IsHandleCreated && !f.IsDisposed)
        {
            try { f.BeginInvoke(new MethodInvoker(f.Invalidate)); } catch (Exception) { }
        }
    }
    public static void Peindre(Graphics gr) { lock (verrou) { gr.DrawImageUnscaled(devant, 0, 0); } }

    public static void Rect(double x, double y, double l, double h, string c, bool plein)
    {
        if (g == null) return;
        if (plein) { using (SolidBrush b = new SolidBrush(Col(c))) g.FillRectangle(b, (float)x, (float)y, (float)l, (float)h); }
        else { using (Pen p = new Pen(Col(c), 2)) g.DrawRectangle(p, (float)x, (float)y, (float)l, (float)h); }
    }
    public static void Cercle(double x, double y, double r, string c, bool plein)
    {
        if (g == null) return;
        if (plein) { using (SolidBrush b = new SolidBrush(Col(c))) g.FillEllipse(b, (float)(x - r), (float)(y - r), (float)(2 * r), (float)(2 * r)); }
        else { using (Pen p = new Pen(Col(c), 2)) g.DrawEllipse(p, (float)(x - r), (float)(y - r), (float)(2 * r), (float)(2 * r)); }
    }
    public static void Ligne(double x1, double y1, double x2, double y2, string c, double ep)
    {
        if (g == null) return;
        using (Pen p = new Pen(Col(c), (float)Math.Max(1, ep)))
        {
            p.StartCap = LineCap.Round;
            p.EndCap = LineCap.Round;
            g.DrawLine(p, (float)x1, (float)y1, (float)x2, (float)y2);
        }
    }
    static bool AvecEmoji(string s)
    {
        foreach (char ch in s) if (char.IsSurrogate(ch) || ch >= (char)0x2600) return true;
        return false;
    }
    public static void Texte(double x, double y, double taille, string c, string texte)
    {
        if (g == null) return;
        string famille = AvecEmoji(texte) ? "Segoe UI Emoji" : "Segoe UI";
        using (Font f = new Font(famille, (float)Math.Max(6, taille), FontStyle.Regular, GraphicsUnit.Pixel))
        using (SolidBrush b = new SolidBrush(Col(c)))
            g.DrawString(texte, f, b, (float)x, (float)y);
    }
    static byte[] Ressource(string nom, bool son)
    {
        Assembly asm = Assembly.GetExecutingAssembly();
        foreach (string rn in asm.GetManifestResourceNames())
        {
            bool estSon = Path.GetExtension(rn).ToLowerInvariant() == ".wav";
            if (estSon == son && Path.GetFileNameWithoutExtension(rn).Equals(nom, StringComparison.OrdinalIgnoreCase))
            {
                using (Stream s = asm.GetManifestResourceStream(rn))
                using (MemoryStream ms = new MemoryStream())
                {
                    s.CopyTo(ms);
                    return ms.ToArray();
                }
            }
        }
        return null;
    }
    static System.Drawing.Image Charger(string nom)
    {
        lock (images)
        {
            System.Drawing.Image im;
            if (images.TryGetValue(nom, out im)) return im;
            byte[] data = Ressource(nom, false);
            im = null;
            if (data != null)
            {
                try { im = System.Drawing.Image.FromStream(new MemoryStream(data)); } catch (Exception) { }
            }
            images[nom] = im;
            return im;
        }
    }
    public static void Sprite(string nom, double x, double y, double l, double h, double angle)
    {
        System.Drawing.Image im = Charger(nom);
        if (im == null || g == null) return;
        GraphicsState etat = g.Save();
        g.TranslateTransform((float)(x + l / 2), (float)(y + h / 2));
        g.RotateTransform((float)angle);
        g.DrawImage(im, (float)(-l / 2), (float)(-h / 2), (float)l, (float)h);
        g.Restore(etat);
    }
    public static void Son(string nom)
    {
        try
        {
            byte[] data = Ressource(nom, true);
            if (data == null) return;
            dernierSon = new System.Media.SoundPlayer(new MemoryStream(data));
            dernierSon.Play();
        }
        catch (Exception) { }
    }
    public static void Bip()
    {
        try { System.Media.SystemSounds.Beep.Play(); } catch (Exception) { }
    }
    public static bool Touche(string nom) { lock (verrou) return touches.Contains(nom); }
    public static bool Frappe(string nom) { lock (verrou) return frappes.Contains(nom); }
    public static bool Touchent(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2)
    { return x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2; }

    static string NomTouche(Keys k)
    {
        if (k == Keys.Left) return "Gauche";
        if (k == Keys.Right) return "Droite";
        if (k == Keys.Up) return "Haut";
        if (k == Keys.Down) return "Bas";
        if (k == Keys.Space) return "Espace";
        if (k == Keys.Enter) return "Entree";
        if (k == Keys.Escape) return "Echap";
        if (k >= Keys.A && k <= Keys.Z) return ((char)('A' + (k - Keys.A))).ToString();
        if (k >= Keys.D0 && k <= Keys.D9) return ((char)('0' + (k - Keys.D0))).ToString();
        if (k >= Keys.NumPad0 && k <= Keys.NumPad9) return ((char)('0' + (k - Keys.NumPad0))).ToString();
        return null;
    }
    public static void EvTouche(bool bas, Keys k)
    {
        string nom = NomTouche(k);
        if (nom == null) return;
        lock (verrou)
        {
            if (bas) { if (touches.Add(nom)) frappes.Add(nom); }
            else touches.Remove(nom);
        }
    }
    public static void EvSouris(int x, int y, int etat)
    {
        lock (verrou)
        {
            sx = x; sy = y;
            if (etat == 1) clic = true;
            if (etat == 2) clic = false;
        }
    }
    public static void EvPerteFocus() { lock (verrou) { touches.Clear(); clic = false; } }
}

class JeuFenetre : Form
{
    public JeuFenetre(int w, int h, string titre)
    {
        Text = titre;
        ClientSize = new Size(w, h);
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        DoubleBuffered = true;
        KeyPreview = true;
        BackColor = Color.Black;
    }
    protected override void OnPaint(PaintEventArgs e) { Jeu.Peindre(e.Graphics); }
    protected override void OnKeyDown(KeyEventArgs e) { Jeu.EvTouche(true, e.KeyCode); e.Handled = true; }
    protected override void OnKeyUp(KeyEventArgs e) { Jeu.EvTouche(false, e.KeyCode); e.Handled = true; }
    protected override void OnMouseMove(MouseEventArgs e) { Jeu.EvSouris(e.X, e.Y, 0); }
    protected override void OnMouseDown(MouseEventArgs e) { if (e.Button == MouseButtons.Left) Jeu.EvSouris(e.X, e.Y, 1); }
    protected override void OnMouseUp(MouseEventArgs e) { if (e.Button == MouseButtons.Left) Jeu.EvSouris(e.X, e.Y, 2); }
    protected override void OnDeactivate(EventArgs e) { base.OnDeactivate(e); Jeu.EvPerteFocus(); }
})CS";

const QMap<QString, QStringList>& helperCode() {
    static const QMap<QString, QStringList> h = {
        {"AttendreTouche", {"static void AttendreTouche()", "{",
                            "    if (Console.IsInputRedirected) Console.ReadLine(); else Console.ReadKey(true);", "}"}},
        {"Caractere", {"static string Caractere(string texte, double position)", "{",
                       "    int i = (int)position;", "    return (i >= 0 && i < texte.Length) ? texte[i].ToString() : \"\";", "}"}},
        {"Demander", {"static string Demander(string message)", "{", "    Console.Write(message);",
                      "    return Console.ReadLine() ?? \"\";", "}"}},
        {"Effacer", {"static void Effacer()", "{",
                     "    if (Console.IsOutputRedirected) Console.Write('\\f'); else Console.Clear();", "}"}},
        {"EstNombre", {"static bool EstNombre(string texte)", "{",
                       "    return double.TryParse(texte.Trim().Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out _);", "}"}},
        {"Extraire", {"static string Extraire(string texte, double debut, double longueur)", "{",
                      "    int d = (int)Math.Max(0, Math.Min(debut, texte.Length));",
                      "    int l = (int)Math.Max(0, Math.Min(longueur, texte.Length - d));",
                      "    return texte.Substring(d, l);", "}"}},
        {"Jeu", L(kJeuClass)},
        {"Nombre", {"static double Nombre(string texte, double siInvalide)", "{",
                    "    return double.TryParse(texte.Trim().Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out double v) ? v : siInvalide;", "}"}},
        {"Touche", {"static string Touche()", "{",
                    "    if (Console.IsInputRedirected) return Console.ReadLine() ?? \"\";",
                    "    return Console.ReadKey(true).Key.ToString();", "}"}},
    };
    return h;
}

class Gen {
public:
    Gen(const NodeScene& s, CodeMode mode) : scene_(s), forRun_(mode == CodeMode::Run), standalone_(mode == CodeMode::Standalone) {}

    CodeResult run() {
        QList<NodeItem*> starts, funcDefs;
        for (NodeItem* n : scene_.nodeItems()) {
            if (n->def().id == "start") starts.append(n);
            else if (n->def().id == "funcdef") funcDefs.append(n);
        }
        if (starts.isEmpty())
            return {"// Ajoute un bloc « Début », puis relie-le à d'autres blocs.\n", {}};

        // 0) variables globales (portée « globale »)
        for (NodeItem* n : scene_.nodeItems())
            if (n->def().id == "setvar" && n->param("scope") == "globale") {
                const QString name = csName(n->param("name"));
                const DataType t = typeFromName(n->param("type"));
                if (globals_.contains(name) && globals_[name] != t)
                    problem(n, QString("la variable globale « %1 » est déclarée avec deux types différents").arg(name));
                else
                    globals_[name] = t;
            }

        // 1) signatures des fonctions
        for (NodeItem* f : funcDefs) {
            FuncInfo fi;
            fi.node = f;
            fi.name = csName(f->param("name"));
            fi.params = parseParams(f);
            const QString r = f->param("ret").trimmed().toLower();
            fi.hasRet = !(r.isEmpty() || r == "aucun");
            if (fi.hasRet) fi.ret = typeFromName(r);
            if (funcs_.contains(fi.name)) problem(f, QString("la fonction « %1 » est définie deux fois").arg(fi.name));
            else funcs_[fi.name] = fi;
        }

        // 2) corps des fonctions
        QStringList funcLines;
        for (const FuncInfo& fi : std::as_const(funcs_)) funcLines << genFunction(fi) << "";

        // 3) Main
        vars_.clear();
        params_.clear();
        loopVars_.clear();
        loopDepth_ = 0;
        curFunc_ = nullptr;
        for (NodeItem* s : starts) collectVars(s, vars_);
        QStringList body;
        for (int i = 0; i < starts.size(); ++i) {
            if (starts.size() > 1) body << QString(8, ' ') + QString("// --- Début n°%1 ---").arg(i + 1);
            chain(flowTarget(starts[i], "next"), 2, body, {starts[i]});
        }

        QStringList cls;
        cls << "class Program" << "{";
        for (auto it = helperCode().begin(); it != helperCode().end(); ++it)
            if (helpers_.contains(it.key())) {
                const QStringList lines = (standalone_ && it.key() == "Jeu") ? L(kJeuStandalone) : it.value();
                for (const QString& l : lines) cls << "    " + l;
                cls << "";
            }
        for (NodeItem* n : scene_.nodeItems())               // « Code C# global » : méthodes, classes…
            if (n->def().id == "csclass" && !n->param("code").trimmed().isEmpty()) {
                for (const QString& l : n->param("code").split('\n')) cls << "    " + l.trimmed();
                cls << "";
            }
        for (auto it = globals_.begin(); it != globals_.end(); ++it)
            cls << QString("    static %1 %2 = %3;").arg(csType(it.value()), it.key(), initValue(it.value()));
        if (!globals_.isEmpty()) cls << "";
        cls << funcLines;
        // Jeu autonome : le programme du joueur devient « Principal », et Main attend la fermeture de la fenêtre
        const bool graphical = helpers_.contains("Jeu");
        const bool wrap = standalone_ && graphical;
        cls << (wrap ? "    static void Principal()" : "    static void Main()") << "    {";
        if (forRun_ || (standalone_ && !graphical))
            cls << "        Console.OutputEncoding = System.Text.Encoding.UTF8;"
                << "        Console.InputEncoding = System.Text.Encoding.UTF8;" << "";
        cls << declarations();
        if (!vars_.isEmpty()) cls << "";
        cls << body << "    }";
        if (wrap)
            cls << "" << "    static void Main()" << "    {" << "        Principal();"
                << "        Jeu.AttendreFermeture();" << "    }";
        cls << "}";

        QStringList out;
        out << "// Code généré par Blocs";
        if (helpers_.contains("Jeu") && !standalone_)
            out << "// Jeu graphique : ce code dialogue avec la fenêtre de jeu de Blocs (il ne s'ouvre pas seul).";
        if (cls.join("\n").contains("List<")) usings_.insert("System.Collections.Generic");
        if (helpers_.contains("Nombre") || helpers_.contains("EstNombre")) usings_.insert("System.Globalization");
        if (helpers_.contains("Jeu")) {
            usings_.insert("System.Globalization");
            usings_.insert("System.Threading");
            usings_.insert("System.Collections.Generic");
            if (standalone_)
                for (const char* u : {"System.Windows.Forms", "System.Drawing", "System.Drawing.Drawing2D", "System.Reflection", "System.IO"})
                    usings_.insert(u);
        }
        QStringList us = usings_.values();
        std::sort(us.begin(), us.end());
        us.removeAll("System");
        out << "using System;";
        for (const QString& u : us) out << "using " + u + ";";
        out << "";
        out << cls;
        return {out.join("\n") + "\n", problems_};
    }

private:
    const NodeScene& scene_;
    bool forRun_;
    bool standalone_;
    QStringList problems_;
    QMap<QString, FuncInfo> funcs_;
    QMap<QString, DataType> vars_;                 // variables locales (Main ou fonction en cours)
    QMap<QString, DataType> params_;               // paramètres de la fonction en cours
    QMap<QString, QVector<DataType>> loopVars_;    // compteurs / éléments de boucle actifs
    QMap<QString, DataType> globals_;              // variables globales (champs de la classe)
    int loopDepth_ = 0;
    const FuncInfo* curFunc_ = nullptr;
    QSet<QString> usings_{"System"};
    QSet<QString> helpers_;

    void problem(NodeItem* n, const QString& text) {
        const QString msg = QString("« %1 » : %2").arg(n->def().title, text);
        if (!problems_.contains(msg)) problems_ << msg;
    }

    NodeItem* flowTarget(NodeItem* n, const QString& outId) const {
        PortItem* p = n->port(PortDir::Out, outId);
        return (p && p->isConnected()) ? p->connections().first()->to()->node() : nullptr;
    }

    QVector<Param> parseParams(NodeItem* n) {
        QVector<Param> res;
        for (QString part : n->param("params").split(',')) {
            part = part.trimmed();
            if (part.isEmpty()) continue;
            QString nm = part, ty = "nombre";
            const int c = part.indexOf(':');
            if (c >= 0) { nm = part.left(c); ty = part.mid(c + 1); }
            res.push_back({csName(nm), typeFromName(ty)});
        }
        if (res.size() > 6) {
            problem(n, "6 paramètres maximum");
            res.resize(6);
        }
        return res;
    }

    QStringList declarations() const {
        QStringList l;
        for (auto it = vars_.begin(); it != vars_.end(); ++it)
            l << QString("        %1 %2 = %3;").arg(csType(it.value()), it.key(), initValue(it.value()));
        return l;
    }

    QStringList genFunction(const FuncInfo& f) {
        vars_.clear();
        params_.clear();
        loopVars_.clear();
        loopDepth_ = 0;
        collectVars(f.node, vars_);
        QStringList sig;
        for (const Param& p : f.params) {
            params_[p.name] = p.type;
            vars_.remove(p.name);                 // déjà déclaré comme paramètre
            sig << csType(p.type) + " " + p.name;
        }
        curFunc_ = &f;
        QStringList body;
        const bool ends = chain(flowTarget(f.node, "next"), 2, body, {f.node});
        QStringList out;
        out << QString("    static %1 %2(%3)").arg(f.hasRet ? csType(f.ret) : "void", f.name, sig.join(", "))
            << "    {";
        out << declarations();
        if (!vars_.isEmpty()) out << "";
        out << body;
        if (f.hasRet && !ends) out << "        return default;";
        out << "    }";
        curFunc_ = nullptr;
        return out;
    }

    // Repère toutes les variables (« Définir variable ») atteignables depuis un bloc de départ
    void collectVars(NodeItem* start, QMap<QString, DataType>& into) {
        QList<NodeItem*> todo{start};
        QSet<NodeItem*> seen{start};
        while (!todo.isEmpty()) {
            NodeItem* n = todo.takeFirst();
            if (n->def().id == "setvar") {
                const QString name = csName(n->param("name"));
                const DataType t = typeFromName(n->param("type"));
                if (globals_.contains(name)) {
                    // variable globale : déjà déclarée au niveau de la classe
                } else if (into.contains(name) && into[name] != t) {
                    problem(n, QString("la variable « %1 » est déclarée avec deux types différents").arg(name));
                } else {
                    into[name] = t;
                }
            }
            for (PortItem* p : n->ports())
                if (p->kind() == PortKind::Flow && p->dir() == PortDir::Out)
                    for (ConnectionItem* c : p->connections()) {
                        NodeItem* t = c->to()->node();
                        if (!seen.contains(t)) { seen.insert(t); todo.append(t); }
                    }
        }
    }

    bool lookupVar(const QString& name, DataType* t) const {
        if (!loopVars_.value(name).isEmpty()) { *t = loopVars_[name].last(); return true; }
        if (params_.contains(name)) { *t = params_[name]; return true; }
        if (vars_.contains(name)) { *t = vars_[name]; return true; }
        if (globals_.contains(name)) { *t = globals_[name]; return true; }
        return false;
    }

    // La liste désignée par le champ « liste » d'un bloc
    bool listVar(NodeItem* n, QString* name, DataType* type) {
        *name = csName(n->param("liste"));
        if (!lookupVar(*name, type)) {
            problem(n, QString("la liste « %1 » n'existe pas (crée-la avec « Définir variable » de type liste)").arg(*name));
            return false;
        }
        if (!isList(*type)) {
            problem(n, QString("« %1 » n'est pas une liste").arg(*name));
            return false;
        }
        return true;
    }

    // ----------------------------------------------------------- expressions
    Expr literal(NodeItem* n, const QString& label, DataType t, const QString& raw) {
        switch (t) {
        case DataType::Number: {
            double v;
            if (parseNumber(raw, &v)) return {numLit(v), DataType::Number};
            problem(n, QString("« %1 » doit être un nombre (reçu « %2 »)").arg(label, raw));
            return {"0.0", DataType::Number};
        }
        case DataType::Bool: {
            const QString l = raw.trimmed().toLower();
            if (l == "true" || l == "vrai") return {"true", DataType::Bool};
            if (l == "false" || l == "faux" || l.isEmpty()) return {"false", DataType::Bool};
            problem(n, QString("« %1 » doit être true ou false (reçu « %2 »)").arg(label, raw));
            return {"false", DataType::Bool};
        }
        case DataType::NumList:
        case DataType::TextList: {          // « 1, 2, 3 »  →  new List<double> { 1.0, 2.0, 3.0 }
            const DataType et = elemType(t);
            QStringList parts;
            for (QString p : raw.split(',')) {
                p = p.trimmed();
                if (!p.isEmpty()) parts << literal(n, label, et, p).code;
            }
            const QString init = parts.isEmpty() ? "()" : " { " + parts.join(", ") + " }";
            return {QString("new List<%1>%2").arg(csType(et), init), t};
        }
        default:
            return {stringLit(raw), DataType::Text};
        }
    }

    // Valeur d'une entrée ● : le fil relié, sinon la valeur par défaut saisie dans le bloc.
    Expr input(NodeItem* n, const QString& id, const QList<NodeItem*>& stack, DataType literalAs = DataType::Any) {
        const DataIn* di = n->def().findInput(id);
        PortItem* p = n->port(PortDir::In, id);
        if (p && p->isConnected()) {
            Expr e = outExpr(p->connections().first()->from()->node(), stack + QList<NodeItem*>{n});
            if (di->type != DataType::Any && e.t != di->type)
                problem(n, QString("l'entrée « %1 » attend un %2 mais reçoit un %3")
                               .arg(di->label, typeName(di->type), typeName(e.t)));
            return e;
        }
        const QString raw = n->param(id);
        DataType lt = literalAs != DataType::Any ? literalAs : di->type;
        if (lt == DataType::Any) {           // on devine : nombre, true/false, sinon texte
            double v;
            lt = parseNumber(raw, &v) ? DataType::Number
                 : (raw == "true" || raw == "false") ? DataType::Bool : DataType::Text;
        }
        return literal(n, di->label, lt, raw);
    }

    // Vérifie le type d'une valeur attendue
    void expect(NodeItem* n, const QString& what, const Expr& e, DataType want) {
        if (e.t != want)
            problem(n, QString("%1 attend un %2 mais reçoit un %3").arg(what, typeName(want), typeName(e.t)));
    }

    QString opCode(const QString& op) {
        if (op == "et") return "&&";
        if (op == "ou") return "||";
        return op;
    }

    Expr callExpr(NodeItem* n, const QList<NodeItem*>& stack, bool wantValue) {
        const QString name = csName(n->param("name"));
        auto it = funcs_.constFind(name);
        if (it == funcs_.constEnd()) {
            problem(n, QString("la fonction « %1 » n'existe pas (ajoute un bloc « ▶ Fonction »)").arg(name));
            return {name + "()", DataType::Number};
        }
        const FuncInfo& f = it.value();
        QStringList args;
        for (int i = 0; i < f.params.size(); ++i) {
            const Expr e = input(n, QString("a%1").arg(i + 1), stack, f.params[i].type);
            expect(n, QString("l'argument %1 de « %2 »").arg(i + 1).arg(f.name), e, f.params[i].type);
            args << e.code;
        }
        for (int i = f.params.size(); i < 6; ++i) {
            PortItem* p = n->port(PortDir::In, QString("a%1").arg(i + 1));
            if (p && p->isConnected())
                problem(n, QString("trop d'arguments : « %1 » n'a que %2 paramètre(s)").arg(f.name).arg(f.params.size()));
        }
        if (wantValue && !f.hasRet) problem(n, QString("la fonction « %1 » ne retourne rien").arg(f.name));
        return {QString("%1(%2)").arg(f.name, args.join(", ")), f.hasRet ? f.ret : DataType::Number};
    }

    Expr outExpr(NodeItem* n, const QList<NodeItem*>& stack) {
        if (stack.contains(n)) {
            problem(n, "boucle infinie entre des blocs de données");
            return {"0.0", DataType::Number};
        }
        const QString id = n->def().id;
        auto in = [&](const QString& pid, DataType as = DataType::Any) { return input(n, pid, stack, as); };

        if (id == "number") return literal(n, "valeur", DataType::Number, n->param("value"));
        if (id == "text") return {stringLit(n->param("value")), DataType::Text};
        if (id == "bool") return {n->param("value") == "false" ? "false" : "true", DataType::Bool};
        if (id == "getvar") {
            const QString name = csName(n->param("name"));
            DataType t;
            if (lookupVar(name, &t)) return {name, t};
            problem(n, QString("la variable « %1 » n'est jamais définie (utilise « Définir variable »)").arg(name));
            return {"0.0", DataType::Number};
        }
        if (id == "random")
            return {QString("(double)Random.Shared.Next((int)(%1), (int)(%2) + 1)").arg(in("a").code, in("b").code),
                    DataType::Number};
        if (id == "math") {
            const QString a = in("a").code, b = in("b").code, op = n->param("op");
            if (op == "//") return {QString("Math.Floor(%1 / %2)").arg(a, b), DataType::Number};
            if (op == "puissance") return {QString("Math.Pow(%1, %2)").arg(a, b), DataType::Number};
            return {QString("(%1 %2 %3)").arg(a, op, b), DataType::Number};
        }
        if (id == "compare") {
            const Expr a = in("a"), b = in("b");
            const QString op = n->param("op");
            if (a.t != b.t) problem(n, QString("on compare un %1 avec un %2").arg(typeName(a.t), typeName(b.t)));
            else if (isList(a.t)) problem(n, "on ne peut pas comparer deux listes directement");
            else if (op != "==" && op != "!=" && a.t != DataType::Number)
                problem(n, QString("« %1 » ne s'utilise qu'avec des nombres").arg(op));
            return {QString("(%1 %2 %3)").arg(a.code, op, b.code), DataType::Bool};
        }
        if (id == "logic")
            return {QString("(%1 %2 %3)").arg(in("a").code, opCode(n->param("op")), in("b").code), DataType::Bool};
        if (id == "not") return {QString("(!%1)").arg(in("a").code), DataType::Bool};
        if (id == "input") {
            helpers_.insert("Demander");
            return {QString("Demander(%1)").arg(in("prompt").code), DataType::Text};
        }
        if (id == "tonumber") {
            helpers_.insert("Nombre");
            return {QString("Nombre(%1, %2)").arg(in("x").code, in("def").code), DataType::Number};
        }
        if (id == "isnumber") {
            helpers_.insert("EstNombre");
            return {QString("EstNombre(%1)").arg(in("x").code), DataType::Bool};
        }
        if (id == "randomreal") return {"Random.Shared.NextDouble()", DataType::Number};
        if (id == "mathfn") {
            static const QMap<QString, QString> fns = {{"abs", "Math.Abs"}, {"racine", "Math.Sqrt"},
                {"plancher", "Math.Floor"}, {"plafond", "Math.Ceiling"}, {"sinus", "Math.Sin"}, {"cosinus", "Math.Cos"}};
            const QString f = n->param("fn"), x = in("x").code;
            if (f == "arrondi") return {QString("Math.Round(%1, MidpointRounding.AwayFromZero)").arg(x), DataType::Number};
            return {QString("%1(%2)").arg(fns.value(f, "Math.Abs"), x), DataType::Number};
        }
        if (id == "minmax")
            return {QString("Math.%1(%2, %3)").arg(n->param("op") == "max" ? "Max" : "Min", in("a").code, in("b").code),
                    DataType::Number};
        if (id == "textlen") return {QString("(double)(%1).Length").arg(in("text").code), DataType::Number};
        if (id == "textcase") {
            const QString op = n->param("op"), t = in("text").code;
            return {QString("(%1).%2()").arg(t, op == "majuscules" ? "ToUpper" : op == "minuscules" ? "ToLower" : "Trim"),
                    DataType::Text};
        }
        if (id == "textsub") {
            helpers_.insert("Extraire");
            return {QString("Extraire(%1, %2, %3)").arg(in("text").code, in("start").code, in("len").code), DataType::Text};
        }
        if (id == "textchar") {
            helpers_.insert("Caractere");
            return {QString("Caractere(%1, %2)").arg(in("text").code, in("index").code), DataType::Text};
        }
        if (id == "textreplace")
            return {QString("(%1).Replace(%2, %3)").arg(in("text").code, in("find").code, in("repl").code), DataType::Text};
        if (id == "textcontains") {
            const QString op = n->param("op");
            return {QString("(%1).%2(%3)").arg(in("text").code,
                        op == "commence par" ? "StartsWith" : op == "finit par" ? "EndsWith" : "Contains", in("part").code),
                    DataType::Bool};
        }
        if (id == "textsplit")
            return {QString("new List<string>((%1).Split(new string[] { %2 }, StringSplitOptions.None))")
                        .arg(in("text").code, in("sep").code), DataType::TextList};
        if (id == "readkey") {
            helpers_.insert("Touche");
            return {"Touche()", DataType::Text};
        }
        if (id == "join")
            return {QString("(%1 + %2)").arg(asTextExpr(in("a")).code, asTextExpr(in("b")).code), DataType::Text};
        if (id == "csexpr") {
            const QString c = n->param("code").trimmed();
            return {"(" + (c.isEmpty() ? QString("0") : c) + ")", typeFromName(n->param("type"))};
        }
        if (id == "callv") return callExpr(n, stack, true);

        // --- jeu graphique
        if (id.startsWith("g_")) {
            helpers_.insert("Jeu");
            if (id == "g_key") return {QString("Jeu.Touche(%1)").arg(stringLit(n->param("key"))), DataType::Bool};
            if (id == "g_keyhit") return {QString("Jeu.Frappe(%1)").arg(stringLit(n->param("key"))), DataType::Bool};
            if (id == "g_mousex") return {"Jeu.SourisX", DataType::Number};
            if (id == "g_mousey") return {"Jeu.SourisY", DataType::Number};
            if (id == "g_mousedown") return {"Jeu.Clic", DataType::Bool};
            if (id == "g_time") return {"Jeu.Temps", DataType::Number};
            if (id == "g_dt") return {"Jeu.Dt", DataType::Number};
            if (id == "g_overlap") {
                QStringList a;
                for (const char* k : {"x1", "y1", "w1", "h1", "x2", "y2", "w2", "h2"}) a << in(k).code;
                return {QString("Jeu.Touchent(%1)").arg(a.join(", ")), DataType::Bool};
            }
            if (id == "g_dist") {
                const QString x1 = in("x1").code, y1 = in("y1").code, x2 = in("x2").code, y2 = in("y2").code;
                return {QString("Math.Sqrt(Math.Pow(%3 - %1, 2) + Math.Pow(%4 - %2, 2))").arg(x1, y1, x2, y2), DataType::Number};
            }
        }

        // --- listes
        if (id == "newlist") {
            DataType t = typeFromName(n->param("type"));
            if (!isList(t)) t = DataType::NumList;
            return literal(n, "éléments", t, n->param("items"));
        }
        if (id == "listjoin") {
            QString name;
            DataType lt = DataType::NumList;
            listVar(n, &name, &lt);
            return {QString("string.Join(%1, %2)").arg(in("sep").code, name), DataType::Text};
        }
        if (id == "listindexof") {
            QString name;
            DataType lt = DataType::NumList;
            const bool ok = listVar(n, &name, &lt);
            const Expr v = in("value", ok ? elemType(lt) : DataType::Any);
            if (ok) expect(n, "la liste", v, elemType(lt));
            return {QString("(double)%1.IndexOf(%2)").arg(name, v.code), DataType::Number};
        }
        if (id == "listget" || id == "listcount" || id == "listcontains") {
            QString name;
            DataType lt = DataType::NumList;
            const bool ok = listVar(n, &name, &lt);
            const DataType et = ok ? elemType(lt) : DataType::Number;
            if (id == "listcount") return {QString("(double)%1.Count").arg(name), DataType::Number};
            if (id == "listget") return {QString("%1[(int)(%2)]").arg(name, in("index").code), et};
            const Expr v = in("value", et);
            if (ok) expect(n, "la liste", v, et);
            return {QString("%1.Contains(%2)").arg(name, v.code), DataType::Bool};
        }
        problem(n, "ce bloc n'est pas reconnu par le générateur");
        return {"0.0", DataType::Number};
    }

    // ------------------------------------------------------------ instructions
    // Renvoie true si tous les chemins se terminent par un « Retourner »
    bool block(NodeItem* first, int depth, QStringList& out, const QSet<NodeItem*>& seen) {
        const QString ind(depth * 4, ' ');
        out << ind + "{";
        const int n0 = out.size();
        const bool ends = chain(first, depth + 1, out, seen);
        if (out.size() == n0) out << QString((depth + 1) * 4, ' ') + "// (vide)";
        out << ind + "}";
        return ends;
    }

    bool chain(NodeItem* n, int depth, QStringList& out, QSet<NodeItem*> seen) {
        const QString ind(depth * 4, ' ');
        bool ends = false;
        while (n) {
            if (seen.contains(n)) {
                out << ind + "// (retour en arrière entre blocs ignoré)";
                return false;
            }
            seen.insert(n);
            ends = false;
            const QString id = n->def().id;
            const QList<NodeItem*> none;

            if (id == "print") {
                const Expr e = input(n, "value", none);
                out << ind + QString("Console.%1(%2);").arg(n->param("nl") == "non" ? "Write" : "WriteLine",
                                                              isList(e.t) ? listAsText(e.code) : e.code);
            } else if (id == "g_open") {
                helpers_.insert("Jeu");
                out << ind + QString("Jeu.Ouvrir(%1, %2, %3);").arg(input(n, "w", none).code, input(n, "h", none).code, input(n, "title", none).code);
            } else if (id == "g_show") {
                helpers_.insert("Jeu");
                out << ind + "Jeu.Afficher();";
            } else if (id == "g_beep") {
                helpers_.insert("Jeu");
                out << ind + "Jeu.Bip();";
            } else if (id == "g_image") {
                helpers_.insert("Jeu");
                out << ind + QString("Jeu.Sprite(%1, %2, %3, %4, %5, %6);")
                                 .arg(input(n, "img", none).code, input(n, "x", none).code, input(n, "y", none).code,
                                      input(n, "w", none).code, input(n, "h", none).code, input(n, "angle", none).code);
            } else if (id == "g_sound") {
                helpers_.insert("Jeu");
                out << ind + QString("Jeu.Son(%1);").arg(input(n, "snd", none).code);
            } else if (id == "g_rect" || id == "g_circle") {
                helpers_.insert("Jeu");
                const QString fill = n->param("fill") == "non" ? "false" : "true";
                if (id == "g_rect")
                    out << ind + QString("Jeu.Rect(%1, %2, %3, %4, %5, %6);")
                                     .arg(input(n, "x", none).code, input(n, "y", none).code, input(n, "w", none).code,
                                          input(n, "h", none).code, input(n, "color", none).code, fill);
                else
                    out << ind + QString("Jeu.Cercle(%1, %2, %3, %4, %5);")
                                     .arg(input(n, "x", none).code, input(n, "y", none).code, input(n, "r", none).code,
                                          input(n, "color", none).code, fill);
            } else if (id == "g_line") {
                helpers_.insert("Jeu");
                out << ind + QString("Jeu.Ligne(%1, %2, %3, %4, %5, %6);")
                                 .arg(input(n, "x1", none).code, input(n, "y1", none).code, input(n, "x2", none).code,
                                      input(n, "y2", none).code, input(n, "color", none).code, input(n, "width", none).code);
            } else if (id == "g_text") {
                helpers_.insert("Jeu");
                out << ind + QString("Jeu.Texte(%1, %2, %3, %4, %5);")
                                 .arg(input(n, "x", none).code, input(n, "y", none).code, input(n, "size", none).code,
                                      input(n, "color", none).code, asTextExpr(input(n, "text", none)).code);
            } else if (id == "g_loop") {
                helpers_.insert("Jeu");
                out << ind + QString("while (Jeu.Boucle(%1))").arg(input(n, "bg", none).code);
                ++loopDepth_;
                block(flowTarget(n, "body"), depth, out, seen);
                --loopDepth_;
            } else if (id == "comment") {
                out << ind + "// " + n->param("text").simplified();
            } else if (id == "clear") {
                helpers_.insert("Effacer");
                out << ind + "Effacer();";
            } else if (id == "waitkey") {
                helpers_.insert("AttendreTouche");
                out << ind + "AttendreTouche();";
            } else if (id == "exit") {
                out << ind + "Environment.Exit(0);";
            } else if (id == "break" || id == "continue") {
                if (loopDepth_ == 0) problem(n, "ce bloc doit être placé dans une boucle");
                out << ind + (id == "break" ? "break;" : "continue;");
            } else if (id == "setvar") {
                const QString name = csName(n->param("name"));
                DataType vt;
                if (!lookupVar(name, &vt)) vt = typeFromName(n->param("type"));
                const Expr e = input(n, "value", none, vt);
                expect(n, QString("la variable « %1 »").arg(name), e, vt);
                out << ind + QString("%1 = %2;").arg(name, e.code);
            } else if (id == "addvar") {
                const QString name = csName(n->param("name"));
                DataType vt;
                if (!lookupVar(name, &vt)) {
                    problem(n, QString("la variable « %1 » n'est jamais définie").arg(name));
                } else if (vt == DataType::Bool || isList(vt)) {
                    problem(n, QString("on ne peut pas « ajouter à » la variable « %1 » (%2)").arg(name, typeName(vt)));
                } else {
                    const Expr e = input(n, "amount", none, vt);
                    if (vt == DataType::Number) expect(n, QString("la variable « %1 »").arg(name), e, vt);
                    out << ind + QString("%1 += %2;").arg(name, vt == DataType::Text ? asTextExpr(e).code : e.code);
                }
            } else if (id == "wait") {
                usings_.insert("System.Threading");
                out << ind + QString("Thread.Sleep((int)((%1) * 1000));").arg(input(n, "seconds", none).code);
            } else if (id == "csharp") {
                QString c = n->param("code").trimmed();
                if (!c.isEmpty()) {
                    if (!c.endsWith(';') && !c.endsWith('}') && !c.endsWith('{')) c += ';';
                    for (const QString& l : c.split('\n')) out << ind + l.trimmed();
                }
            } else if (id == "call") {
                out << ind + callExpr(n, none, false).code + ";";
            } else if (id == "return") {
                if (curFunc_ && curFunc_->hasRet) {
                    const Expr e = input(n, "value", none, curFunc_->ret);
                    expect(n, QString("la fonction « %1 »").arg(curFunc_->name), e, curFunc_->ret);
                    out << ind + QString("return %1;").arg(e.code);
                } else {
                    out << ind + "return;";
                }
                ends = true;
            } else if (id == "listadd" || id == "listremove" || id == "listset" || id == "listclear" || id == "listorder") {
                QString name;
                DataType lt;
                if (listVar(n, &name, &lt)) {
                    if (id == "listclear") {
                        out << ind + QString("%1.Clear();").arg(name);
                    } else if (id == "listorder") {
                        const QString op = n->param("op");
                        if (op == "inverser") out << ind + QString("%1.Reverse();").arg(name);
                        else {
                            out << ind + QString("%1.Sort();").arg(name);
                            if (op == "trier décroissant") out << ind + QString("%1.Reverse();").arg(name);
                        }
                    } else if (id == "listset") {
                        const Expr e = input(n, "value", none, elemType(lt));
                        expect(n, QString("la liste « %1 »").arg(name), e, elemType(lt));
                        out << ind + QString("%1[(int)(%2)] = %3;").arg(name, input(n, "index", none).code, e.code);
                    } else if (id == "listadd") {
                        const Expr e = input(n, "value", none, elemType(lt));
                        expect(n, QString("la liste « %1 »").arg(name), e, elemType(lt));
                        out << ind + QString("%1.Add(%2);").arg(name, e.code);
                    } else {
                        out << ind + QString("%1.RemoveAt((int)(%2));").arg(name, input(n, "index", none).code);
                    }
                }
            } else if (id == "if") {
                out << ind + QString("if (%1)").arg(input(n, "cond", none).code);
                const bool t = block(flowTarget(n, "then"), depth, out, seen);
                bool e = false;
                if (NodeItem* other = flowTarget(n, "else")) {
                    out << ind + "else";
                    e = block(other, depth, out, seen);
                    ends = t && e;
                }
            } else if (id == "while") {
                out << ind + QString("while (%1)").arg(input(n, "cond", none).code);
                ++loopDepth_;
                block(flowTarget(n, "body"), depth, out, seen);
                --loopDepth_;
            } else if (id == "repeat") {
                const QString v = csName(n->param("var"));
                DataType dummy;
                if (lookupVar(v, &dummy))
                    problem(n, QString("le compteur « %1 » a le même nom qu'une variable ou une boucle englobante").arg(v));
                // la borne est calculée UNE seule fois (sinon « Demander » serait posé à chaque tour)
                out << ind + QString("for (double %1 = 0, _fin_%1 = %2; %1 < _fin_%1; %1++)")
                                 .arg(v, input(n, "count", none).code);
                loopVars_[v].push_back(DataType::Number);
                ++loopDepth_;
                block(flowTarget(n, "body"), depth, out, seen);
                --loopDepth_;
                loopVars_[v].pop_back();
            } else if (id == "forrange") {
                const QString v = csName(n->param("var"));
                DataType dummy;
                if (lookupVar(v, &dummy))
                    problem(n, QString("le compteur « %1 » a le même nom qu'une variable ou une boucle englobante").arg(v));
                out << ind + QString("for (double %1 = %2, _fin_%1 = %3, _pas_%1 = %4; _pas_%1 > 0 ? %1 <= _fin_%1 : %1 >= _fin_%1; %1 += _pas_%1)")
                                 .arg(v, input(n, "from", none).code, input(n, "to", none).code, input(n, "step", none).code);
                loopVars_[v].push_back(DataType::Number);
                ++loopDepth_;
                block(flowTarget(n, "body"), depth, out, seen);
                --loopDepth_;
                loopVars_[v].pop_back();
            } else if (id == "foreach") {
                QString name;
                DataType lt = DataType::NumList;
                const bool ok = listVar(n, &name, &lt);
                const DataType et = ok ? elemType(lt) : DataType::Number;
                const QString v = csName(n->param("element"));
                DataType dummy;
                if (lookupVar(v, &dummy))
                    problem(n, QString("l'élément « %1 » a le même nom qu'une variable ou une boucle englobante").arg(v));
                out << ind + QString("foreach (%1 %2 in %3)").arg(csType(et), v, name);
                loopVars_[v].push_back(et);
                ++loopDepth_;
                block(flowTarget(n, "body"), depth, out, seen);
                --loopDepth_;
                loopVars_[v].pop_back();
            }
            n = flowTarget(n, "next");
        }
        return ends;
    }
};

}  // namespace

CodeResult generateCSharp(const NodeScene& scene, CodeMode mode) { return Gen(scene, mode).run(); }
CodeResult generateCSharp(const NodeScene& scene, bool forRun) {
    return Gen(scene, forRun ? CodeMode::Run : CodeMode::Export).run();
}
