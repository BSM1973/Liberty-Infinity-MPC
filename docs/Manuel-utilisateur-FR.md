# LIBERTY INFINITY — Manuel utilisateur français v0.9.6

Liberty Music Software — version candidate, non validée sur tous les matériels. Instrument exclusivement ROMpler : lecture de multisamples, quatre couches maximum, filtres, enveloppes et effets. Aucun oscillateur de synthèse temps réel.

## Installation
MPC : utiliser exclusivement l'archive expérimentale compatible avec votre architecture et les instructions du SDK/firmware. Sauvegarder votre configuration avant installation. Ne pas installer l'application macOS sur MPC.
macOS : Apple Silicon ARM64, application Standalone. Le PKG installe l'application dans Applications. Ne pas contourner Gatekeeper ; utiliser le PKG signé/notarisé s'il a effectivement passé les vérifications Apple. Les builds de test ad hoc ne sont pas les installateurs commerciaux. Quitter avant toute mise à jour. Aucun AU/VST3/Windows.

## Jouer et naviguer
Sélectionner un preset Factory par catégorie, style ou recherche. Écouter avec le bouton Écouter, jouer au clavier virtuel ou MIDI. Favoris et presets personnels : commandes du navigateur macOS, stockage local. Les paramètres d'enveloppe Attack/Decay/Sustain/Release règlent l'évolution d'amplitude, Cutoff/Res le filtre, Pan/Volume le niveau et la balance. Delay et Reverb sont les effets intégrés. La polyphonie maximale est de 24 voix ARM32, 32 voix MPC ARM64, 64 voix macOS, sous réserve de charge réelle.

## Multisample Studio (macOS)
Menu Multisample > Ouvrir Multisample Studio. Sélectionner le preset (0–99), une des quatre couches, une source (0–99) distincte par couche. Ajouter ou déposer les WAV mono/stéréo (8–192 kHz, jusqu'à 120 s sous réserve de budget). Définir note fondamentale MIDI ou −1 pour la détection automatique, plages de notes et vélocités, boucle par glissement. Écouter prévisualise le WAV brut. Les zones sont projetées sur cinq notes 36/48/60/72/84 et quatre vélocités 16/48/80/112 : couvrir toutes ces combinaisons. Normalisation de crête ou RMS indicative ; régler gain/pan/accordage. Mute/Solo influencent l'export. Rôles : Normal, Round Robin (au moins deux couches), Vélocité douce/forte (une paire, transition 48–80), Relâchement (une couche déclenchée après note-off effectif, pédale relevée). Ces rôles partagent les quatre couches.

Enregistrer JSON conserve les chemins WAV absolus, pas les fichiers audio eux-mêmes. Conserver les WAV originaux à leur emplacement. Exporter banque MPC dans un dossier vide crée Factory.pcm et Factory.pcm.map ; ne remplace pas l'original. Fermer l'hôte avant remplacement, sauvegarder les deux fichiers. Une source remplacée peut modifier d'autres presets. Sur macOS modifier le bundle invalide la signature : réserver les banques personnalisées à un environnement de test tant qu'un stockage externe signé n'est pas disponible. Streaming disque optionnel, expérimental ; désactiver si décrochages. 2 Go maximum par banque ; 48 Mio PCM par slot, deux slots, hors DSP/cache.

## Sauvegarde macOS
Menu Sauvegarde > Exporter favoris et presets. Conserver le JSON et les banques/WAV séparément. Restaurer favoris et presets valide la structure puis demande confirmation avant remplacement ; exporter d'abord les réglages actuels. Ce JSON ne contient ni samples, ni projets DAW, ni états du firmware MPC. Ne pas fusionner des cartes personnelles et Level Match. Le Level Match est une option RMS réversible pour Factory d'origine uniquement.

## Bibliothèque et limites
100 Factory sources rendues hors ligne, et banque CC0 d'évaluation séparée (piano droit, violon solo, trompette, violoncelles pizzicato). La banque acoustique n'est pas une reproduction intégrale de ces instruments ; les registres extrêmes sont transposés. Pianos électriques et guitares réellement enregistrés à compléter. Voir PROVENANCE.json dans l'archive acoustique. N'utiliser que des WAV dont les droits autorisent redistribution si une banque est commercialisée.

## Dépannage
En cas de son absent : vérifier banque, sortie audio, MIDI, preset chargé, volume et couches actives. En cas de coupures : revenir au mode RAM, réduire la polyphonie, tester un stockage adapté. En cas d'échec export : vérifier couverture des zones, sources distinctes, rôles et budget mémoire. Conserver l'archive d'origine et rapporter architecture, firmware, version, preset, étapes et symptômes.

## Avant commercialisation
Écoute humaine des 100 WAV, tests de restitution sur MPC physiques et écran 13 pouces, sauvegarde/restauration réelle et inspection finale des caractères. Signature et notarisation doivent être vérifiées séparément. Les tests automatisés ne remplacent pas ces contrôles.
