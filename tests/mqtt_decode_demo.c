/**
 * mqtt_decode_demo.c — manual demo, not a real automated test yet (that will come).
 *
 * Purpose: visually check that mqtt_topics_dispatch() does what we expect,
 * WITHOUT needing MQTT, the network, or the board. Just plain C.
 *
 * Build and run (see the exact command used in the setup):
 *   gcc mqtt_decode_demo.c ../app/comms/mqtt/mqtt_topics.c \
 *       ../app/comms/mqtt/mqtt_decode.c -I../app/comms/mqtt -o demo.exe
 *   ./demo.exe
 */
#include "mqtt_topics.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    printf("--- 1. Valid values (should print) ---\n");
    mqtt_topics_dispatch("uosm/battery/voltage", "47.02", strlen("47.02"));
    mqtt_topics_dispatch("uosm/motor/rpm", "1830", strlen("1830"));
    mqtt_topics_dispatch("uosm/lap/new", "1", strlen("1"));

    printf("\n--- 2. Invalid payload (nothing should print, no crash) ---\n");
    mqtt_topics_dispatch("uosm/battery/current", "abc", strlen("abc"));
    mqtt_topics_dispatch("uosm/motor/speed", "24.5V", strlen("24.5V")); /* trailing text after the number */
    mqtt_topics_dispatch("uosm/motor/temperature", "", 0);             /* empty payload */

    printf("\n--- 3. Unknown topic (nothing should print, no crash) ---\n");
    mqtt_topics_dispatch("uosm/unknown/value", "123", strlen("123"));

    printf("\n--- 4. Proof that 'len' matters, not just the buffer content ---\n");
    /* We only pass the first 5 bytes ("47.02") as len, even though the
     * buffer has more text after it. If we read with strlen() instead of
     * respecting len, we'd read the extra "XXXXX" too -> invalid text ->
     * value ignored. By respecting len, we read exactly "47.02". */
    char raw[] = "47.02XXXXX";
    mqtt_topics_dispatch("uosm/battery/voltage", raw, 5);

    printf("\n--- Demo done ---\n");
    return 0;
}
