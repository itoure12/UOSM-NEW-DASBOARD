#include "mqtt_topics.h"
#include "mqtt_decode.h"

#include <string.h>

/** Type of a decode function: the common signature of every OnXxx function
 *  declared in mqtt_decode.h. */
typedef void (*mqtt_decoder_fn)(const char* payload, size_t len);

/** One table row: a topic + the function that knows how to handle it. */
typedef struct {
    const char* topic;
    mqtt_decoder_fn decode;
} mqtt_topic_entry_t;

/*
 * THE table. To add a value: one line here (recipe in
 * Documentation/Architecture.md section 8), after adding it to
 * Documentation/MQTT_TOPICS.md.
 */
static const mqtt_topic_entry_t kTopics[] = {
    { "uosm/battery/voltage",     OnBatteryVoltage },
    { "uosm/battery/current",     OnBatteryCurrent },
    { "uosm/motor/speed",         OnMotorSpeed },
    { "uosm/motor/rpm",           OnMotorRpm },
    { "uosm/motor/temperature",   OnMotorTemperature },
    { "uosm/lap/efficiency",      OnLapEfficiency },
    { "uosm/lap/new",             OnLapNew },
};

#define TOPIC_COUNT (sizeof(kTopics) / sizeof(kTopics[0]))

void mqtt_topics_dispatch(const char* topic, const char* payload, size_t len) {
    for (size_t i = 0; i < TOPIC_COUNT; i++) {
        if (strcmp(topic, kTopics[i].topic) == 0) {
            kTopics[i].decode(payload, len);
            return;
        }
    }
    /* Unknown topic: ignored, never a crash (rule 4).
     * TODO(step 4 / DebugView): count unknown topics to display them,
     * once DebugView is migrated. */
}
