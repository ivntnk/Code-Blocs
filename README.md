# Blocs

Programmation visuelle par blocs : on assemble des blocs (77 dans le catalogue), on les relie par des fils,
et Blocs génère le programme **C#** correspondant, l'exécute, ou l'exporte en application Windows (`.exe`)
ou en projet Visual Studio.

## Fonctionnalités

- Éditeur de graphe : blocs, ports d'ordre d'exécution (▶) et de valeur (●), zoom, annulation/rétablissement.
- Génération de code C# en trois modes : export « propre », exécution depuis Blocs, application autonome.
- Fenêtre de jeu intégrée (graphismes, clavier, souris, images, sons WAV).
- Export en `.exe` léger ou autonome, ou en projet Visual Studio / `dotnet`.
- Explorateur de fichiers réel (comme dans VS Code) : sans projet ouvert il parcourt tout l'ordinateur (C:, D:…) ;
  clic droit sur un dossier > « Ouvrir comme dossier de projet ». Nouveau programme, renommer (F2),
  copier/coller, glisser-déposer, aperçu des images, import de ressources. Lecteurs, dossier personnel,
  Windows et Program Files sont protégés contre la suppression et le déplacement.
- Projets d'exemple et 13 tutoriels intégrés.

## Prérequis

| Pour… | Il faut… |
|---|---|
| Compiler Blocs | Qt 6.4+ (6.12 testé en local), CMake 3.21+, Ninja, un compilateur C++17 (MinGW 64-bit, GCC, Clang ou MSVC) |
| Exécuter / exporter des programmes | SDK .NET 6+ (8 recommandé) — <https://dotnet.microsoft.com/download> |
| Fabriquer l'installateur Windows | Inno Setup 6.3+ |
| Contrôles qualité (facultatif) | clang-format, clang-tidy, cppcheck |

## Compiler

Avec Qt Creator : *Ouvrir un projet…* → `CMakeLists.txt` → kit **Desktop Qt 6.x MinGW 64-bit** → *Exécuter*.

En ligne de commande (les *presets* sont dans `CMakePresets.json`) :

```bat
:: Windows (Qt 6.12.0 + MinGW 13.1 dans C:\Qt, générateur « MinGW Makefiles », sans Ninja)
cmake --preset mingw-release
cmake --build --preset mingw-release
```

```bash
# Linux / macOS (nécessite Ninja)
cmake --preset release -DCMAKE_PREFIX_PATH=/chemin/vers/Qt/6.12.0/gcc_64
cmake --build --preset release
```

Les presets `mingw-*` supposent l'installation Qt par défaut (`C:/Qt/6.12.0/mingw_64`,
`C:/Qt/Tools/mingw1310_64`). Si les tiens diffèrent, copier `CMakeUserPresets.json.example` en
`CMakeUserPresets.json` et l'adapter (ce fichier est ignoré par Git).

> **Qt Creator** importe les presets comme des kits. Dans *Projets*, ne garder cochés que
> « Desktop Qt 6.12.0 MinGW 64-bit » ou les kits « Windows MinGW - … ».

Les exécutables sont dans `build/<preset>/`. Presets Windows : `mingw-debug`, `mingw-release`, `mingw-strict`. Presets Linux/macOS/CI : `debug`, `release`, `strict`
(avertissements supplémentaires, traités comme des erreurs), `lint` (clang-tidy + cppcheck à chaque compilation),
`asan` (ASan + UBSan, hors MinGW).

## Tester

```bash
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --test-dir build/mingw-debug --output-on-failure
```

Les tests (Qt Test, dossier `tests/`) vérifient le catalogue de blocs, la sauvegarde JSON, la génération C# de
tous les exemples dans les 3 modes, l'export de projet, l'explorateur de fichiers et la cohérence des tutoriels. Ils tournent sans écran
(`QT_QPA_PLATFORM=offscreen`, déjà configuré dans CTest). Sous Windows, le dossier `bin` de Qt doit être dans le `PATH`.

## Qualité du code

| Commande | Effet |
|---|---|
| `cmake --build build/mingw-debug --target format-check` | vérifie le style (clang-format), ne modifie rien |
| `cmake --build build/mingw-debug --target format` | **reformate `src/` et `tests/` sur place** (à lancer après un commit) |
| `cmake --build build/mingw-debug --target tidy` | analyse clang-tidy (lecture seule) |
| `cmake --build build/mingw-debug --target cppcheck` | analyse cppcheck (lecture seule) |

Configuration : `.clang-format`, `.clang-tidy`, `cppcheck-suppressions.txt`. L'intégration continue
(`.github/workflows/ci.yml`) compile et teste sous Linux et Windows ; les contrôles de style/analyse y sont
**informatifs** pour l'instant (voir `CONTRIBUTING.md`).

## Distribuer

```bash
cmake --preset mingw-release
cmake --build --preset mingw-release
cmake --install build/mingw-release --prefix dist/Blocs     # copie aussi les DLL et plugins Qt (Windows)
iscc installer\Blocs.iss                              # → dist\installer\Blocs-<version>-Setup.exe
```

Autre méthode, inchangée : `scripts\deploy.bat "chemin\complet\vers\Blocs.exe"` (nécessite l'invite Qt MinGW).
Un tag `vX.Y.Z` déclenche `.github/workflows/release.yml` (zip portable + installateur).
La version se règle à un seul endroit : `project(Blocs VERSION …)` dans `CMakeLists.txt`.

## Organisation du dépôt

```
src/             code C++
tests/           tests Qt Test (CTest)
resources/       icône, ressources Qt, ressource Windows
cmake/           modules CMake (avertissements, analyse statique, outils, paquets, version Windows)
installer/       script Inno Setup
scripts/         deploy.bat
docs/            architecture
.github/         intégration continue et publication
```

Détail des modules : [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). Historique : [CHANGELOG.md](CHANGELOG.md).
Contribuer : [CONTRIBUTING.md](CONTRIBUTING.md).