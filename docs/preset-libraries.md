# Liberty Infinity — Collections de presets v0.9.7

## Factory 1
Les 100 presets d'origine restent inclus, avec les mêmes noms, indices et données audio. Sur macOS, le sélecteur de collections affiche Factory 1 et les extensions installées. Sur MPC, le sélecteur natif est conservé, nommé Factory 1.

## Créer un pack vendable (macOS)
1. Créer des variations de sons Factory 1 et les enregistrer dans Mes presets.
2. Menu Collections > Créer une Library de presets. Les 1 à 500 presets personnels sont inclus ; donner un nom unique, exporter le fichier .infinitylibrary.
3. Tester sur une autre installation via Collections > Installer une Library. Choisir la collection dans le navigateur. Le fichier est indépendant de l'application et peut être livré à un client.

Le pack comporte un UUID, un nom, un éditeur Liberty Music Software, une référence Factory 1 et des réglages normalisés. Il ne contient pas de WAV, ne remplace pas la banque d'origine et ne nécessite pas de nouvelle notarisation de l'application pour chaque pack. L'installation se fait dans Application Support/Liberty Music Software/Liberty Infinity/Libraries ; les favoris Factory ne sont pas modifiés. Les sauvegardes de presets personnels ne contiennent pas les packs : conserver les fichiers .infinitylibrary séparément.

## Limites et extension commerciale
Ce format n'est pas chiffré et n'a pas de protection anti-copie ni d'activation. Ne pas promettre de licence technique ou de verrouillage. La vente de presets supplémentaires basés sur les sons existants est l'objectif immédiat. La prochaine étape doit ajouter un stockage de banques PCM indépendantes, des identifiants stables de collection/preset dans les états MPC, un catalogue dynamique et des tests sur les deux MPC. Les packs avec de nouveaux multisamples nécessitent des droits de redistribution et un format distinct ; ils ne sont pas pris en charge dans cette version. Aucun nouveau preset commercial n'est fourni.
