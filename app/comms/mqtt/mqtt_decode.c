#include "mqtt_decode.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * TODO(step 4): once app/vehicle_state/ is ready, replace the printf() calls
 * below with the real Set...() functions (SetBatteryVoltage(), SetSpeed(),
 * ...). For now we just print, so the mqtt_topics -> mqtt_decode pipeline
 * can be tested without depending on anything else.
 */

/** A value is never longer than this in text form (e.g. "-1234.5678"). */
#define MAX_PAYLOAD_LEN 31

/**
 * Copies payload/len into a local null-terminated buffer, then converts it
 * to a float. Returns false (and leaves *out untouched) if the payload is
 * too long, empty, or not a valid number.
 */
static bool parse_float(const char* payload, size_t len, float* out) {
    if (len == 0 || len > MAX_PAYLOAD_LEN) {
        return false;
    }

    char buf[MAX_PAYLOAD_LEN + 1];
    memcpy(buf, payload, len);
    buf[len] = '\0';

    char* end = NULL;
    float value = strtof(buf, &end);

    if (end == buf) {
        return false; /* nothing was parsed as a number */
    }
    if (*end != '\0') {
        return false; /* leftover text after the number, e.g. "47.02V" */
    }

    *out = value;
    return true;
}

void OnBatteryVoltage(const char* payload, size_t len) {
    float volts;
    if (!parse_float(payload, len, &volts)) {
        return;
    }
    printf("[mqtt_decode] battery.voltage = %.2f V\n", volts);
}

void OnBatteryCurrent(const char* payload, size_t len) {
    float amps;
    if (!parse_float(payload, len, &amps)) {
        return;
    }
    printf("[mqtt_decode] battery.current = %.2f A\n", amps);
}

void OnMotorSpeed(const char* payload, size_t len) {
    float kmh;
    if (!parse_float(payload, len, &kmh)) {
        return;
    }
    printf("[mqtt_decode] motor.speed = %.1f km/h\n", kmh);
}

void OnMotorRpm(const char* payload, size_t len) {
    float rpm;
    if (!parse_float(payload, len, &rpm)) {
        return;
    }
    printf("[mqtt_decode] motor.rpm = %.0f rpm\n", rpm);
}

void OnMotorTemperature(const char* payload, size_t len) {
    float celsius;
    if (!parse_float(payload, len, &celsius)) {
        return;
    }
    printf("[mqtt_decode] motor.temperature = %.1f degC\n", celsius);
}

void OnLapEfficiency(const char* payload, size_t len) {
    float km_per_kwh;
    if (!parse_float(payload, len, &km_per_kwh)) {
        return;
    }
    printf("[mqtt_decode] lap.efficiency = %.1f km/kWh\n", km_per_kwh);
}

void OnLapNew(const char* payload, size_t len) {
    float flag;
    if (!parse_float(payload, len, &flag)) {
        return;
    }
    printf("[mqtt_decode] lap.new = %s\n", (flag != 0.0f) ? "true" : "false");
}
