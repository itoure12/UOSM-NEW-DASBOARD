#include "mqtt_client.h"
#include "mqtt_topics.h"
#include "comms_config.h"

#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"

#include <stdio.h>
#include <string.h>

/*
 * Threading: every function in this file runs in LwIP's own task (tcpip_thread).
 * LwIP calls our callbacks from there, the reconnect timer (sys_timeout) fires
 * there, and comms_mqtt_start() hands the setup over to it with tcpip_callback().
 * Since only that task touches the LwIP client and the buffers below, no lock is
 * needed. The one exception is s_connected, which other tasks read.
 *
 * TODO(step 4): replace printf() with board_log() once board/ implements it.
 */

/* LwIP refuses topics longer than its header buffer (MQTT_VAR_HEADER_BUFFER_LEN). */
#define TOPIC_BUF_LEN   MQTT_VAR_HEADER_BUFFER_LEN
/* Our payloads are short numbers ("47.02"); anything longer is not ours. */
#define PAYLOAD_BUF_LEN 64

static mqtt_client_t* s_client;
static ip_addr_t s_broker_ip;
static volatile bool s_connected;

/* The message being received. LwIP gives its topic first (on_incoming_publish),
 * then its payload, possibly in several pieces (on_incoming_data). */
static char s_topic[TOPIC_BUF_LEN];
static char s_payload[PAYLOAD_BUF_LEN];
static size_t s_payload_len;
static bool s_drop_message; /* message too long for our buffers: ignore it */

static void connect_to_broker(void* arg);

static void schedule_reconnect(void) {
    printf("[mqtt_client] retrying in %d ms\n", COMMS_MQTT_RECONNECT_DELAY_MS);
    sys_timeout(COMMS_MQTT_RECONNECT_DELAY_MS, connect_to_broker, NULL);
}

static void on_subscribed(void* arg, err_t err) {
    (void)arg;
    if (err != ERR_OK) {
        printf("[mqtt_client] subscribe to %s failed (err %d)\n", COMMS_MQTT_SUBSCRIBE_TOPIC, err);
    }
}

/* Called by LwIP when the connection succeeds, is refused, or drops. */
static void on_connection(mqtt_client_t* client, void* arg, mqtt_connection_status_t status) {
    (void)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        s_connected = true;
        printf("[mqtt_client] connected to %s:%d\n", COMMS_MQTT_BROKER_IP, COMMS_MQTT_BROKER_PORT);
        mqtt_subscribe(client, COMMS_MQTT_SUBSCRIBE_TOPIC, 0, on_subscribed, NULL);
        return;
    }
    s_connected = false;
    printf("[mqtt_client] disconnected (status %d)\n", (int)status);
    schedule_reconnect();
}

/* Called by LwIP when a message starts arriving: its topic and total payload size.
 * `topic` is only valid during this call, so we copy it. */
static void on_incoming_publish(void* arg, const char* topic, u32_t tot_len) {
    (void)arg;
    size_t topic_len = strlen(topic);
    s_payload_len = 0;
    s_drop_message = (topic_len >= sizeof(s_topic)) || (tot_len > sizeof(s_payload));
    if (!s_drop_message) {
        memcpy(s_topic, topic, topic_len + 1);
    }
}

/* Called by LwIP with each piece of the payload. MQTT_DATA_FLAG_LAST marks the
 * last piece: only then is the message complete and handed to the dispatcher. */
static void on_incoming_data(void* arg, const u8_t* data, u16_t len, u8_t flags) {
    (void)arg;
    if (!s_drop_message) {
        if (s_payload_len + len <= sizeof(s_payload)) {
            memcpy(s_payload + s_payload_len, data, len);
            s_payload_len += len;
        } else {
            s_drop_message = true;
        }
    }
    if (flags & MQTT_DATA_FLAG_LAST) {
        if (!s_drop_message) {
            mqtt_topics_dispatch(s_topic, s_payload, s_payload_len);
        }
        s_payload_len = 0;
    }
}

static void connect_to_broker(void* arg) {
    (void)arg;
    struct mqtt_connect_client_info_t info;
    memset(&info, 0, sizeof(info)); /* no user, password or last-will message */
    info.client_id = COMMS_MQTT_CLIENT_ID;
    info.keep_alive = COMMS_MQTT_KEEP_ALIVE_S;

    err_t err = mqtt_client_connect(s_client, &s_broker_ip, COMMS_MQTT_BROKER_PORT,
                                    on_connection, NULL, &info);
    /* On success, on_connection() reports the outcome later. On failure here,
     * LwIP will not call it, so we schedule the retry ourselves. */
    if (err != ERR_OK) {
        printf("[mqtt_client] connect failed (err %d)\n", err);
        schedule_reconnect();
    }
}

/* Runs in LwIP's task (see comms_mqtt_start). */
static void start_in_tcpip_thread(void* arg) {
    (void)arg;
    if (!ipaddr_aton(COMMS_MQTT_BROKER_IP, &s_broker_ip)) {
        printf("[mqtt_client] invalid broker IP: %s\n", COMMS_MQTT_BROKER_IP);
        return;
    }
    s_client = mqtt_client_new();
    if (s_client == NULL) {
        printf("[mqtt_client] out of memory\n");
        return;
    }
    mqtt_set_inpub_callback(s_client, on_incoming_publish, on_incoming_data, NULL);
    connect_to_broker(NULL);
}

void comms_mqtt_start(void) {
    /* LwIP functions must run in LwIP's own task: hand the setup over to it. */
    if (tcpip_callback(start_in_tcpip_thread, NULL) != ERR_OK) {
        printf("[mqtt_client] could not start (LwIP not initialised?)\n");
    }
}

bool comms_mqtt_is_connected(void) {
    return s_connected;
}
