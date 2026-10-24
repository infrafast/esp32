# Roadmap et suivi des modifications

## Rôle et règles de suivi

Ce document est la référence unique des demandes, travaux ouverts, validations et résultats d'audits. L'architecture stable est décrite dans [architecture.md](architecture.md) ; les règles de contribution dans [AGENT.md](../AGENT.md).

Consolidation documentaire du 24 octobre 2026. Les anciennes cases non cochées sont conservées comme demandes historiques **à requalifier**, sans supposer que le code actuel ne les implémente pas déjà. Les pistes issues d'une analyse restent des propositions, pas des fonctionnalités commandées ou validées.

Statuts : **À qualifier**, **À faire**, **En cours**, **À valider**, **Terminé**, **Abandonné**. Chaque nouvelle demande reçoit un identifiant stable, une origine, un périmètre, des critères d'acceptation et, à sa clôture, la preuve de validation et le commit concerné. Ne pas déclarer « Terminé » sur la seule présence d'une classe.

## 1. Demande documentaire

| ID | Demande | Statut | Critères / résultat |
|---|---|---|---|
| DOC-01 | Séparer présentation utilisateur, architecture, suivi et consignes agent | Terminé — documentation | Création de README.md, docs/architecture.md, docs/roadmap.md et AGENT.md ; regroupement des trois anciens documents de projet |
| DOC-02 | Centraliser le contenu des audits et conserver les supports en sous-dossiers | Terminé — documentation | Mesures et constats PCB dans l'architecture ; essais et corrections dans cette roadmap ; supports d'origine conservés |
| DOC-03 | Vérifier l'application du modèle documentaire à chaque changement | Règle permanente | Aucun nouveau document de contenu ; mise à jour du fichier de référence concerné |

### Résultat de l'audit documentaire

- Aucun README racine, document d'architecture ou roadmap n'était présent dans l'arbre examiné.
- `ANALYSE_REPO.md` regroupait présentation, architecture, consignes et pistes de modernisation : contenu redistribué entre les quatre références.
- `lib/Interval/README.md` contenait une liste de demandes : reprise complète ci-dessous.
- `docs/audit-pcb-2.0-2.2/README.md` mélangeait constats, essais et propositions : séparation entre architecture et roadmap, avec conservation des mesures et de leurs limites.
- L'ancien ordre de démarrage n'est plus universel : `dev` active TFT avant composants ; `prod` conserve le défaut historique. Le rapport PCB est identifié comme historique.
- Les anciens documents de projet regroupés sont supprimés pour éviter deux sources concurrentes.
- Les README, changelogs et documents des dépendances distribuées, notamment sous `flash/` (esptool) et les bibliothèques tierces, restent des documents amont. Ils ne constituent pas des références de conception du projet et ne sont pas supprimés.
- Le sous-dossier PCB conserve illustrations PNG, mesures JSON et script de reproduction existant. Aucun rapport distinct n'y sera ajouté.

## 2. Fonctionnalités et points incomplets observés

| ID | Travail | Statut | Critères d'acceptation |
|---|---|---|---|
| MIX-01 | Terminer le strip de gain d'entrée | À faire | Relier lecture/écriture au contrat console, borner et convertir les valeurs, gérer les retours ; vérifier les backends supportés et la persistance |
| OSC-01 | Requalifier et terminer le strip OSC générique | À qualifier | Vérifier bibliothèque de messages, parsing, sérialisation et réception ; scénario émission/réception validé |
| MEM-01 | Auditer les réservations et liaisons des strips console | À qualifier | Absence de pointeur non initialisé/dangling, libération sur changement/suppression, interface absente/remplacée gérée ; deux strips de même clé cohérents |
| HW-01 | Comparer les deux ordres d'initialisation TFT/W5500 | À valider | Essais dev/prod, démarrages à froid et réveils, résultats Ethernet et couleurs enregistrés ; décision documentée avant généralisation |

Ces points sont dérivés du code relu pour la consolidation ; ils ne constituent pas un audit exhaustif du firmware.

## 3. Demandes historiques de la bibliothèque Interval

Origine : toutes les cases non cochées de l'ancien `lib/Interval/README.md`. Aucun achèvement n'est déduit ici.

| ID | Demande conservée | Statut | Validation attendue |
|---|---|---|---|
| INT-01 | Ajouter des templates supplémentaires avec exemples | À qualifier | Identifier les templates manquants ; exemples intégrés aux fichiers autorisés ou au code concerné |
| INT-02 | Vérifier la consommation mémoire lors de la suppression de MenuSystem | À qualifier | Mesurer créations/suppressions répétées et mémoire récupérée |
| INT-03 | Implémenter le destructeur de menuSystem, suppression de tous les MenuComponent puis ajustement de l'allocation | À qualifier | Vérifier l'ownership et l'allocateur actuel ; l'ancienne suggestion `realloc()` n'est pas une prescription sans cette vérification |
| INT-04 | Ajouter le mode AP pour le wizard | À qualifier | Définir l'écart avec l'AccessPoint existant ; valider entrée/sortie du parcours |
| INT-05 | Ajouter l'écran wizard | À qualifier | Parcours initial et retour à la configuration normale validés |
| INT-06 | Ajouter l'icône de profil | À qualifier | Affichage et sélection cohérents |
| INT-07 | Ajouter chargement et sauvegarde de profil | À qualifier | Aller-retour persistant et erreurs de lecture gérées |
| INT-08 | Rendre Label compatible float et y déplacer le code concerné d'ArcMeter | À qualifier | Affichage numérique commun, formats et valeurs limites vérifiés |
| INT-09 | Ajouter une icône aux éléments de menu | À qualifier | Rendu cohérent dans les menus concernés |
| INT-10 | Ajouter icône et label au fichier de traduction | À qualifier | Clés, repli et changement de langue vérifiés |
| INT-11 | Implémenter le texte des éléments de menu | À qualifier | Valeurs initiales et mises à jour affichées |
| INT-12 | Implémenter le feedback des éléments de menu | À qualifier | Retour d'état synchronisé avec l'action |
| INT-13 | Ajouter les fichiers multilingues chinois | À qualifier | Couverture des clés, polices et encodage vérifiés |
| INT-14 | Ajouter/compléter les fichiers multilingues espagnols | À qualifier | Un `espanol.json.gz` existe déjà ; vérifier la couverture avant de considérer cette demande terminée |

## 4. Maintenance issue de l'analyse du dépôt

Ces éléments reprennent les risques et opportunités de `ANALYSE_REPO.md`.

| ID | Proposition | Statut | Critères d'acceptation |
|---|---|---|---|
| TOOL-01 | Paramétrer les chemins source/destination des scripts | À qualifier | Plus de dépendance obligatoire à K: ou /Volumes/web/fwupdate ; conventions documentées dans l'architecture |
| TOOL-02 | Séparer packaging et déplacement de déploiement | À qualifier | Build local déterministe, effets externes explicites et optionnels |
| TOOL-03 | Prévoir une minification locale ou facultative | À qualifier | Préparation des assets possible sans service HTTP externe ; échec contrôlé |
| TOOL-04 | Éviter le lancement du workflow lors d'un simple import des scripts | À qualifier | Import sans suppression, copie ni appel réseau involontaire |
| DEP-01 | Inventorier les patchs locaux et références de dépendances | À qualifier | Origine/version/raison des adaptations décrites dans l'architecture ; mise à jour reproductible |
| CI-01 | Vérifier automatiquement les compilations dev et prod | À qualifier | Deux environnements compilés, effets de packaging maîtrisés ; aucun nouveau fichier de workflow dans la présente demande documentaire |

## 5. Audit PCB 2.0 / 2.2 — suivi des essais

Origine : audit initial du 21 octobre 2026, révision U6 du 22 octobre, code étudié `58ba13a`. Les constats, brochages, mesures, illustrations, empreintes d'archives et limites de la méthode sont conservés dans [l'architecture](architecture.md). Aucun résultat de mesure sur carte n'est fourni par cet audit documentaire.

| ID | Action | Statut | Validation attendue |
|---|---|---|---|
| PCB-01 | Contrôler continuités et soudures des trois masses U6 et du TFT | À valider | Mesures hors tension, contrôle de la métallisation de la pastille batterie |
| PCB-02 | Tester le strap GND XIAO → GND inférieur droit U6 | À valider | Même firmware, démarrages répétés ; noter séparément détection Ethernet et couleurs |
| PCB-03 | Tester le retour GND TFT si nécessaire | À valider | Comparaison avec/sans retour supplémentaire après PCB-02 |
| PCB-04 | Capturer rail local, reset, CS et SPI | À valider | Captures au démarrage et au réveil, référence de sonde adaptée |
| PCB-05 | Définir la prochaine révision de PCB à partir des essais | Proposition à valider | Retour direct/plan GND, découplage, états CS et amortissement évalués ; aucune cause présentée comme démontrée |
| PCB-06 | Étudier un reset W5500 distinct seulement si nécessaire | Proposition à valider | Chronologie et compatibilité électrique/firmware validées |

### Protocole et propositions conservés de l'audit

Le protocole ci-dessous est conservé comme proposition historique. Pour comparer les PCB, fixer et noter la valeur de `HW_INIT_TFT_BEFORE_COMPONENTS` : l'ordre TFT d'abord est désormais activé dans dev. Ne pas mélanger un changement matériel et un changement de firmware dans le même essai.

## 4. Essais prioritaires, avec le firmware actuel

Faire un seul changement à la fois et répéter plusieurs démarrages à froid ainsi que plusieurs extinctions/réveils.

| Ordre | Essai | Résultat exploitable |
|---|---|---|
| 1 | Hors tension, mesurer GND XIAO ↔ chacun des trois GND U6 ↔ GND TFT. Contrôler leurs soudures et le passage entre faces de la pastille négative batterie | Une coupure ou une résistance anormale donne une cause matérielle directe |
| 2 | Poser hors tension un fil de masse isolé, aussi direct que possible, entre **GND XIAO et GND inférieur droit U6** : points A-B sur l'illustration. Refaire les démarrages avec le même firmware | Si Ethernet redevient détectable, la liaison supprimée devient fortement impliquée ; si les couleurs reviennent aussi, une origine électrique commune est renforcée |
| 3 | Garder A-B, puis ajouter un retour GND direct XIAO → TFT (A-C) si nécessaire | Distingue une amélioration propre à U6 d'une sensibilité supplémentaire de l'écran ; noter séparément Ethernet et couleurs |
| 4 | Mesurer à l'oscilloscope le 3,3 V **entre les broches locales 3V3 et GND de U6**, puis aux bornes d'alimentation du TFT, pendant tout le démarrage | Une chute ou une montée incorrecte oriente vers alimentation, commutation ou découplage |
| 5 | Capturer GPIO5/rail, RST U6, CS U6, CS TFT et SCLK, avec référence de masse locale courte | Vérifier la libération du reset avant la première trame Ethernet, les 20 ms LOW ultérieurs et les CS pendant les échanges de l'autre périphérique |
| 6 | Si le TFT est démontable, le déconnecter hors tension puis tester U6 seul ; le W5500 étant soudé, commencer par les essais sans dessoudage | Une récupération Ethernet désigne une interaction par alimentation, reset ou SPI, sans identifier à elle seule laquelle |
| 7 | Si alimentation/reset/CS sont corrects, observer SCLK et MOSI aux récepteurs et essayer une résistance série proche de la source | Une amélioration reproductible confirme une sensibilité à la qualité des fronts |

Pour le W5500 nu, la plage VDD spécifiée est **2,97 à 3,63 V**, par rapport à son GND local. Si le module comporte un régulateur ou une autre alimentation, mesurer aussi la tension réellement appliquée au circuit. [Source : W5500, §5.3](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf).

Utiliser une connexion de sonde courte ou une mesure différentielle adaptée : les masses communes des instruments peuvent elles-mêmes créer un retour supplémentaire et masquer le défaut recherché.

Pour les fausses couleurs, vérifier si les commandes TFT de format et de contrôle mémoire sont reçues correctement : l'initialisation écrit `PIXFMT (0x3A) = 0x55`, puis `MADCTL (0x36) = 0x08` ; la rotation réécrit ensuite `MADCTL`. Une couleur incorrecte ne prouve pas que l'écran a changé de mode : un bit perdu, une commande mal reçue ou des données pixels corrompues restent possibles. Le même XIAO et le même écran rendent l'hypothèse d'une différence de contrôleur ou de configuration persistante beaucoup moins convaincante.

## 5. Corrections proposées pour une prochaine PCB compatible avec le code

1. **Rétablir un retour direct de masse vers U6 en priorité.** Relier largement la masse commune de la zone alimentation/XIAO au GND inférieur droit de U6, comme la 2.0, puis prévoir un plan de masse continu et cousu par des vias jusqu'aux périphériques. Conserver les trois GND U6 raccordés au plan et assurer un retour TFT proche du SPI. Ne plus dépendre d'un unique trou oblong mécanique pour le retour des périphériques. Le strap A-B doit idéalement confirmer cette piste avant une nouvelle fabrication.

2. **Prévoir du découplage local.** Placer les emplacements près des entrées d'alimentation des modules, avec des liaisons GND courtes : typiquement 100 nF et quelques µF. Déterminer les valeurs montées après identification des modules, de l'interrupteur d'alimentation et de leurs contraintes de stabilité et de courant de démarrage. Ajouter une grosse capacité au hasard peut aussi dégrader la montée du rail.

3. **Définir les états de démarrage des CS.** Prévoir un pull-up, typiquement 10 kΩ, sur chaque CS vers le **rail 3,3 V commuté des périphériques**, sous réserve des résistances déjà présentes sur les modules. Cela conserve les sélections actives à LOW attendues par le code. Le reset doit aussi avoir un niveau de repos défini ; vérifier sa polarisation existante avant d'ajouter une résistance. Éviter de tirer un périphérique éteint vers un rail permanent.

4. **Prévoir l'amortissement du SPI.** Ajouter des emplacements pour résistances série sur SCLK et MOSI, près du XIAO. Essayer par exemple 22–47 Ω, à ajuster sur les formes d'onde ; ce n'est pas une valeur garantie par les Gerbers. Pour MISO, la source est le W5500. Des emplacements sur CS/DC sont utiles si les mesures révèlent des fronts perturbés. Le placement près de la source suit les principes de terminaison série décrits dans le [guide de conception SPI TI TIDA-060045](https://www.ti.com/lit/pdf/tiduf78).

5. **Limiter les dérivations inutilisées vers U7.** Les empreintes alternatives U6/U7 créent des branches sur le bus. Prévoir des liaisons sélectionnables ou des résistances 0 Ω permettant d'isoler l'empreinte non montée, sans changer les GPIO du firmware. Sur cette carte, il faut conserver U6 et l'écran. Garder une référence de masse proche des signaux.

6. **Traiter séparément le reset partagé si les captures le justifient.** Le firmware actuel remet physiquement le W5500 à zéro pendant l'initialisation TFT. Un montage distinct de reset à la mise sous tension pour le W5500 peut éviter cela, mais demande une vraie validation de son délai, de la rampe du rail et de la procédure Ethernet après reset ESP32. Ne pas installer un simple gros condensateur sur GPIO44 ni mettre en opposition un superviseur et la sortie push-pull du XIAO. Ce point reste une option de conception, pas un correctif démontré de la régression.

Les priorités 1 à 5 conservent le brochage, les polarités et le firmware actuels. Leur efficacité sur la panne doit être validée par les essais ci-dessus ; la priorité 6 demande une étude supplémentaire.


## 6. Journal des validations

### 24 octobre 2026 — consolidation documentaire

Lecture des documents de projet et des points d'intégration du code : application active, interfaces, strips, datasets, consoles, réservations, options de compilation et ordre matériel. Vérification documentaire des chemins, de la répartition et de la conservation du contenu de l'audit PCB.

Cette intervention ne modifie ni firmware, ni routage PCB, ni bibliothèque tierce. Aucune compilation ni mesure matérielle n'a été exécutée pour cette réorganisation. Les fonctionnalités et corrections ouvertes ci-dessus ne sont donc pas déclarées validées.
