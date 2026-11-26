# Cinematic Horizons — MASTER EDITION v0.9.12 (QA)
Produit destiné exclusivement au plugin Liberty Infinity sur MPC. Le Standalone macOS est un outil de test, pas un produit commercial.

Le dossier Cinematic Horizons contient la banque PCM24 stéréo 44,1 kHz, 100 presets, 1000 sources synthétiques originales rendues hors ligne, les métadonnées, la couverture, les contrôles SHA256 et un reçu DEMO. La calibration par preset est mesurée sur le rendu du moteur, fenêtre active de 100 ms à vélocité forte, cible RMS -25 dBFS, trim +/-12 dB, crête mono-note <= -12 dBFS. Ce n'est pas LUFS ni une garantie d'équilibre en contexte polyphonique. Nouvelle identité UUID : installer en emplacement libre, ne jamais écraser une version utilisée par un projet.

## Installation QA
Arrêter le plugin/hôte, sauvegarder la MPC et les projets. Installer Python 3.12 et les dépendances numpy scipy pillow sur ordinateur. Depuis le dossier extrait :
```
python Installer/mpc_expansion_manager.py install "Cinematic Horizons" "/chemin/VST2 Plugins/Liberty Music Software - VST - Liberty Infinity" 1
```
Choisir un emplacement 1–4 libre ; le gestionnaire vérifie les copies, préserve Factory 1 et actualise les couvertures. Réouvrir le plugin. Si actualisation échoue, relancer refresh. Nécessite une version compatible du plugin MPC, firmware/External Plugin Support vérifiés sur matériel. 

## Contrôles avant vente
Écouter les 100 rendus et jouer les 100 presets à diverses notes/vélocités. Vérifier différences musicales, clics de boucles, enveloppes, niveau, polyphonie, RAM et CPU sur MPC physiques, affichage et réouverture de projets. CI mesure 24/32/64 voix sur hôte, pas sur les processeurs MPC. Le test de sauvegarde utilise le format SDK, pas un projet MPC physique.

## Licences
Pack de contrôle DEMO. La vente exige une clé publique de production configurée et une licence signée avec la clé privée locale, selon docs/expansion-licensing.md et le workflow mpc-production-key.yml. Aucune activation réelle ni mise en vente. Ne jamais transmettre la graine privée. La signature garantit authenticité/intégrité, pas l'anti-copie. Vérifier également les conditions de distribution et la compatibilité Akai.
