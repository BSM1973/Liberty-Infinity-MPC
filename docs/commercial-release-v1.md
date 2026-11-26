# Liberty Infinity — préparation commerciale v1.0
Gel fonctionnel sur 2a7ccf6f4033a425d1090a890c03047fa6d37715, run 45791021315, deux builds verts. Aucun changement DSP/UI.
Fichiers validés :
- MPC : https://github.com/BSM1973/Liberty-Infinity-MPC/actions/runs/45791021315/artifacts/15102813179
- Master Edition QA : https://github.com/BSM1973/Liberty-Infinity-MPC/actions/runs/45791021315/artifacts/15103187165
- Écoute : https://github.com/BSM1973/Liberty-Infinity-MPC/actions/runs/45791021315/artifacts/15102948375
- Rapports : https://github.com/BSM1973/Liberty-Infinity-MPC/actions/runs/45791021315/artifacts/15103207204

100 presets Master et 1000 rendus distincts. Crête maximale mono-note -12,1658 dBFS. RMS global avant -50,2595 à -35,1593 dBFS ; après -40,4934 à -28,744 dBFS. Ne pas confondre RMS global, RMS actif et LUFS. PCM décodé 7 056 000 octets par preset hors moteur et cache. Les essais de polyphonie hôte et d'état SDK ont réussi, mais ne certifient pas le matériel.

## Validation à renseigner avant production
| Contrôle | État | Observations |
|---|---|---|
| Écoute 100 Factory 1 + 100 Master | À faire | |
| MPC ARM32 (modèle, firmware, date) | À faire | |
| MPC ARM64 (modèle, firmware, date) | À faire | |
| Couverture, navigateur, préécoute, Q-Link, accents, chevauchements | À faire | |
| Polyphonie, RAM, CPU, chargement, stabilité | À faire | |
| Sauvegarde projet, banque absente et restaurée | À faire | |
| Compatibilité et droits de distribution SDK/firmware | À vérifier | |

Les confirmations du workflow sont des déclarations humaines, pas des tests matériels automatisés.

## Licence de production
Sur le Mac de l'éditeur, depuis le dépôt cloné :
```bash
mkdir -p "$HOME/.liberty-infinity-keys"
chmod 700 "$HOME/.liberty-infinity-keys"
python3 tools/license_expansion.py keygen "$HOME/.liberty-infinity-keys/seed.bin" "$HOME/.liberty-infinity-keys/public.hex"
cat "$HOME/.liberty-infinity-keys/public.hex"
```
Réutiliser une clé existante si elle existe. Sauvegarder la graine privée hors ligne, ne jamais la transmettre. Seul public.hex doit être communiqué. Après validation, lancer Actions > Liberty Infinity MPC - v1.0 production, fournir la clé publique et les trois confirmations + référence QA. Ce workflow manuel utilise le code figé, la banque Factory extraite du build vert, les rapports validés, désactive DEMO et estampille 1.0.0. Il vérifie les deux ELF, le ZIP, les empreintes, les 100 presets Factory et le mécanisme de signature.

Conserver la Master Edition QA exacte (UUID 0b8e8b40-4376-44bc-b1e2-74bc6c94291e), ne pas régénérer ni modifier PCM/MAP sous le même UUID. Pour chaque commande, générer License.txt avec tools/license_expansion.py issue, selon docs/expansion-licensing.md. Copier le dossier Master sans modifier les fichiers signés, remplacer le reçu DEMO par le reçu signé dans la copie client, livrer le ZIP de cette copie avec l'installateur et les instructions. Ne pas livrer de clé privée. Un reçu est copiable, pas d'activation par appareil. Tester la version production réelle avec reçu valide, absent et altéré sur les MPC physiques avant mise en vente. Aucun produit macOS commercial. Aucune publication boutique automatique.
