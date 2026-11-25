# Liberty Infinity — MPC ROMpler v0.1.0

Liberty Music Software. Cible : MPC OS 3.10.1+ avec External Plugin Support (bêta).

Prototype : lecture PCM stéréo de 20 samples de test originaux générés hors ligne (4 sons, 5 notes racines), 32 voix, interpolation linéaire, ADSR, filtre passe-bas, panoramique, gain, pitch bend ±2 demi-tons et sustain. Aucun oscillateur temps réel. Les sons sont courts et sans boucle, uniquement destinés à valider la lecture. La banque commerciale de 100 presets, les effets et la façade LCD rétro personnalisée ne sont pas encore implémentés. Aucun contenu reFX.

Le SDK officiel MIT est épinglé au commit e78bad508a305a92443758eb2bc8b428e9e7a687. Générer le contenu puis compiler :

    python3 tools/generate_samples.py
    git clone https://github.com/inmusic-software/mpc-plugin-builder-sdk.git sdk
    git -C sdk checkout e78bad508a305a92443758eb2bc8b428e9e7a687
    python3 sdk/tools/build.py plugins/Infinity
    python3 sdk/tools/package.py plugins/Infinity

Installer uniquement un build testé : dossier complet dans VST2 Plugins sur stockage externe. Activer les améliorations et External Plugin Support dans les préférences MPC. Sauvegarder les projets avant essai. La MPC Sample n'accepte pas les instruments.

Interface initiale générée par le SDK, argent et cyan, avec Q-Links ; pas encore la façade finale. Validation physique obligatoire.
