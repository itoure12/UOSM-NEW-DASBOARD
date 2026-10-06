# MQTT_TOPICS.md

L'accord entre toutes les cartes de la voiture sur les topics MQTT. **On ajoute une ligne
ici, on la fait valider, et seulement après on écrit le code correspondant** (règle 3 de
`Architecture.md`).

## Règles de format

- Topic en minuscules, forme `uosm/<carte>/<valeur>`.
- Contenu : **un seul nombre en texte**, point décimal (`47.02`), **sans unité**.
- Toujours dans l'unité définie par la table ci-dessous (jamais autre chose).
- QoS 0, pas de `retain`.
- Un topic inconnu ou un texte invalide côté dashboard est ignoré, jamais un crash.

## Topics (v1)

| Topic | Exemple | Unité | Carte émettrice | Fréquence | Statut |
|---|---|---|---|---|---|
| `uosm/battery/voltage` | `47.02` | V | Batterie | ? | ⚠️ à confirmer (unité interne `voltage_t` pas documentée) |
| `uosm/battery/current` | `3.15` | A | Batterie | ? | ⚠️ à confirmer (unité interne `current_t` pas documentée) |
| `uosm/motor/speed` | `24.5` | km/h | Moteur | ? | à valider |
| `uosm/motor/rpm` | `1830` | tr/min | Moteur | ? | à valider |
| `uosm/motor/temperature` | `41.3` | °C | Moteur | ? | à valider |
| `uosm/lap/efficiency` | `312.4` | km/kWh | ? | ? | à valider |
| `uosm/lap/new` | `1` | — (signal, pas une mesure) | ? | ? | à valider |

Colonnes `Fréquence` et `Carte émettrice` à remplir avec les autres équipes avant de
considérer cette version comme validée.

## Points ouverts

- **Unités internes batterie.** `voltage_t` et `current_t` sont des entiers 16 bits dont
  l'unité n'est écrite nulle part dans le code actuel. À confirmer avec la carte batterie
  pour que MQTT et CAN affichent la même valeur (voir `Architecture.md` §7).
- **Broker.** Il faut un Mosquitto qui tourne quelque part dans la voiture (sur quelle
  machine ?) plus un switch Ethernet.

## Comment tester un topic à la main

```bash
# Écouter tout le trafic dashboard
mosquitto_sub -v -t "uosm/#"

# Simuler une valeur
mosquitto_pub -t uosm/battery/voltage -m 47.02
```
