/**
 * mqtt_client.h — connection to the MQTT broker.
 *
 * Connects to the broker (config/comms_config.h), subscribes to uosm/#, and
 * hands every received message to mqtt_topics_dispatch(). Reconnects on its own
 * if the broker is unreachable or the connection drops.
 *
 * Functions are prefixed comms_mqtt_ because LwIP already owns the mqtt_client_
 * prefix (mqtt_client_new, mqtt_client_is_connected, ...).
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Starts the client. Call once, from any task, AFTER LwIP is initialised
 * (MX_LWIP_Init() in board/). Returns immediately: the connection happens
 * in the background, in LwIP's own task.
 */
void comms_mqtt_start(void);

/** True while connected to the broker (for the debug screen). Safe from any task. */
bool comms_mqtt_is_connected(void);

#ifdef __cplusplus
}
#endif
