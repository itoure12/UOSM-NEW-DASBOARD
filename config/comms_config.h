/**
 * comms_config.h — network settings for the MQTT client (app/comms/mqtt/).
 *
 * Prefixed COMMS_ because LwIP already defines its own MQTT_* macros.
 */
#pragma once

/* Address of the machine running the Mosquitto broker.
 * TODO: placeholder until the team decides which machine runs the broker.
 * The dashboard itself is at 192.168.1.10 (static IP set in board/). */
#define COMMS_MQTT_BROKER_IP        "192.168.1.100"
#define COMMS_MQTT_BROKER_PORT      1883

/* Name the dashboard uses on the broker. Must be unique among connected clients. */
#define COMMS_MQTT_CLIENT_ID        "uosm-dashboard"

/* Everything under uosm/ (see Documentation/MQTT_TOPICS.md). */
#define COMMS_MQTT_SUBSCRIBE_TOPIC  "uosm/#"

/* Wait before trying again when the broker is unreachable or the connection drops. */
#define COMMS_MQTT_RECONNECT_DELAY_MS 2000

/* The client pings the broker this often. If the broker stays silent for about
 * 1.5x this, the connection is considered dead and we reconnect. Without it,
 * an unplugged cable could go unnoticed. */
#define COMMS_MQTT_KEEP_ALIVE_S     10
