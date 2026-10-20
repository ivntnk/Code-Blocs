# Journal des modifications

Format inspiré de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/), versionnement [SemVer](https://semver.org/lang/fr/).

## [Non publié]

### Ajouté
- CMake modernisé : bibliothèque `blocs_core` + exécutable, options de qualité, installation et CPack.
- Tests automatisés (Qt Test / CTest) : catalogue, JSON, génération C#, export, tutoriels, historique.
- `CMakePresets.json` (debug, release, strict, lint, asan).
- `.clang-format`, `.clang-tidy`, cppcheck, cibles `format`, `format-check`, `tidy`, `cppcheck`.
- Informations de version dans les propriétés du `.exe` (Windows).
- Installateur Inno Setup (`installer/Blocs.iss`) avec association des fichiers `.blocs`.
- Intégration continue GitHub Actions (Linux, Windows) et workflow de publication.
- `.editorconfig`, `.gitignore`, `.gitattributes`, `CONTRIBUTING.md`, `docs/ARCHITECTURE.md`.

### Modifié
- `README.md` réécrit.
- Explorateur : sans dossier de projet, il affiche maintenant les vrais lecteurs de l'ordinateur (C:, D:…) et déplie le
  dossier personnel, au lieu d'une page « Aucun dossier ouvert ». Nouveau : clic droit > « Ouvrir comme dossier de projet »,
  boutons 📁 (ouvrir un dossier) et 🖥 (tout l'ordinateur) dans l'en-tête.
- Les lecteurs, le dossier personnel (et ses parents), Windows et Program Files ne peuvent plus être supprimés,
  renommés ni déplacés depuis l'explorateur.

## [1.0.0]

### Ajouté
- Explorateur de fichiers du dossier de projet (onglet « Explorateur ») : nouveau programme/dossier/fichier,
  renommer (F2), supprimer (corbeille), copier-couper-coller, dupliquer, glisser-déposer, importer des fichiers,
  ouverture d'un `.blocs` par double-clic, aperçu des images, import d'images/WAV comme ressources.
- Menu Fichier : Ouvrir un dossier (Ctrl+Maj+O), Nouveau projet (dossier), Dossiers récents, Fermer le dossier.
  Le dernier dossier est rouvert au lancement.
- Raccourcis : Ctrl+Maj+E (explorateur), Ctrl+Maj+B (exporter l'application).
- Tutoriel 13 : organiser son projet.
