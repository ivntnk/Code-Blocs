# Contribuer à Blocs

## Mise en route

1. Installer Qt 6.4+, CMake, Ninja (voir `README.md`).
2. Windows : `cmake --preset mingw-debug` puis `cmake --build --preset mingw-debug` (Linux/macOS : preset `debug` + `-DCMAKE_PREFIX_PATH=…`).
3. `ctest --test-dir build/mingw-debug --output-on-failure` doit passer avant toute proposition de changement.

## Style

- C++17, 4 espaces, 120 colonnes, accolades attachées : `.clang-format` fait foi.
- Commentaires et textes affichés à l'utilisateur en français, identifiants en anglais (comme le code existant).
- Membres privés suffixés `_`, constantes `kNom` (voir `.clang-tidy`).
- Les fichiers `.cpp`/`.h` sont en UTF-8, fins de ligne LF.

## Ajouter un bloc

Une ligne dans le catalogue de `src/graph.cpp`, son code dans `src/codegen.cpp`, et si possible un exemple
ou un test qui l'utilise. `tests/test_graph.cpp` vérifie la cohérence du catalogue (identifiants uniques,
titres, catégories).

## Avant de proposer un changement

- [ ] Ça compile sans nouvel avertissement (`cmake --preset strict` pour être exigeant).
- [ ] `ctest` passe ; un test a été ajouté ou adapté si le comportement change.
- [ ] `format-check` ne signale rien sur les fichiers touchés.
- [ ] `CHANGELOG.md` mis à jour (section « Non publié »).

## Rendre l'analyse statique bloquante

Le code existant n'a pas encore été passé à clang-format/clang-tidy, donc ces contrôles sont informatifs
(`continue-on-error: true` dans `.github/workflows/ci.yml`). Marche à suivre :

1. Commit « propre », puis `cmake --build build/mingw-debug --target format` et commit séparé « Formatage ».
2. Parcourir `--target tidy`, corriger ou désactiver (avec une raison) les règles dans `.clang-tidy`.
3. Retirer `continue-on-error` du job `quality`, et passer `WarningsAsErrors` à `'*'` dans `.clang-tidy`.

## Publier une version

1. Mettre à jour `VERSION` dans `project(Blocs …)` (`CMakeLists.txt`) et `CHANGELOG.md`.
2. `git tag vX.Y.Z && git push --tags` : le workflow `release.yml` fabrique le zip et l'installateur.
