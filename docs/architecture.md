# Architecture d'Interval Footswitch

## Rôle et état de référence

Ce document est la référence architecturale du projet : structure, responsabilités, patterns et procédure d'extension. Les travaux demandés, hypothèses à tester et critères de validation sont suivis dans [la roadmap](roadmap.md). Les consignes de contribution sont dans [AGENT.md](../AGENT.md).

Consolidation du 24 octobre 2026 à partir du code de `main`, de `ANALYSE_REPO.md`, de `lib/Interval/README.md` et de l'audit PCB révisé le 22 octobre. Les constats historiques de l'audit sont datés et ne remplacent pas l'état actuel du code.

## 1. Structure et responsabilités

| Emplacement | Responsabilité |
|---|---|
| `src/main.cpp` | Sélection de l'application active : `MixerApplication.h` |
| `src/MixerApplication.h` | Activation réseau, création des consoles, déclaration des strips, `setup()/loop()` |
| `src/TestApplication.h` | Application de test alternative, non sélectionnée |
| `lib/Interval/src/Interval.h` | Intégration du framework, utilitaires, événements, configuration et ressources communes |
| `APPManager.h`, `Banks.h` | Orchestration applicative et organisation des banques |
| `HWManager.h`, `HWComponents.h` | Rail d'alimentation, boutons, batterie, réseau, écran, modes USB et arrêt |
| `GUIManager.h`, `GUIComponents.h` | Composition et rendu de l'interface locale |
| `Interfaces.h`, `interfaces/` | Contrats et transports des périphériques |
| `DataManagement.h` | Adresses, datasets, dépôts et sérialisation des paramètres |
| `Strips.h`, `strips/` | Modèles de strips, fabrique et implémentations réutilisables |
| `src/mixers/` | Abstraction console, adaptateurs, valeurs partagées et strips métier |
| `data/webapp/` | Ressources web compressées, traductions et bibliothèques de messages |
| `platformio.ini`, scripts Python racine | Compilation, configuration TFT et préparation des mises à jour |

Les noms sans chemin dans cette table désignent `lib/Interval/src/`.

## 2. Cycle applicatif

`setup()` crée `MixingStation` et `XMSeries`, enregistre les squelettes `input2mixbus` et `inputGain`, puis appelle `app.init()`. `loop()` appelle `app.refresh()`.

Les nouvelles fonctionnalités doivent s'intégrer dans ce cycle : configuration déclarative au démarrage, traitement périodique borné dans les méthodes de surveillance et événements. Ne pas ajouter une boucle d'attente réseau bloquante au traitement courant.

Le matériel possède son propre cycle : activation du rail, initialisation de l'écran et des composants, montage LittleFS, sélection du mode USB, puis surveillance. L'arrêt modifie aussi les niveaux des broches avant coupure du rail ; les essais doivent couvrir démarrage à froid et réveil après arrêt logiciel.

### Ordre TFT / W5500 actuel

`HW_INIT_TFT_BEFORE_COMPONENTS` accepte 0 ou 1 dans `HWManager.h` :

- 1 : `initTFT()` avant l'initialisation des composants, donc avant le réseau.
- 0 : composants avant `initTFT()`, ordre historique.
- `env:dev` définit explicitement 1.
- `env:prod` ne définit pas cette option et utilise actuellement la valeur par défaut 0.

Le reset TFT et W5500 est partagé. Le changement d'ordre en développement est une variante de diagnostic présente dans le code, pas une preuve de résolution matérielle ni une validation de production.

## 3. Patterns et contrats

### Interfaces et adaptateurs

`DeviceInterface` combine `SerializableParameters`, `EventDispatcher` et `EventListener`. Ses instances s'inscrivent dans `interfaceRepo`. Le contrat couvre notamment `begin()`, `end()`, `monitor()`, la sérialisation/restauration des propriétés et les transitions d'état. Les fonctions `_begin()` et `update()` assurent une partie du cycle commun : préserver leur comportement lors d'une surcharge.

`NetworkDevice` ajoute réseau, ports, événements de connexion et surveillance de présence. Les transports concrets comprennent `NetworkOSC` et `NetworkSocket`. Les états incluent ready, disconnected, pending, error, disabled, offline et syncing ; utiliser les mécanismes existants plutôt qu'un booléen parallèle.

### Abstraction console et composition par templates

`MixerBase` expose les opérations métier indépendantes du protocole, dont demandes et écritures des niveaux/mutes de bus et d'envois. `MixerDevice<NetworkBase>` compose ce contrat avec un transport :

- `MixingStation : MixerDevice<NetworkSocket>` ;
- `XMSeries : MixerDevice<NetworkOSC>`.

Le constructeur de `MixerDevice` associe explicitement son `DeviceInterface*` à son `MixerBase*`. `MixerBase::fromDeviceInterface()` résout cette association. Éviter les conversions supposant que toutes les interfaces sont une console concrète donnée.

### Fabrique de strips et composition de comportements

`StripSkeleton` décrit un type, un groupe, un constructeur et les adresses de propriétés. Sa construction l'enregistre dans `stripFactory`. La fabrique reconstruit les instances à partir de leur identifiant ; cet identifiant fait donc partie du format persistant.

`Strip` fournit les événements, le rendu et le comportement commun. `OneSelectStrip` et `TwoSelectStrip` ajoutent des sélections alimentées par datasets. `StripMixerMixin<BaseStripT>` compose un strip visuel avec la logique console.

Dans `s_mixer.h`, la visibilité déclenche les demandes de valeurs et le focus les demandes de configuration. Les rafraîchissements périodiques utilisent `STRIP_REFRESH_VISIBLE` et vérifient l'état `IF_READY`. Une interface peut être assignée après construction ou remplacée : la liaison console doit être résolue à nouveau si elle change.

### Événements, datasets et paramètres

Le framework utilise un mécanisme observateur (`EventDispatcher/EventListener`) pour découpler producteurs et consommateurs. Un pointeur attaché à un événement n'implique pas un transfert de propriété ; ne pas le conserver au-delà de sa durée de validité.

`DataSet` inscrit ses valeurs dans un `DataRepository` et se désinscrit à sa destruction. `DynamicDataSet` délègue à un `DataProvider`. `SerializableParameters` décrit les adresses et peut créer les datasets de configuration associés. `dataRepo` sert notamment aux squelettes ; `IFdataRepo` sépare les paramètres des interfaces.

Les consoles utilisent `DataSetSynced` et `Synchronizer` pour récupérer les noms et propager leurs changements à l'application web. Les capacités maximales du code ne doivent pas être confondues avec celles du modèle de console détecté. Vérifier les conversions entre indices internes et adresses du protocole, souvent numérotées à partir de 1.

### Valeurs partagées et durée de vie

`values.h` fournit `OneIndexValueList` et `TwoIndexValueList`, reposant sur des réservations. Deux strips visant la même clé reçoivent le même objet : niveaux et mutes restent cohérents par pointeurs partagés.

Chaque `reserve(...)` doit avoir son `unreserve(...)` correspondant. Le dernier retrait détruit l'objet ; un pointeur restant devient invalide. Libérer l'ancienne réservation lors d'un changement de sélection et à la destruction, initialiser les pointeurs, vérifier les liaisons nulles. Cette règle décrit le contrat à respecter : les strips existants ne sont pas tous une implémentation complète et sûre de ce contrat.

## 4. Ajouter une fonctionnalité

### Nouveau strip

1. Décrire la demande et ses critères dans la roadmap, en indiquant si le backend est déjà disponible.
2. Choisir `Strip`, `OneSelectStrip` ou `TwoSelectStrip`. Pour une console, réutiliser `StripMixerMixin`.
3. Définir un identifiant stable et implémenter les méthodes abstraites : action, sérialisation/restauration des propriétés, focus, visibilité et labels ; composer les widgets adaptés aux formes et au zoom.
4. Créer le callback constructeur, puis le `StripSkeleton` et enregistrer ses adresses dans l'ordre attendu par les sélections. Suivre le montage de `src/MixerApplication.h`.
5. Pour une console, implémenter `getConfig()`, `getValues()` et les callbacks d'augmentation/diminution/action via `MixerBase`. Réutiliser les valeurs partagées et les bornes/conversions du domaine.
6. Préserver le cycle d'inscription aux événements. Si `init()` est surchargée, examiner ce que fait la classe de base et ne pas perdre ses effets utiles.
7. Vérifier création, sauvegarde/rechargement, changement de sélection/interface, déconnexion/reconnexion, visibilité/focus, formes graphiques, suppression et cohérence entre deux strips de même clé.

`Input2MixbusStrip` illustre les requêtes et setters. `inputGainStrip::setInputGain()` contient encore une implémentation commentée : ne pas copier ce squelette en le considérant terminé. Le strip OSC générique comporte aussi du code incomplet, à examiner avant réutilisation.

### Nouvelle interface ou nouvelle console

1. Séparer le transport de la logique métier. Étendre `DeviceInterface` ou un transport existant ; pour une console, suivre `MixerDevice<Transport>`.
2. Choisir un nom et un groupe stables ; exposer les propriétés par `Address` et `registerDataSetAddress()`.
3. Implémenter démarrage, arrêt et surveillance ; gérer perte réseau, reconnexion et changements de propriétés sans bloquer `app.refresh()`.
4. Définir sérialisation, restauration, états et événements. Préserver l'inscription/désinscription et les budgets de services réseau existants.
5. Pour une console, respecter `MixerBase`, les couples request/set, les datasets de noms, la découverte/identification et les limites du modèle. Répercuter les écritures et retours dans les valeurs partagées.
6. Instancier l'interface dans l'application active et vérifier son exposition à la configuration web.
7. Valider entrées invalides, limites d'indices, mode hors ligne, synchronisation et compatibilité des configurations sauvegardées.

### Mémoire et robustesse

Employer des buffers bornés dans les chemins de parsing/transport, vérifier les tailles et résultats d'allocation, documenter qui possède les objets. Le code mélange `std::vector`, LinkedList et allocations dynamiques : conserver les contrats existants et mesurer avant une migration de conteneur. Une refonte générale de mémoire ne fait pas partie de cette consolidation documentaire.

## 5. Construction, dépendances et ressources

La référence des options de build est `platformio.ini` : `espressif32@6.4.0`, Arduino, Seeed XIAO ESP32-S3, LittleFS, console 115200 bauds.

| Environnement | Particularités |
|---|---|
| `dev` | DEBUG, ordre TFT avant composants, scripts pre/post et production d'archives |
| `prod` | release, PROD=1, USB OTG/TinyUSB avec CDC au démarrage désactivé, script pre TFT uniquement |

Commandes de compilation : `pio run -e dev` et `pio run -e prod`. La compilation dev déclenche des effets sur les fichiers et une destination externe : inspecter les scripts avant exécution. Une compilation réussie ne prouve pas le fonctionnement matériel.

`prescript.py` injecte `USER_SETUP_LOADED`, le pilote `ILI9341_2_DRIVER`, les broches TFT, les transactions SPI et une fréquence demandée de 48 MHz. Les réglages génériques de TFT_eSPI ne sont donc pas la référence effective de ce montage.

Les dépendances combinent registre PlatformIO, URL Git et bibliothèques embarquées localement. Certaines références sont épinglées, d'autres non. Ethernet, mWebSockets et TFT_eSPI contiennent des adaptations ou réglages locaux : ne pas les remplacer aveuglément. `Interfaces_LEGACY.h` est un fichier historique, pas le modèle conseillé pour de nouvelles interfaces.

### Web et mise à jour

`data/webapp/` contient principalement des ressources gzip, traductions, polices et images. `preparefiles.py` et sa variante macOS assurent la préparation des assets. L'analyse initiale relève un chemin source `K:`, un helper Windows, des appels aux services de minification Toptal et un déclenchement de workflow à l'import. Ces contraintes de portabilité sont suivies dans la roadmap ; le code source web éditable n'est pas à présumer disponible à partir des seuls fichiers compressés.

`gzfw_to_data.py` compresse le firmware dans `data/`. `make_tgz.py` utilise la version `_V`, archive `data/` et déplace l'archive ainsi que le firmware compressé vers `/Volumes/web/fwupdate`, en créant le dossier si nécessaire. Le build dev couple donc compilation et préparation de déploiement. Le build prod ne déclenche pas ces deux post-scripts.

Les constantes du framework indiquent `infrafast.fr`, `/fwupdate/`, `/webapp` et l'extension `.tgz`. Ne pas déduire le protocole HTTP/TLS de ces seuls noms.

Les zones de fort impact sont `Interval.h`, `src/mixers/`, `platformio.ini` et les scripts de préparation : vérifier leurs effets transversaux avant modification.

## 6. Audit matériel PCB 2.0 / 2.2 — référence historique

Le rapport ci-dessous conserve les mesures, preuves et limites de l'audit du 21 octobre, révisé le 22 octobre 2026, sur le code `58ba13a`. Sa chronologie d'initialisation est historique : voir l'ordre actuel en section 2. Les essais et corrections proposées ont été déplacés dans la roadmap ; ils ne sont pas déclarés réalisés.


Audit initial du 21 octobre 2026, **révisé le 22 octobre après confirmation que le W5500 est soudé sur U6, et non U7**. Code étudié au commit `58ba13a`, inchangé pour les fichiers d'initialisation depuis l'audit initial, et mêmes archives Gerber. Aucun changement de firmware ni de routage n'a été effectué pendant cet audit.

## Conclusion

**Avec le W5500 sur U6, la suppression de la liaison centrale de masse devient le changement matériel à tester en priorité.** Le trajet du cuivre du support entre GND XIAO et GND inférieur de U6 passe de **69,7 à 127,9 mm**, alors que les pistes SPI vers U6 changent très peu. Un fil direct vers ce GND U6 reproduit la fonction de la liaison retirée en 2.2, sans modifier le firmware.

Cela renforce une hypothèse de retour de masse perturbé ou défectueux, **sans démontrer la cause de la panne** : les pistes de masse sont plus larges en 2.2 et le module relie normalement ses trois GND en interne. Les connexions idéales SPI, reset et alimentation restent compatibles avec le code. Les deux révisions partagent aussi une initialisation sensible aux états de démarrage : le W5500 est interrogé avant le premier pilotage logiciel du reset partagé et du CS écran.

Le défaut décrit est donc : W5500 non détecté sur **U6**, couleurs fausses avec **le même XIAO et le même TFT**. L'identité du module W5500 essayé sur les deux PCB n'est pas établie ; son état et ses soudures restent à vérifier. Le contexte U7 du rapport initial est remplacé par U6 dans les calculs, essais et illustrations ci-dessous.

## Périmètre et méthode

Les deux ZIP contiennent les couches cuivre, sérigraphie, masques, contour et fichiers de perçage. Ils ne contiennent ni schéma, ni nomenclature, ni modèles des composants ou empilage électrique détaillé. Les modules TFT et W5500 ont leur électronique propre, absente de ces fichiers.

J'ai décodé les deux couches cuivre et reconstruit leurs continuités géométriques avec Gerbonara/Shapely, en reliant les couches par les perçages métallisés, **y compris les trous oblongs**. Les pastilles CMS sont évaluées sur leur propre couche : une piste passant dessous sur l'autre couche ne constitue pas une connexion. Les broches sont identifiées par la sérigraphie et le brochage XIAO. Une deuxième lecture indépendante confirme les continuités principales.

Les **27 groupes de connexions nommées** comparés sont identiques ; leur inventaire est conservé dans [connexions-verifiees.json](audit-pcb-2.0-2.2/connexions-verifiees.json). Cela couvre les points explicitement identifiés dans cet audit, sans constituer une netlist complète du produit assemblé.

La révision U6 reconstruit à nouveau les continuités critiques et les trajets à partir des deux ZIP. Le script [analyse_gerbers.py](audit-pcb-2.0-2.2/analyse_gerbers.py) régénère les illustrations et [mesures-u6.json](audit-pcb-2.0-2.2/mesures-u6.json), avec les empreintes SHA-256 des archives. Un second modèle relie idéalement les trois GND de U6 pour distinguer les chemins du support seul de ceux rendus possibles par le module ; il ne modélise ni ses résistances internes ni ses inductances.

Il s'agit d'une vérification de connectivité et d'une reconstitution de la chronologie logicielle, **pas d'une simulation SPICE ou d'une mesure de carte**. Les continuités supposent une fabrication conforme aux Gerbers ; elles ne prouvent pas la qualité des soudures, métallisations ou composants montés.

## 1. Ce que le code exige

La configuration effective est injectée par [prescript.py](../prescript.py). `USER_SETUP_LOADED` fait que la configuration générique de TFT_eSPI ne détermine pas ces broches.

| Signal | XIAO | GPIO | Destination | Comparaison 2.0 / 2.2 |
|---|---|---:|---|---|
| Activation du rail périphériques | D4 | 5 | EN de l'alimentation commutée, actif HIGH | Identique |
| CS Ethernet | D5 | 6 | U6 CS, actif LOW ; également U7 CS | Identique |
| CS écran | D3 | 4 | TFT CS, actif LOW | Identique |
| Commande/données écran | D6 | 43 | TFT DC | Identique |
| Reset commun | D7 | 44 | TFT RST et W5500 RST | Identique |
| Horloge SPI | D8 | 7 | TFT CLK et U6 SCLK ; également U7 | Identique |
| MOSI | D10 | 9 | TFT MOSI et U6 MOSI ; également U7 | Identique |
| MISO | D9 | 8 | U6 MISO ; également U7 | Identique |

Le connecteur TFT représenté comporte sept broches et **ne ramène pas de MISO/SDO**. Une contention sur MISO ne doit donc pas être attribuée à cet écran sans autre connexion matérielle. Un CS écran indéterminé peut néanmoins lui faire recevoir des commandes indésirables.

Les connexions des résistances et des entrées analogiques vérifiées sont également identiques. Les différences initialement suspectées à cet endroit provenaient de croisements entre couches.

## 2. Exécution virtuelle du démarrage

Les références principales sont [HWManager.h](../lib/Interval/src/HWManager.h), [HWComponents.h](../lib/Interval/src/HWComponents.h), [w5100.cpp](../lib/Ethernet/src/utility/w5100.cpp) et [TFT_eSPI.cpp](../lib/TFT_eSPI/TFT_eSPI.cpp).

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

![Comparaison des retours de masse](audit-pcb-2.0-2.2/comparaison-masses.png)

En 2.0, une piste centrale relie directement la masse de la zone alimentation au GND inférieur de U6, vers `(38,735 ; 26,035) mm`, puis au réseau de masse de la partie droite.

En 2.2, cette liaison centrale a disparu. Le retour entre la zone XIAO/alimentation et la partie droite passe par :

`GND XIAO → CN2 → pastille oblongue batterie négative → U16/U15 → U5 → GND inférieur U6`.

Le GND inférieur de U6 se trouve désormais au bout d'une branche remontant depuis U5, au lieu d'être relié directement à la zone alimentation. Le réseau de masse se prolonge aussi vers TFT/U7 et les GND supérieurs de U6.

La transition de couche sur la pastille négative de batterie, centrée vers `(1,831 ; 13,537) mm`, devient donc critique. Le fichier `Drill_PTH_Through.DRL` la déclare bien métallisée. **Une métallisation absente ou défectueuse à cet endroit peut couper la masse des périphériques en 2.2**, alors que la 2.0 dispose de la liaison centrale. Ce serait un défaut réel de fabrication à vérifier ; il n'est pas démontré par le ZIP.

En outre, aucune des deux versions ne comporte de plan de masse continu sous le bus SPI. Le retour des courants rapides se fait par des pistes éloignées, et non immédiatement sous les signaux. Le changement de leur trajet peut modifier les perturbations même si un multimètre indique une continuité.

### Longueurs et largeurs : ce qu'elles permettent de conclure

Estimations de chemins de centreligne à partir du cuivre, arrondies ; elles excluent les liaisons internes des modules, les contacts et les boîtiers.

| Trajet | 2.0 | 2.2 | Lecture |
|---|---:|---:|---|
| XIAO → U6 CS | 39,9 mm | 39,9 mm | Inchangé |
| XIAO → U6 SCLK | 52,6 mm | 52,8 mm | Pratiquement inchangé |
| XIAO → U6 MOSI | 58,1 mm | 56,9 mm | Pratiquement inchangé |
| XIAO → U6 MISO | 39,8 mm | 38,2 mm | Un peu plus court |
| XIAO → U6 RST | 36,1 mm | 35,8 mm | Pratiquement inchangé |
| Sortie alimentation commutée → 3V3 U6 | 33,0 mm | 36,7 mm | Un peu plus long, mais plus large |
| XIAO → TFT CLK | 108,7 mm | 109,0 mm | Pratiquement inchangé |
| XIAO → TFT MOSI | 110,7 mm | 110,5 mm | Pratiquement inchangé |
| XIAO GND → U6 GND inférieur | **69,7 mm** | **127,9 mm** | **+58,2 mm, soit environ +84 % sur le support seul** |
| XIAO GND → U6 GND supérieur gauche | 202,7 mm | 198,9 mm | Support seul, avant les ponts internes du module |
| XIAO GND → U6 GND supérieur droit | 200,1 mm | 196,3 mm | Support seul, avant les ponts internes du module |
| XIAO GND → TFT GND | 132,8 mm | 129,0 mm | Pas d'allongement global vers l'écran |

Les pistes SPI restent à environ **0,15 mm**. Les pistes principales d'alimentation et de masse passent d'environ **0,40 à 0,60 mm**. Une explication par des pistes de puissance devenues plus fines serait donc incorrecte.

**Le module U6 change le réseau de masse de la carte assemblée.** Ses GND1, GND2 et GND3 sont normalement communs sur son propre circuit. Les deux trajets de plus de 190 mm du tableau ne sont donc pas les seuls chemins réels : le GND inférieur fournit aussi un accès aux broches supérieures via le module. En reliant idéalement ces trois broches dans le modèle, le plus court chemin de cuivre du support vers leur réseau commun reste de 69,7 mm en 2.0 contre 127,9 mm en 2.2. Cette abstraction ne décrit pas le partage des courants ni le comportement des fronts SPI ; vérifier la continuité réelle entre les trois GND du module et leurs soudures.

L'allongement de 84 % ne signifie pas une résistance effective accrue de 84 %. À titre d'illustration, pour une même épaisseur de cuivre et un trajet uniformément élargi de 0,4 à 0,6 mm, la résistance de cette branche n'augmenterait que d'environ 22 %, avant les chemins parallèles. Cela ne suffit pas à expliquer à soi seul une chute importante de la tension 3,3 V. La géométrie des boucles et la qualité des contacts restent les hypothèses à vérifier.

La bibliothèque demande 48 MHz pour le TFT et 14 MHz pour Ethernet. Avec les diviseurs du framework installé et une APB à 80 MHz, les fréquences attendues sont environ 40 MHz et 13,3 MHz ; ce sont des valeurs déduites du code, à mesurer. À ces vitesses, les fronts, les retours de masse et les dérivations vers U7 non monté comptent autant que la longueur globale.

Le contrôle géométrique n'a pas révélé de court-circuit idéal entre les réseaux SPI identifiés. Il ne remplace pas le DRC du projet CAO ni le contrôle de la carte fabriquée.


Les essais et propositions de correction figurent dans [la roadmap](roadmap.md).

## Points de mesure

![Repères de mesure et de retouche](audit-pcb-2.0-2.2/points-mesure.png)

Coordonnées en millimètres dans le repère Gerber, vue depuis le dessus. Les broches carrées sont des repères d'orientation ; utiliser aussi la sérigraphie, surtout en retournant la carte.

| Point | X | Y |
|---|---:|---:|
| GND XIAO | -18,875 | 23,840 |
| Reset XIAO D7 | -6,175 | 23,840 |
| **GND U6 inférieur droit — point B du strap** | **38,735** | **26,035** |
| GND U6 supérieur gauche | 36,195 | 46,355 |
| GND U6 supérieur droit | 38,735 | 46,355 |
| 3V3 U6, rangée inférieure | 33,655 et 36,195 | 26,035 |
| RST U6, rangée inférieure | 28,575 | 26,035 |
| CS U6, rangée supérieure | 28,575 | 46,355 |
| SCLK U6, rangée supérieure | 31,115 | 46,355 |
| MOSI U6, rangée supérieure | 33,655 | 46,355 |
| MISO U6, rangée inférieure | 26,035 | 26,035 |
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
