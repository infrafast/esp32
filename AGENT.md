# Consignes à l'intention de l'agent

## Documents de référence autorisés

Lire avant intervention :

1. `docs/architecture.md` : architecture, patterns et procédure d'extension.
2. `docs/roadmap.md` : demandes, avancement, audits et validations.
3. `AGENT.md` : présentes consignes.

`README.md` est uniquement la présentation de haut niveau destinée aux utilisateurs. Les trois fichiers ci-dessus sont les seules références de travail du projet ; le README est leur porte d'entrée utilisateur.

## Discipline documentaire obligatoire

- Ne créer aucun autre fichier de documentation de contenu, quelle que soit son extension : pas de README supplémentaire, rapport d'audit séparé, plan, guide, changelog projet ou fichier de consignes parallèle.
- Mettre à jour ces fichiers existants à leur emplacement et avec leur casse exacte. Conserver le nom singulier `AGENT.md` demandé ; ne pas créer un second `AGENTS.md`.
- Répartir les informations : présentation utilisateur dans README ; fonctionnement et décisions architecturales stables dans architecture ; demandes, propositions, essais et résultats datés dans roadmap ; règles agent ici.
- Ne pas dupliquer une explication complète : créer un lien vers sa référence.
- Les sous-dossiers de `docs/` ne servent qu'aux pièces jointes : images, schémas, captures et données de mesure. Leur contenu explicatif reste dans architecture ou roadmap. Un schéma peut être une pièce jointe ; un rapport en PDF ne doit pas contourner cette règle.
- Conserver le script de reproduction PCB déjà présent comme support de preuve ; ne pas en faire une nouvelle source de consignes ou un rapport.
- Ne pas modifier ni supprimer la documentation livrée avec les dépendances tierces pour appliquer cette règle. Elle reste documentaire amont et ne remplace pas les références du projet.
- L'interdiction vise les nouveaux documents. Les fichiers source, configuration et tests nécessaires à une fonctionnalité restent possibles dans une tâche qui les autorise. La présente réorganisation est strictement documentaire.

## Avant et pendant une modification

- Vérifier le code actuel et les options effectives ; un ancien rapport ne prouve pas que le comportement décrit existe toujours.
- Inscrire la demande dans roadmap avec identifiant, origine, statut et critères de validation. Distinguer exigence utilisateur, proposition technique, hypothèse et constat.
- Identifier si le changement relève du framework Interval, d'une interface, d'un strip, d'un backend console ou des outils de build.
- Garder `setup()` déclaratif et le traitement courant de `app.refresh()` non bloquant.
- Suivre les contrats `DeviceInterface`, `MixerBase`, `MixerDevice<Transport>`, les squelettes et datasets décrits dans architecture. Ne pas contourner l'abstraction console par un cast vers un backend particulier.
- Préserver les identifiants persistants, la sérialisation et les conversions d'indices. Documenter toute migration nécessaire.
- Associer les inscriptions aux événements et les réservations de valeurs à leur désinscription/libération ; vérifier les pointeurs nuls, changements de sélection, remplacement d'interface et destruction.
- Employer des buffers bornés et contrôler tailles, allocations et durée de vie. Ne pas lancer une migration globale de conteneurs sans demande et mesures.
- Respecter les patchs locaux des dépendances ; ne pas remplacer une bibliothèque embarquée sans examiner leurs raisons.
- Considérer `platformio.ini` comme référence des environnements. Vérifier en particulier les différences USB et l'ordre TFT/W5500.
- Inspecter les effets des scripts avant exécution : packaging, déplacements, suppressions et appels réseau. Toute évolution d'outillage doit rendre les effets explicites et éviter les chemins machine obligatoires.

## Vérification et clôture

- Choisir les contrôles adaptés au changement. Pour firmware/build, vérifier les environnements concernés ; pour les scripts, contrôler les effets depuis un espace de travail propre ; pour console/strip, vérifier synchronisation, valeurs partagées et reconnexion.
- Pour une modification documentaire seule, vérifier liens, chemins, cohérence et conservation des informations ; ne pas prétendre avoir compilé ou testé du matériel.
- Ne pas copier un strip incomplet comme s'il constituait une implémentation validée. Consulter les points ouverts de la roadmap.
- Inscrire les résultats, limites et références de commit dans roadmap. Mettre à jour architecture si le fonctionnement ou une décision change, et README seulement si le comportement utilisateur évolue.
- Avant livraison, auditer la liste des fichiers modifiés : aucun nouveau document hors des quatre fichiers autorisés, aucun contenu explicatif déporté dans les sous-dossiers, aucun changement de code hors du périmètre demandé.
