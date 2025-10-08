# Analyse du dépôt ESP32

## Aperçu général
- Projet PlatformIO ciblant la carte Seeed XIAO ESP32-S3 et utilisant le framework Arduino avec un système de fichiers LittleFS pour embarquer des ressources web et des polices personnalisées.【F:platformio.ini†L1-L56】
- L'application principale s'appuie sur une architecture modulaire centrée sur un serveur de mixage audio, des interfaces réseau et un rendu graphique via des polices OpenFontRender.【F:src/FullApplication.h†L1-L120】【F:src/FullApplication.h†L121-L192】

## Organisation du code
- `src/TestApplication.h` fournit un point d'entrée minimal qui initialise l'application complète et rafraîchit la logique métier dans la boucle principale, activant les composants réseau via le drapeau `USE_NETWORK`.【F:src/TestApplication.h†L1-L18】
- `src/FullApplication.h` déclare les structures centrales :
  - Gestion du serveur de mixage (`MixerServer`) et suivi d'état des interfaces connectées.【F:src/FullApplication.h†L121-L192】
  - Typographie personnalisée pour les icônes (`MyIcon`) et gestion de progression pour les synchronisations réseau.【F:src/FullApplication.h†L93-L151】【F:src/FullApplication.h†L152-L192】
- `preparefiles.py` automatise la minification et la compression des ressources web avant téléversement sur le filesystem embarqué, en s'appuyant sur des services web pour minifier CSS/JS/HTML et sur `gzip` pour compresser les fichiers sélectionnés.【F:preparefiles.py†L1-L132】【F:preparefiles.py†L133-L208】

## Dépendances et configuration PlatformIO
- Sélectionne explicitement la version `espressif32@6.4.0` (correspondant à Arduino-ESP32 2.0.11) afin de garantir la compatibilité avec l'interface Ethernet et TinyUSB, avec de nombreux commentaires pour documenter les essais de versions alternatives.【F:platformio.ini†L9-L40】
- Liste des bibliothèques externes majeures : AppleMIDI/USBMIDI pour la connectivité musicale, EspTinyUSB pour la pile USB, OpenFontRender/TJpg_Decoder pour l'affichage, aWOT/HttpClient pour le web, ESP32Ping et ESP-FTP-Server-Lib pour les communications réseau.【F:platformio.ini†L33-L74】
- Deux environnements PlatformIO (`dev` et `prod`) permettent d'activer des drapeaux de compilation distincts, dont `DEBUG` en développement et la désactivation des CDC USB en production.【F:platformio.ini†L76-L96】

## Scripts et utilitaires
- `preparefiles.py` propose également un workflow complet (`workflow()`) qui purge le dossier de destination, minifie les ressources et prépare les fichiers LittleFS en compressant automatiquement les extensions cibles tout en respectant des exceptions configurables.【F:preparefiles.py†L168-L213】
- `map_folder.bat` (appelé depuis `prep_data_folder`) est intégré dans le pipeline pour synchroniser les ressources depuis un lecteur réseau ou externe avant la compression finale.【F:preparefiles.py†L133-L167】

## Pistes de vérification future
- Documenter ou ajouter des tests automatisés pour vérifier la disponibilité des services de minification externes utilisés par `preparefiles.py`.
- Consolider l'initialisation et la gestion mémoire des composants graphiques (ex. `StripSkeleton`) mentionnée dans les commentaires de `FullApplication.h` pour prévenir les crashs liés aux pointeurs nuls.【F:src/FullApplication.h†L1-L32】

