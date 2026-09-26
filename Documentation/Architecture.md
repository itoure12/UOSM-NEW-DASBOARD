# Architecture du dashboard

Ce document explique comment le dashboard est organisé et pourquoi.
Lisez-le avant d'ouvrir le code : il suffit pour savoir où chercher.

> **Statut :** ce document décrit l'architecture **cible**. La migration est en cours.
> La section [Où sont les fichiers aujourd'hui](#où-sont-les-fichiers-aujourdhui) fait le lien avec le code actuel.

---

## 1. Le principe en une phrase

Une valeur (ex. la tension batterie) suit toujours le même trajet :
**elle arrive** (MQTT ou CAN) → **elle est décodée** → **elle est stockée avec son heure** → **l'écran l'affiche**.
Chaque étape vit dans son propre dossier, et **seul `board/` connaît le matériel**.

```
MQTT (défaut) : broker ──► mqtt_client ──► mqtt_topics ──► mqtt_decode ─┐
                                                                         ├──► vehicle_state ──► ui_task ──► écran
CAN (secours) : MCP2515 ──► UOSM-Core ──► can_callbacks ────────────────┘      (+ heure)       (20×/s)
```

---

## 2. Décisions

| Décision | Choix | Pourquoi |
|---|---|---|
| Transport principal | **Ethernet + MQTT** | Décision équipe : toute la voiture passe à l'Ethernet |
| Format MQTT | **Un topic par valeur, en texte** (`uosm/battery/voltage` → `47.02`) | Lisible avec `mosquitto_sub`, simple à produire sur un STM32 (`snprintf`) |
| CAN | **Gardé en secours, inchangé** | Plan B si l'Ethernet pose problème. Choisi à la compilation |
| UOSM-Core | **Pas modifié** | Partagé avec les autres cartes. Tout le MQTT vit dans le dashboard |
| Portabilité | **Toute la dépendance matérielle dans `board/`** | Nouveau board = réécrire ~5 fichiers. Permet aussi le simulateur PC et les tests |
| Mise à jour de l'écran | **L'UI lit `vehicle_state` 20×/s** | Une seule tâche touche LVGL → plus de mutex LVGL ni de crash aléatoire |
| Valeurs périmées | **Chaque valeur a son heure de réception** | L'écran affiche `--` au lieu d'un chiffre figé si une carte ne répond plus |

---

## 3. L'arbre cible

```
UOSM-Dashboard/
├── CMakeLists.txt
├── CMakePresets.json                 "disco_mqtt" (défaut), "disco_can", "simulator"
│
├── config/
│   ├── dashboard_config.h            rafraîchissement UI, timeout "valeur périmée"
│   ├── comms_config.h                TRANSPORT = MQTT | CAN, IP du broker, port
│   ├── lv_conf.h, FreeRTOSConfig.h
│   └── UOSMCoreConfig.h/.c           config du CAN pour UOSM-Core (inchangée)
│
├── app/                              code portable : AUCUN header STM32 / HAL
│   ├── app_main.cpp                  crée les tâches selon TRANSPORT
│   ├── comms/
│   │   ├── mqtt/
│   │   │   ├── mqtt_client.c/.h      connexion au broker, abonnement "uosm/#", reconnexion
│   │   │   ├── mqtt_topics.c/.h      LA table : topic → fonction de décodage
│   │   │   └── mqtt_decode.c/.h      texte → nombre → Set...() dans vehicle_state
│   │   └── can/
│   │       ├── can_task.c/.h         lecture CAN (code actuel, inchangé)
│   │       └── can_callbacks.c/.h    décodage CAN (code actuel, inchangé)
│   ├── vehicle_state/
│   │   ├── DataAggregator.hpp        dernières valeurs + heure de réception + is_stale()
│   │   └── DataAggregatorWrapper.*   API C : SetBatteryVoltage(), SetSpeed()…
│   └── ui/
│       ├── ui_task.cpp               seule tâche qui touche LVGL
│       ├── application.cpp/.h
│       ├── screens/                  HomeView, DebugView
│       ├── widgets/                  View, Cards
│       └── assets/                   polices
│
├── board/
│   ├── board.h                       le contrat matériel (voir §5)
│   ├── stm32f769_disco/
│   │   ├── UOSM-Dashboard.ioc        CubeMX (ETH + LwIP activés)
│   │   ├── Core/, Drivers/, Middlewares/, LWIP/   code généré par CubeMX, enfermé ici
│   │   ├── board_init.c              horloges, MPU, réseau
│   │   ├── display.c, touch.c        écran et tactile ↔ LVGL
│   │   └── time.c, log.c
│   └── simulator/                    même contrat, sur PC (SDL)
│
├── UOSM-Core/                        subtree partagé — NE PAS MODIFIER ici
├── lib/                              lvgl, FreeRTOS, LwIP (code externe, on n'y touche pas)
├── tests/                            tournent sur PC
├── tools/                            fake_car.py, mqtt_monitor.sh
└── Documentation/                    ce fichier + MQTT_TOPICS.md
```

---

## 4. Les règles

1. **Seul `board/` inclut des headers STM32, HAL, BSP ou MCP2515.**
   Si un fichier de `app/` en a besoin, il manque une fonction dans `board.h`.
2. **UOSM-Core ne se modifie pas depuis ce repo.** Tout le MQTT vit dans `app/comms/mqtt/`.
3. **Un topic = une valeur = une ligne** dans `MQTT_TOPICS.md` **et** dans `mqtt_topics.c`.
   On écrit la ligne dans la doc **avant** de coder.
4. **Un topic inconnu ou un texte invalide est ignoré, jamais un crash.** Il est compté dans `DebugView`.
5. **Seule `ui_task` touche LVGL.** Les autres tâches écrivent dans `vehicle_state`, l'UI lit.
6. **Chaque valeur a son heure de réception.** Au-delà du timeout (`dashboard_config.h`), l'écran affiche `--`.
7. **CubeMX ne génère que dans `board/stm32f769_disco/`.**

---

## 5. Le contrat matériel : `board.h`

C'est tout ce que l'application a le droit de demander au matériel.

```c
void     board_init(void);                 // horloges, pins, écran, réseau
uint32_t board_millis(void);               // ms depuis le démarrage
void     board_log(const char* msg);       // print série (ou console sur PC)

void     board_display_flush(const lv_area_t* area, const uint8_t* pixels);  // appelé par LVGL
bool     board_touch_read(int16_t* x, int16_t* y);                          // appelé par LVGL
```

Le réseau n'a pas de fonction dédiée : LwIP fournit des sockets standard, donc le client MQTT
est portable. Le board initialise juste l'Ethernet dans `board_init()`.
Le CAN reste géré par UOSM-Core, configuré via `UOSMCoreConfig.h`.

---

## 6. Les tâches FreeRTOS

FreeRTOS fait tourner plusieurs boucles « en parallèle » et garantit que la réception passe avant le dessin.

| Tâche | Active si | Rôle |
|---|---|---|
| `tcpip_thread` (LwIP) | TRANSPORT=MQTT | réseau ; les callbacks MQTT décodent et écrivent dans `vehicle_state` |
| `can_task` | TRANSPORT=CAN | lecture CAN (tâche actuelle, inchangée) |
| `ui_task` | toujours | lit `vehicle_state`, met à jour l'écran, `lv_timer_handler()` |

Un seul mutex, court, protège `vehicle_state`.

---

## 7. Le contrat MQTT (résumé)

La référence complète est `Documentation/MQTT_TOPICS.md`. Règles du format :

- Topics en minuscules, forme `uosm/<carte>/<valeur>`.
- Contenu : **un seul nombre en texte**, point décimal (`47.02`), **sans unité**.
- Toujours dans l'unité définie par le tableau des topics.
- QoS 0, pas de `retain`.

| Topic | Exemple | Unité |
|---|---|---|
| `uosm/battery/voltage` | `47.02` | V |
| `uosm/battery/current` | `3.15` | A |
| `uosm/motor/speed` | `24.5` | km/h |
| `uosm/motor/rpm` | `1830` | tr/min |
| `uosm/motor/temperature` | `41.3` | °C |
| `uosm/lap/efficiency` | `312.4` | km/kWh |
| `uosm/lap/new` | `1` | — |

`mqtt_decode.c` convertit l'unité du texte vers le type interne :

```c
static void OnMotorTemperature(float celsius) {
    SetMotorTemperature(aggregator, (temperature_t)(celsius * 100));  // temperature_t = centi-°C
}
static void OnSpeed(float kmh) {
    SetSpeed(aggregator, (speed_t)(kmh * 1000));                      // même échelle que le CAN
}
```

> ⚠️ L'unité interne de `voltage_t` et `current_t` n'est pas documentée. À confirmer avec la carte
> batterie pour que MQTT et CAN affichent la même chose.

---

## 8. Recettes

### Ajouter une nouvelle valeur (MQTT)
1. Ajouter une ligne dans `MQTT_TOPICS.md` (topic, unité, carte qui l'envoie, fréquence) et la faire valider.
2. Ajouter le champ dans `vehicle_state` (`DataAggregator.hpp`) + son `Set...()` dans le wrapper.
3. Ajouter `{ "uosm/.../...", OnMaValeur }` dans `mqtt_topics.c` et écrire `OnMaValeur` dans `mqtt_decode.c`.
4. L'afficher dans un écran de `ui/screens/`.
5. Tester avec `mosquitto_pub -t uosm/.../... -m 12.3`.

### Porter sur un nouveau board
1. Créer `board/<nouveau_board>/` avec son projet CubeMX (ou équivalent).
2. Implémenter les fonctions de `board.h` (écran, tactile, temps, log, init réseau).
3. Ajouter un preset dans `CMakePresets.json`.
4. **Rien à changer dans `app/`.**

### Passer en CAN de secours
Compiler avec le preset `disco_can` et reflasher. Le code CAN est celui d'aujourd'hui.

### Tester sans la voiture
- Sur la carte : Mosquitto sur un PC relié en Ethernet + `tools/fake_car.py`.
- Sans carte : preset `simulator` (fenêtre SDL sur PC, même client MQTT).
- Voir le trafic : `mosquitto_sub -v -t "uosm/#"`.

---

## Où sont les fichiers aujourd'hui

| Rôle cible | Aujourd'hui |
|---|---|
| `board/stm32f769_disco/` (généré) | `Core/Inc`, `Core/Src`, `Core/Startup`, `Drivers/STM32*`, `Drivers/CMSIS`, `Drivers/Components`, `Middlewares/` |
| `board/.../display.c`, `touch.c` | `Drivers/tft/`, `Drivers/touchpad/` |
| `app/app_main.cpp` | `Core/Tasks/TaskManager.c` + fin de `Core/Src/main.c` |
| `app/comms/can/` | `Core/Tasks/InternalCommsTask.c`, `Core/Modules/CANCallbacks.c` |
| `app/comms/mqtt/` | *à créer* |
| `app/vehicle_state/` | `Core/UI/Data/` (Observable, sans heure de réception) |
| `app/ui/` | `Core/UI/` (`HomeView`, `Utils/Cards`, `Utils/View`) |
| `ui_task` | `Core/Tasks/LVGLTimerTask.c` |

**Différence importante avec aujourd'hui :** actuellement, `DataAggregator` *pousse* chaque nouvelle valeur
vers l'écran (pattern Observable, listeners dans `HomeView.cpp`), depuis la tâche de communication,
protégée par `LVGL_Lock()`. La cible inverse le sens : l'UI *lit* l'état à son rythme.

---

## Plan de migration

Chaque étape laisse un dashboard qui fonctionne.

- [ ] 1. Écrire `MQTT_TOPICS.md` et le faire valider par les autres cartes.
- [ ] 2. CubeMX : activer ETH + LwIP, configurer le MPU, tester un ping.
- [ ] 3. `mqtt_client` + `mqtt_topics` + `mqtt_decode`, testés avec Mosquitto + `fake_car.py` (CAN intact).
- [ ] 4. `vehicle_state` avec heure de réception + `ui_task` qui lit (suppression des listeners et du mutex LVGL).
- [ ] 5. Rangement `app/` + `board/`, retrait des includes HAL hors de `board/`.
- [ ] 6. Simulateur PC + `tests/` en CI.
