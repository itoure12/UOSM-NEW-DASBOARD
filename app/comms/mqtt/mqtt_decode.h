/**
 * mqtt_decode.h — one function per vehicle value.
 *
 * Each function receives the raw payload of a topic (see mqtt_topics.h for
 * the payload/len rules), converts it, and writes the result somewhere
 * (for now: a simple log — see the TODO in mqtt_decode.c).
 *
 * One function here = one line in app/comms/mqtt/mqtt_topics.c
 *                    = one line in Documentation/MQTT_TOPICS.md
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void OnBatteryVoltage(const char* payload, size_t len);
void OnBatteryCurrent(const char* payload, size_t len);
void OnMotorSpeed(const char* payload, size_t len);
void OnMotorRpm(const char* payload, size_t len);
void OnMotorTemperature(const char* payload, size_t len);
void OnLapEfficiency(const char* payload, size_t len);
void OnLapNew(const char* payload, size_t len);

#ifdef __cplusplus
}
#endif
