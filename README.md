\# UOSM Dashboard



Dashboard du véhicule UOSM, sur STM32F769I-Discovery.

Reçoit les données de la voiture par Ethernet (MQTT), avec le CAN en secours.



\## Cloner le projet



&#x20;   git clone --recurse-submodules https://github.com/itoure12/UOSM-NEW-DASBOARD.git



Si tu as déjà cloné sans `--recurse-submodules` :



&#x20;   git submodule update --init



\## Outils nécessaires



\- à compléter



\## Structure



Voir \[Documentation/Architecture.md](Documentation/Architecture.md).



\## Bibliothèques



| Bibliothèque | Version | Comment elle est incluse |

|---|---|---|

| LVGL | v8.3.11 | submodule dans `lib/lvgl` |

| UOSM-Core | main | git subtree dans `UOSM-Core/` |

| HAL, CMSIS, FreeRTOS, LwIP | FW\_F7 | générés par CubeMX dans `board/stm32f769\_disco/` (étape 2) |



\## Mettre à jour UOSM-Core



&#x20;   git subtree pull --prefix UOSM-Core https://github.com/UOSupermileage/UOSM-Core.git main --squash

