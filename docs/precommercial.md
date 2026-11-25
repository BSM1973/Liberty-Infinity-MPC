# Liberty Infinity v0.9.5 — Précommercialisation

## Livrables de test
- MPC : archive SDK ARM32/ARM64 expérimentale. Suivre les instructions du SDK et du firmware de la machine. Ne pas déployer sur une configuration de production sans sauvegarde.
- macOS Apple Silicon : application Standalone (pas AU/VST3), ZIP signé ad hoc et PKG de test non signé. Le PKG installe Liberty Infinity MPC Test.app dans Applications. Ce n’est pas un installateur commercial notarisé. Désinstallation : quitter puis retirer l’application de Applications. Sauvegarder les banques personnelles avant remplacement.
- Banque CC0 séparée : copier les deux fichiers PCM et MAP ensemble, hôte arrêté, sauvegarder la banque d’origine. Sur Mac, modifier le bundle invalide sa signature ; réserver cette opération à un environnement de test.
- Carte de niveaux facultative : uniquement avec la Factory originale sans autre MAP, ne pas fusionner avec les banques personnelles. Supprimer la carte pour revenir au niveau original.

## Conditions avant vente
- Écouter les 100 WAV et les quatre presets acoustiques, relever notes, nuances, défauts, qualité des boucles et niveaux. Les noms Factory ne sont pas tous adaptés aux sons de la banque expérimentale.
- Tests sur MPC ARM32/ARM64 et macOS Apple Silicon, écran 13 pouces, MIDI, sauvegarde, réouverture, import/export et lecture longue. Streaming désactivé par défaut tant que les lectures manquantes persistent.
- Vérifier la licence et provenance des nouvelles prises, compléter les pianos électriques et guitares avec enregistrements redistribuables.
- Signer l’application avec Developer ID Application et le PKG avec Developer ID Installer ; utiliser notarytool, stapler et vérifier Gatekeeper avant publication. Aucune identité, clé ou secret de signature n'est fourni par ce workflow de test.
