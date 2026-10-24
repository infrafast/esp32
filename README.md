# Interval Footswitch

Interval Footswitch est un contrôleur matériel programmable destiné au pilotage de consoles de mixage et d'équipements de scène. Ce dépôt contient le firmware ESP32-S3, le framework Interval, les ressources de configuration web et les outils de mise à jour.

## Présentation

Le pédalier associe trois boutons, un écran TFT et une connexion réseau Wi-Fi/Ethernet. La configuration s'effectue depuis son interface web. L'application actuellement sélectionnée intègre deux adaptateurs de console : Mixing Station et Behringer X/M Series.

Le framework comporte aussi des interfaces MIDI, OSC et socket. Leur disponibilité dépend de la configuration et de l'environnement compilé ; la présence d'une classe ne garantit pas qu'une fonction soit terminée ou validée sur scène.

## Matériel et utilisation

La cible configurée est le Seeed XIAO ESP32-S3, avec stockage LittleFS, écran ILI9341 et Ethernet W5500. Les banques et strips organisent les commandes affichées et déclenchées par les boutons.

L'application active enregistre un strip d'envoi entrée → bus. Un strip de gain est également enregistré, mais son émission de commande reste à terminer : il ne faut pas le considérer comme opérationnel.

Les ressources web sont incluses dans `data/webapp/`. Les mises à jour regroupent firmware et ressources dans des archives `.tgz` ; leur préparation et les différences entre versions de développement et de production sont décrites dans l'architecture.

## Documentation

| Document | Usage |
|---|---|
| [Architecture](docs/architecture.md) | Référence technique, patterns, ajout de strips/interfaces, compilation et audit matériel |
| [Roadmap](docs/roadmap.md) | Évolutions, problèmes ouverts, validations et suivi des demandes |
| [Consignes agent](AGENT.md) | Règles de contribution et organisation documentaire |

Ces trois documents sont les seules références de travail du projet. Le présent README reste une présentation pour les utilisateurs.

Les sous-dossiers documentaires contiennent uniquement les pièces jointes et supports de vérification, notamment les illustrations et mesures de l'audit PCB. Les explications et le suivi sont centralisés dans les documents ci-dessus.
