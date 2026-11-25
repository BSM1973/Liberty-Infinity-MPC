# Liberty Infinity — MPC ROMpler v0.2.0

Liberty Music Software. Instrument Akai MPC OS 3.10.1+ (External Plugin Support bêta). Le produit principal reste le plugin MPC ARM32/ARM64. L'application macOS ARM64 est un banc d'essai local indépendant de la MPC, pas une télécommande et pas un AU.

Moteur C++ commun : 32 voix, 20 PCM stéréo de test, 4 presets, ADSR, filtre résonant et enveloppe, amp modifier, master filter, delay et reverb expérimentaux. Les algorithmes et performances ne sont pas validés sur matériel. Samples courts sans boucle, banque commerciale 100 presets à développer. Aucun contenu reFX.

Façade rétro commune : argent, LCD bleu, boutons argentés, sections nommées. Le Mac présente une liste de quatre presets, clavier d'ordinateur et clavier de test à la souris, et vu-mètres. MPC : sélection via menu du paramètre Sound / presets natifs, trois banques Q-Links. Les vu-mètres MPC ne sont pas implémentés ; la façade MPC et les gestes tactiles restent à tester sur appareil. MIDI externe sur Mac à ajouter. Signature ad hoc seulement, pas de notarisation.

SDK MIT épinglé au commit e78bad508a305a92443758eb2bc8b428e9e7a687. Voir GitHub Actions pour les deux ZIP. Pour Mac : extraire le ZIP de l'artefact puis le ZIP de l'application. Utiliser les touches A W S E D F T G Y H U J K O L P ou le clavier affiché.

Pour MPC : dossier complet dans VST2 Plugins sur stockage externe ; activer External Plugin Support. Sauvegarder les projets avant essai ; MPC Sample non compatible instruments. Valider le build et le matériel avant distribution.
