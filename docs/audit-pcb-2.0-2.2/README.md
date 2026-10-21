# Audit SPI et démarrage : PCB 2.0 / PCB 2.2

Audit du 21 octobre 2026, sur le code du dépôt au commit `7e9f875`, inchangé pour les fichiers étudiés au commit `78ff734`, et les deux archives Gerber fournies. Aucun changement de firmware ni de routage n'a été effectué pendant cet audit.

## Conclusion

**Je n'ai pas trouvé de différence de connexion idéale des broches SPI, des resets ou des alimentations qui suffise à démontrer la cause de la panne.** La PCB 2.2 conserve les connexions attendues par le code. En revanche, son retour de masse change de topologie et dépend d'un passage entre couches par la pastille oblongue négative de batterie. Les deux révisions partagent aussi une initialisation sensible aux états électriques de démarrage : le W5500 est interrogé avant le premier pilotage logiciel du reset partagé et du CS écran.

Le défaut décrit — W5500 non détecté sur **U7**, couleurs fausses avec **le même XIAO et le même TFT** — justifie de tester d'abord les masses réelles, l'alimentation locale, puis les signaux de reset et SPI. Les Gerbers seuls ne permettent pas de prouver lequel est fautif. Il serait prématuré de lancer une nouvelle fabrication sur la seule hypothèse d'un délai trop court.

## Périmètre et méthode

Les deux ZIP contiennent les couches cuivre, sérigraphie, masques, contour et fichiers de perçage. Ils ne contiennent ni schéma, ni nomenclature, ni modèles des composants ou empilage électrique détaillé. Les modules TFT et W5500 ont leur électronique propre, absente de ces fichiers.

J'ai décodé les deux couches cuivre et reconstruit leurs continuités géométriques avec Gerbonara/Shapely, en reliant les couches par les perçages métallisés, **y compris les trous oblongs**. Les pastilles CMS sont évaluées sur leur propre couche : une piste passant dessous sur l'autre couche ne constitue pas une connexion. Les broches sont identifiées par la sérigraphie et le brochage XIAO. Une deuxième lecture indépendante confirme les continuités principales.

Les **27 groupes de connexions nommées** comparés sont identiques ; leur inventaire est conservé dans [connexions-verifiees.json](connexions-verifiees.json). Cela couvre les points explicitement identifiés dans cet audit, sans constituer une netlist complète du produit assemblé.

Il s'agit d'une vérification de connectivité et d'une reconstitution de la chronologie logicielle, **pas d'une simulation SPICE ou d'une mesure de carte**. Les continuités supposent une fabrication conforme aux Gerbers ; elles ne prouvent pas la qualité des soudures, métallisations ou composants montés.

## 1. Ce que le code exige

La configuration effective est injectée par [prescript.py](../../prescript.py). `USER_SETUP_LOADED` fait que la configuration générique de TFT_eSPI ne détermine pas ces broches.

| Signal | XIAO | GPIO | Destination | Comparaison 2.0 / 2.2 |
|---|---|---:|---|---|
| Activation du rail périphériques | D4 | 5 | EN de l'alimentation commutée, actif HIGH | Identique |
| CS Ethernet | D5 | 6 | U7 CS, actif LOW ; également U6 CS | Identique |
| CS écran | D3 | 4 | TFT CS, actif LOW | Identique |
| Commande/données écran | D6 | 43 | TFT DC | Identique |
| Reset commun | D7 | 44 | TFT RST et W5500 RST | Identique |
| Horloge SPI | D8 | 7 | TFT CLK et U7 SCLK ; également U6 | Identique |
| MOSI | D10 | 9 | TFT MOSI et U7 MOSI ; également U6 | Identique |
| MISO | D9 | 8 | U7 MISO ; également U6 | Identique |

Le connecteur TFT représenté comporte sept broches et **ne ramène pas de MISO/SDO**. Une contention sur MISO ne doit donc pas être attribuée à cet écran sans autre connexion matérielle. Un CS écran indéterminé peut néanmoins lui faire recevoir des commandes indésirables.

Les connexions des résistances et des entrées analogiques vérifiées sont également identiques. Les différences initialement suspectées à cet endroit provenaient de croisements entre couches.

## 2. Exécution virtuelle du démarrage

Les références principales sont [HWManager.h](../../lib/Interval/src/HWManager.h), [HWComponents.h](../../lib/Interval/src/HWComponents.h), [w5100.cpp](../../lib/Ethernet/src/utility/w5100.cpp) et [TFT_eSPI.cpp](../../lib/TFT_eSPI/TFT_eSPI.cpp).

| Étape | Action effectivement exécutée | Conséquence matérielle |
|---|---|---|
| Construction et attente du bouton | Rail périphériques initialement désactivé | Les périphériques ne sont pas encore en fonctionnement normal |
| `HWManager.h:281` | GPIO5 passe HIGH | L'alimentation commutée doit monter correctement |
| `HWManager.h:285` | Initialisation des composants, réseau enregistré en premier | Le W5500 est traité avant le TFT |
| `HWComponents.h:1290` | `Ethernet.init(GPIO6)`, puis `Ethernet.begin(...)` | Préparation de la détection Ethernet |
| `w5100.cpp:101` | Attente de **560 ms**, puis configuration SPI/CS Ethernet | Il existe déjà une attente importante avant les premiers échanges |
| Détection Ethernet | Reset logiciel et tests de registres via SPI, dont `VERSIONR == 4` | Nécessite une alimentation, une masse et un reset physique déjà valides |
| `HWManager.h:287` | `tft.init(1)` configure enfin CS TFT et GPIO44 | Le matériel détermine leur état pendant la première tentative Ethernet |
| `TFT_eSPI.cpp:677` | Reset HIGH 5 ms, LOW **20 ms**, HIGH ; attente **150 ms** | Le W5500 reçoit lui aussi ce reset, après sa première initialisation |
| Configuration TFT | Écriture du format pixels et du contrôle mémoire, sortie de veille ; attente **120 ms** | Des commandes mal reçues peuvent produire un affichage incorrect |

Le code de reset matériel propre à `WiFiEth` existe, mais sa configuration initiale et son appel sont commentés (`HWComponents.h:1257` et `:1288`).

**Le reset TFT de 20 ms n'est pas trop court pour le W5500** : sa spécification exige au moins 500 µs à LOW. L'ordre d'initialisation et les états avant la première communication sont plus suspects qu'un manque de largeur de cette impulsion. Le commentaire « 500 ns » dans la méthode inutilisée ne correspond pas à la spécification. [Source : fiche constructeur W5500, §5.5.1](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf).

Cette fragilité existe sur les deux versions. Elle ne démontre donc pas la régression 2.2. De plus, la bibliothèque peut retenter la détection si elle n'a jamais réussi ; une première détection manquée ne suffit pas à expliquer une panne permanente.

À l'arrêt, le firmware place notamment le reset en `INPUT_PULLDOWN` et les CS à LOW avant de couper le rail (`HWManager.h:397`). Il faut donc comparer aussi un démarrage après extinction logicielle et un démarrage après retrait complet de l'alimentation.

## 3. Différences réelles dans le cuivre

### Retour de masse

![Comparaison des retours de masse](comparaison-masses.png)

En 2.0, une piste centrale relie directement la masse de la zone alimentation au GND inférieur de U6, vers `(38,735 ; 26,035) mm`, puis au réseau de masse de la partie droite.

En 2.2, cette liaison centrale a disparu. Le retour entre la zone XIAO/alimentation et la partie droite passe par :

`GND XIAO → CN2 → pastille oblongue batterie négative → U16/U15 → U5 → TFT/U7`.

La transition de couche sur la pastille négative de batterie, centrée vers `(1,831 ; 13,537) mm`, devient donc critique. Le fichier `Drill_PTH_Through.DRL` la déclare bien métallisée. **Une métallisation absente ou défectueuse à cet endroit peut couper la masse des périphériques en 2.2**, alors que la 2.0 dispose de la liaison centrale. Ce serait un défaut réel de fabrication à vérifier ; il n'est pas démontré par le ZIP.

En outre, aucune des deux versions ne comporte de plan de masse continu sous le bus SPI. Le retour des courants rapides se fait par des pistes éloignées, et non immédiatement sous les signaux. Le changement de leur trajet peut modifier les perturbations même si un multimètre indique une continuité.

### Longueurs et largeurs : ce qu'elles permettent de conclure

Estimations de chemins de centreligne à partir du cuivre, arrondies ; elles excluent les liaisons internes des modules, les contacts et les boîtiers.

| Trajet | 2.0 | 2.2 | Lecture |
|---|---:|---:|---|
| XIAO → U7 SCLK | 77,4 mm | 77,6 mm | Pratiquement inchangé |
| XIAO → U7 MOSI | 79,3 mm | 78,1 mm | Pratiquement inchangé |
| XIAO → U7 MISO | 88,8 mm | 85,8 mm | Un peu plus court |
| XIAO → U7 RST | 96,5 mm | 94,9 mm | Un peu plus court |
| XIAO → TFT CLK | 108,7 mm | 109,0 mm | Pratiquement inchangé |
| XIAO → TFT MOSI | 110,7 mm | 110,5 mm | Pratiquement inchangé |
| XIAO GND → U7 GND supérieur droit | 165,8 mm | 162,0 mm | Le trajet n'a pas augmenté |
| XIAO GND → U6 GND inférieur | 69,7 mm | 127,9 mm | Détour important, mais U6 n'est pas l'emplacement testé |

Les pistes SPI restent à environ **0,15 mm**. Les pistes principales d'alimentation et de masse passent d'environ **0,40 à 0,60 mm**. Une explication par des pistes de puissance devenues plus fines serait donc incorrecte. Pour U7, les seules longueurs et résistances continues ne démontrent pas une dégradation ; la topologie du retour et sa réalisation physique doivent être testées.

La bibliothèque demande 48 MHz pour le TFT et 14 MHz pour Ethernet. Avec les diviseurs du framework installé et une APB à 80 MHz, les fréquences attendues sont environ 40 MHz et 13,3 MHz ; ce sont des valeurs déduites du code, à mesurer. À ces vitesses, les fronts, les retours de masse et les dérivations vers U6 comptent autant que la longueur globale.

Le contrôle géométrique n'a pas révélé de court-circuit idéal entre les réseaux SPI identifiés. Il ne remplace pas le DRC du projet CAO ni le contrôle de la carte fabriquée.

## 4. Essais prioritaires, avec le firmware actuel

Faire un seul changement à la fois et répéter plusieurs démarrages à froid ainsi que plusieurs extinctions/réveils.

| Ordre | Essai | Résultat exploitable |
|---|---|---|
| 1 | Hors tension, mesurer GND XIAO ↔ chacun des GND U7 ↔ GND TFT. Contrôler le passage entre faces de la pastille négative batterie | Une coupure ou une résistance anormale donne une cause matérielle directe |
| 2 | Poser hors tension un fil de masse isolé, aussi direct que possible, entre XIAO et U7 ; tester, puis ajouter un retour direct vers TFT si nécessaire | Si les deux défauts disparaissent, le retour de masse est fortement impliqué. Une mesure de continuité permet de distinguer coupure et perturbations dynamiques |
| 3 | Mesurer à l'oscilloscope le 3,3 V **entre les broches locales 3V3 et GND de U7**, puis aux bornes d'alimentation du TFT, pendant tout le démarrage | Une chute ou une montée incorrecte oriente vers alimentation, commutation ou découplage |
| 4 | Capturer GPIO5/rail, RST U7, CS U7, CS TFT et SCLK, avec référence de masse locale courte | Vérifier la libération du reset avant la première trame Ethernet, les 20 ms LOW ultérieurs et les CS pendant les échanges de l'autre périphérique |
| 5 | Tester TFT débranché, puis U7 débranché, carte éteinte pour chaque changement | Si le périphérique restant retrouve un fonctionnement normal, interaction par rail, reset ou signaux à rechercher ; ce test ne distingue pas à lui seul ces causes |
| 6 | Si alimentation/reset/CS sont corrects, observer SCLK et MOSI aux récepteurs et essayer une résistance série proche de la source | Une amélioration reproductible confirme une sensibilité à la qualité des fronts |

Pour le W5500 nu, la plage VDD spécifiée est **2,97 à 3,63 V**, par rapport à son GND local. Si le module comporte un régulateur ou une autre alimentation, mesurer aussi la tension réellement appliquée au circuit. [Source : W5500, §5.3](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf).

Utiliser une connexion de sonde courte ou une mesure différentielle adaptée : les masses communes des instruments peuvent elles-mêmes créer un retour supplémentaire et masquer le défaut recherché.

Pour les fausses couleurs, vérifier si les commandes TFT de format et de contrôle mémoire sont reçues correctement : l'initialisation écrit `PIXFMT (0x3A) = 0x55`, puis `MADCTL (0x36) = 0x08` ; la rotation réécrit ensuite `MADCTL`. Une couleur incorrecte ne prouve pas que l'écran a changé de mode : un bit perdu, une commande mal reçue ou des données pixels corrompues restent possibles. Le même XIAO et le même écran rendent l'hypothèse d'une différence de contrôleur ou de configuration persistante beaucoup moins convaincante.

## 5. Corrections proposées pour une prochaine PCB compatible avec le code

1. **Reprendre la masse en priorité.** Ajouter un plan de masse continu et cousu par des vias, ou à défaut des retours directs et larges entre XIAO, alimentation commutée, U7 et TFT. Ne plus dépendre d'un unique trou oblong mécanique pour le retour des périphériques. Restaurer la liaison centrale de la 2.0 fournit aussi un second chemin ; le plan reste préférable pour les signaux SPI.

2. **Prévoir du découplage local.** Placer les emplacements près des entrées d'alimentation des modules, avec des liaisons GND courtes : typiquement 100 nF et quelques µF. Déterminer les valeurs montées après identification des modules, de l'interrupteur d'alimentation et de leurs contraintes de stabilité et de courant de démarrage. Ajouter une grosse capacité au hasard peut aussi dégrader la montée du rail.

3. **Définir les états de démarrage des CS.** Prévoir un pull-up, typiquement 10 kΩ, sur chaque CS vers le **rail 3,3 V commuté des périphériques**, sous réserve des résistances déjà présentes sur les modules. Cela conserve les sélections actives à LOW attendues par le code. Le reset doit aussi avoir un niveau de repos défini ; vérifier sa polarisation existante avant d'ajouter une résistance. Éviter de tirer un périphérique éteint vers un rail permanent.

4. **Prévoir l'amortissement du SPI.** Ajouter des emplacements pour résistances série sur SCLK et MOSI, près du XIAO. Essayer par exemple 22–47 Ω, à ajuster sur les formes d'onde ; ce n'est pas une valeur garantie par les Gerbers. Pour MISO, la source est le W5500. Des emplacements sur CS/DC sont utiles si les mesures révèlent des fronts perturbés. Le placement près de la source suit les principes de terminaison série décrits dans le [guide de conception SPI TI TIDA-060045](https://www.ti.com/lit/pdf/tiduf78).

5. **Limiter les dérivations inutilisées.** Les empreintes alternatives U6/U7 créent des branches sur le bus. Prévoir des liaisons sélectionnables ou des résistances 0 Ω permettant d'isoler la branche non montée, sans changer les GPIO du firmware. Garder une référence de masse proche des signaux.

6. **Traiter séparément le reset partagé si les captures le justifient.** Le firmware actuel remet physiquement le W5500 à zéro pendant l'initialisation TFT. Un montage distinct de reset à la mise sous tension pour le W5500 peut éviter cela, mais demande une vraie validation de son délai, de la rampe du rail et de la procédure Ethernet après reset ESP32. Ne pas installer un simple gros condensateur sur GPIO44 ni mettre en opposition un superviseur et la sortie push-pull du XIAO. Ce point reste une option de conception, pas un correctif démontré de la régression.

Les priorités 1 à 5 conservent le brochage, les polarités et le firmware actuels. Leur efficacité sur la panne doit être validée par les essais ci-dessus ; la priorité 6 demande une étude supplémentaire.

## Points de mesure

![Repères de mesure et de retouche](points-mesure.png)

Coordonnées en millimètres dans le repère Gerber, vue depuis le dessus. Les broches carrées sont des repères d'orientation ; utiliser aussi la sérigraphie, surtout en retournant la carte.

| Point | X | Y |
|---|---:|---:|
| GND XIAO | -18,875 | 23,840 |
| Reset XIAO D7 | -6,175 | 23,840 |
| GND U7 supérieur droit | 71,755 | 43,180 |
| Autres GND U7, rangée gauche | 51,435 | 43,180 et 40,640 |
| 3V3 U7, rangée droite | 71,755 | 40,640 et 38,100 |
| RST U7 | 71,755 | 33,020 |
| CS U7 | 51,435 | 33,020 |
| SCLK U7 | 51,435 | 35,560 |
| MOSI U7 | 51,435 | 38,100 |
| MISO U7 | 71,755 | 30,480 |
| GND TFT | 76,280 | 14,729 |
| Alimentation TFT | 76,280 | 17,269 |
| CLK TFT | 76,280 | 19,809 |
| MOSI TFT | 76,280 | 22,349 |
| RST TFT | 76,280 | 24,889 |
| DC TFT | 76,280 | 27,429 |
| CS TFT | 76,280 | 29,969 |
| Centre du trou oblong négatif batterie | 1,831 | 13,537 |

## Traçabilité des archives

```text
gerber 2.0.zip : 816f48444a10b10940ad6487be7759d7bb7c74b2495aaf69b204603db69626e4
gerber 2.2.zip : e54ce7f20fed21e5af8e3364e44150f06e8e662e9860993b7cdabde0eb5ed2c9
```

Avant une correction de CAO définitive, les éléments les plus utiles seraient le résultat des deux premiers essais, une capture rail/reset/CS au démarrage et les schémas/nomenclatures des modules et de l'alimentation. Ils permettraient de transformer les hypothèses électriques en correction ciblée.
