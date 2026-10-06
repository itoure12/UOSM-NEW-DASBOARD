/**
 * mqtt_topics.h — the dispatch table.
 *
 * Knows nothing about vehicle values (voltage, speed...). Just knows
 * "this topic goes with this function". See Documentation/MQTT_TOPICS.md
 * for the list of topics.
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Looks up the topic in the table and calls the matching decode function.
 * Does nothing if the topic is unknown (rule: never crash on unexpected data).
 *
 * @param topic    the MQTT topic, e.g. "uosm/battery/voltage" (regular C
 *                 string, null-terminated — that's how LwIP gives it to us)
 * @param payload  the message content, e.g. "47.02"
 * @param len      length of payload IN BYTES (payload is NOT guaranteed to
 *                 be null-terminated, don't use strlen() on it)
 */
void mqtt_topics_dispatch(const char* topic, const char* payload, size_t len);

#ifdef __cplusplus
}
#endif
