# Architecture

```
            ┌──────────────┐   toJson / fromJson    ┌───────────────┐
 mainwindow │  NodeScene   │ ─────────────────────▶ │ fichier .blocs│
 explorer   │ (editor.*)   │                        └───────────────┘
 ...        │  NodeItem    │
            │  PortItem    │──┐ catalogue (graph.*)
            └──────┬───────┘  │ 77 types de blocs
                   │ generateCSharp(scene, mode)
                   ▼
            ┌──────────────┐
            │ codegen.*    │ ──▶ code C#
            └──────┬───────┘
        ┌──────────┼───────────────┐
        ▼          ▼               ▼
   runner.*    gamewindow.*    exporter.*
 (dotnet run)  (fenêtre de jeu) (.exe / projet VS)
```

| Module | Rôle |
|---|---|
| `graph.*` | Catalogue des blocs (`NodeDef`), éléments graphiques `NodeItem`, `PortItem`, `ConnectionItem`, types de données. |
| `editor.*` | `NodeScene` (le plan de travail), vue avec zoom/déplacement, sérialisation JSON, ressources (`Asset`). |
| `codegen.*` | Transforme la scène en C# (`CodeMode::Export`, `Run`, `Standalone`) et signale les problèmes (types, variables). |
| `runner.*` | Écrit un projet temporaire, lance `dotnet`, relaie la sortie ; dialogue avec la fenêtre de jeu. |
| `gamewindow.*` | Fenêtre de jeu : dessin, clavier/souris, sprites, sons (WinMM sous Windows). |
| `exporter.*` | `.exe` léger / autonome / projet Visual Studio ; `writeProject` est utilisable sans lancer `dotnet`. |
| `explorer.*` | Explorateur de fichiers du dossier de projet. |
| `assetsdialog.*` | Gestion des images et sons du projet. |
| `examples.*`, `tutorials.*`, `tutorialdialog.*` | Projets d'exemple, tutoriels (HTML), fenêtre d'aide. |
| `history.h` | Annulation par instantanés du projet entier (200 états maximum). |
| `highlighter.h` | Coloration syntaxique du code C# affiché. |
| `mainwindow.*` | Fenêtre principale, menus, onglets, assemblage de l'ensemble. |
| `main.cpp` | Point d'entrée, thème sombre. |

## Découpage CMake

- `blocs_core` (bibliothèque statique) : tout `src/` sauf `main.cpp`. C'est ce qui est testé et analysé.
- `Blocs` (exécutable) : `main.cpp` + ressources Qt (`resources.qrc`, `:/icon.png`) + ressources Windows.
- `tests/` : un exécutable Qt Test par domaine, tous liés à `blocs_core`.

Le code est portable : les appels Windows (`PlaySoundW`, etc.) sont protégés par `Q_OS_WIN`.
