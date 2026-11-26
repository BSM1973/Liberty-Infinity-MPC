# Licences Liberty Infinity — préparation commerciale

Le moteur vérifie une signature EdDSA-BLAKE2b via Monocypher 4.0.2, avec une clé publique compilée et une empreinte BLAKE2b-256 du fichier MAP et PCM. Le reçu contient UUID de collection et identifiant opaque de commande. Aucune clé privée dans le code, aucun appel réseau sur MPC. La vérification se déroule lors de l'instanciation : lecture complète du PCM par blocs, donc temps de démarrage à mesurer sur matériel. La signature garantit authenticité/intégrité, pas une protection contre la copie d'une Library et de son reçu.

## Procédure de production (non exécutée)
Sur un ordinateur de confiance, hors dépôt et sans transmettre la clé privée :

1. `python tools/license_expansion.py keygen /chemin-prive/seed.bin /chemin-prive/public.hex` — sauvegarder la graine de 32 octets, permissions 0600.
2. `python tools/license_expansion.py policy /chemin-prive/public.hex /chemin-prive/LicensePolicy.h` — remplacez le fichier public de configuration dans src avant la compilation commerciale. Cette politique désactive les licences DEMO non signées. Ne publiez jamais seed.bin.
3. `python tools/license_expansion.py issue /chemin-prive/seed.bin dossier-pack commande-123 License-signee.txt` — l'outil refuse l'écrasement. Après validation, intégrer le reçu signé au pack sous License.txt. Les droits de redistribution des WAV restent nécessaires.
4. Recompiler et valider le plugin MPC ARM32/ARM64 avec la clé publique de production. Tester licence valide, clé différente, PCM modifié, absence du reçu et ancien projet. Conserver le même UUID pour la même version immuable.

La configuration v0.9.9 reste une version de développement : clé publique nulle, DEMO permis, aucune licence commerciale délivrée. Aucune clé réelle n'a été générée. CI utilise uniquement une graine de test publique. Pour une boutique, délivrance manuelle dans un premier temps ; intégration Shopify, anti-copie, activation par appareil et révocation ne sont pas implémentées. Vérifier les exigences Akai et compatibilité du firmware avant vente.

Monocypher upstream: https://github.com/LoupVaillant/Monocypher/tree/0d85f98c9d9b0227e42cf795cb527dff372b40a4. Les sources et licences sont conservées dans src/vendor.
