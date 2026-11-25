# Liberty Infinity — MPC ROMpler v0.4.0

Plugin principal MPC ARM32/ARM64, MPC OS 3.10.1+ External Plugin Support bêta. Application macOS ARM64 de test, même DSP et banque. Pas un AU.

100 presets de développement originaux dans 12 catégories, 500 zones stéréo PCM16 24 kHz, 2 secondes par zone, environ 96 MB de PCM préchargé par instance. Les sons sont générés hors ligne (pas de synthèse d'oscillateur dans le moteur de lecture). Les noms de pianos/cordes/cuivres décrivent des approximations synthétiques, pas des enregistrements acoustiques. Boucles à fondu sur les catégories tenues. Banque premium et qualité sonore à valider.

Mac : catégories, pagination, clavier virtuel/informatique, MIDI CoreMIDI (sources présentes au lancement, notes/vélocité, sustain, pitch bend), vu-mètres. Les branchements MIDI après lancement et les messages MIDI complexes restent à valider. Signature ad hoc, non notarisé. MPC : sélection via programmes natifs ou menu Sound (ce dernier sélectionne le sample, pas tous les réglages du programme), Q-Links. Pas encore de vu-mètres MPC. Les presets de projet v0.2 peuvent nécessiter une nouvelle sélection en raison de la plage Sound élargie.

CI : 500 zones testées pour signal non nul, fini et borné, et contrôle des sons bouclés, puis compilation et packaging. Ces tests ne remplacent pas l'écoute ni la validation CPU/RAM et UI sur MPC. Installer seulement pour essais, sauvegarder ses projets. SDK MIT épinglé au commit e78bad508a305a92443758eb2bc8b428e9e7a687. Aucun contenu reFX.

## Black Gold Edition

Façade noire, textes blancs, accents or champagne. Ressources originales partagées Mac et MPC : potentiomètres métallisés 127 images, vrais interrupteurs ON/OFF à LED ambre, sélecteurs de mode distincts, écran sombre. Sur Mac les sélecteurs ouvrent un menu et les vu-mètres sont dorés ; sur MPC les sélecteurs ouvrent le menu au double-tap. Le layout est contrôlé automatiquement pour 39 zones non superposées. Aperçu graphique statique disponible en artefact (pas une capture réelle). Contrôle visuel sur les deux hôtes encore nécessaire. DSP et banque inchangés, hors numéro de version.
