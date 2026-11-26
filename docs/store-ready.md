# EXPANSION STORE READY — v0.9.10
Cinematic Horizons : 100 presets (16 basses, 12 claviers, 20 pads, 16 plucks, 16 leads, 8 cloches, 8 atmosphères, 4 FX), 1000 rendus stéréo originaux hors ligne. Pack candidat DEMO, séparé de Factory 1. Aucune prise acoustique ni validation humaine revendiquée.

## Installation
Sur ordinateur, Python 3.12 avec numpy scipy pillow. Arrêter le plugin et l'hôte, sauvegarder le dossier MPC et les projets. Monter le stockage MPC. Exécuter :
```
python tools/mpc_expansion_manager.py install /chemin/cinematic-horizons "/chemin/VST2 Plugins/Liberty Music Software - VST - Liberty Infinity" 1
```
Choisir un slot libre 1–4, ne jamais déplacer une collection utilisée dans un projet. La commande copie le pack et rafraîchit Plugin Skins/libraries.png. Ni Factory ni les binaires ne sont modifiés. La commande refresh <dossier-plugin> permet de refaire le visuel après fermeture du plugin. L'image et le nombre de presets reflètent les fichiers présents, pas un verdict de licence. Le moteur vérifie les licences. Pas de chargement dynamique ni file picker natif validé. Si rafraîchissement échoue après copie, relancer refresh.

## Production
Générer la clé privée localement avec tools/license_expansion.py keygen, la conserver hors dépôt. Actions > Liberty Infinity MPC - production key candidate > Run workflow : saisir uniquement les 64 caractères hex de la clé publique. Le workflow compile les deux architectures avec DEMO non signé désactivé. Générer ensuite le reçu signé pour chaque commande avec issue. Aucune clé réelle ni licence payante délivrée. Le reçu peut être copié; pas d'activation liée à un appareil.

## Validation physique à renseigner
Modèle, firmware, architecture, support, testeur, date. Activer External Plugin Support selon instructions SDK; vérifier compatibilité réelle. Tester installation, véritable couverture après redémarrage, navigation catégories, Q-Link, préécoute, les 100 presets à plusieurs notes/vélocités, boucles, pics CPU, RAM et 60 secondes de jeu, sauvegarde/réouverture de projet, pack absent ou reçu invalide, retour au pack original. Mesurer temps d'ouverture signé (hachage PCM complet). Contrôler accents, chevauchements, mises à jour/rollback. Aucun contrôle matériel n'est déclaré réussi. Écoute humaine et validation matériel obligatoires avant commercialisation.
