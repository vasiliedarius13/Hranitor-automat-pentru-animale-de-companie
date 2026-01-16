/**
 * @file mqtt.c
 * @brief Client MQTT simplificat pentru AVR
 */

#include "mqtt.h"
#include "wifi.h"
#include <string.h>
#include <util/delay.h>

#define MQTT_BUFFER_SIZE 512

static mqtt_callback_t message_callback = NULL;
static char client_id[32];
static uint8_t packet_id = 1;

/* ==================== FUNCȚII PRIVATE =================== */
static uint16_t create_connect_packet(char* buffer, const char* client_id) {
    uint16_t idx = 0;
    
    // Fixed header
    buffer[idx++] = 0x10; // CONNECT
    
    // Remaining length (se calculează mai târziu)
    uint16_t rl_idx = idx;
    idx++;
    
    // Protocol name
    buffer[idx++] = 0x00;
    buffer[idx++] = 0x04;
    buffer[idx++] = 'M';
    buffer[idx++] = 'Q';
    buffer[idx++] = 'T';
    buffer[idx++] = 'T';
    
    // Protocol level
    buffer[idx++] = 0x04; // MQTT 3.1.1
    
    // Connect flags
    buffer[idx++] = 0xC2; // Clean session = 1, Will flag = 0, QoS = 0, Retain = 0
    
    // Keep alive (60 seconds)
    buffer[idx++] = 0x00;
    buffer[idx++] = 0x3C;
    
    // Client ID
    uint16_t client_id_len = strlen(client_id);
    buffer[idx++] = client_id_len >> 8;
    buffer[idx++] = client_id_len & 0xFF;
    memcpy(&buffer[idx], client_id, client_id_len);
    idx += client_id_len;
    
    // Calculate remaining length
    uint16_t remaining_length = idx - rl_idx - 1;
    buffer[rl_idx] = remaining_length;
    
    return idx;
}

static uint16_t create_publish_packet(char* buffer, const char* topic, 
                                     const char* payload, uint8_t qos, uint8_t retain) {
    uint16_t idx = 0;
    
    // Fixed header
    buffer[idx++] = 0x30 | (qos << 1) | retain; // PUBLISH
    
    // Remaining length placeholder
    uint16_t rl_idx = idx;
    idx++;
    
    // Topic
    uint16_t topic_len = strlen(topic);
    buffer[idx++] = topic_len >> 8;
    buffer[idx++] = topic_len & 0xFF;
    memcpy(&buffer[idx], topic, topic_len);
    idx += topic_len;
    
    // Packet ID for QoS > 0
    if (qos > 0) {
        buffer[idx++] = packet_id >> 8;
        buffer[idx++] = packet_id & 0xFF;
        packet_id++;
    }
    
    // Payload
    uint16_t payload_len = strlen(payload);
    memcpy(&buffer[idx], payload, payload_len);
    idx += payload_len;
    
    // Calculate remaining length
    uint16_t remaining_length = idx - rl_idx - 1;
    buffer[rl_idx] = remaining_length;
    
    return idx;
}

static uint16_t create_subscribe_packet(char* buffer, const char* topic, uint8_t qos) {
    uint16_t idx = 0;
    
    // Fixed header
    buffer[idx++] = 0x82; // SUBSCRIBE
    
    // Remaining length placeholder
    uint16_t rl_idx = idx;
    idx++;
    
    // Packet ID
    buffer[idx++] = packet_id >> 8;
    buffer[idx++] = packet_id & 0xFF;
    packet_id++;
    
    // Topic
    uint16_t topic_len = strlen(topic);
    buffer[idx++] = topic_len >> 8;
    buffer[idx++] = topic_len & 0xFF;
    memcpy(&buffer[idx], topic, topic_len);
    idx += topic_len;
    
    // QoS
    buffer[idx++] = qos;
    
    // Calculate remaining length
    uint16_t remaining_length = idx - rl_idx - 1;
    buffer[rl_idx] = remaining_length;
    
    return idx;
}

static void parse_mqtt_packet(const char* buffer, uint16_t len) {
    if (len < 2) return;
    
    uint8_t packet_type = buffer[0] >> 4;
    
    switch (packet_type) {
        case 3: // PUBLISH
            if (message_callback != NULL) {
                // Extrage topic și payload (simplificat)
                uint16_t topic_len = (buffer[2] << 8) | buffer[3];
                char topic[128];
                memcpy(topic, &buffer[4], topic_len);
                topic[topic_len] = '\0';
                
                uint16_t payload_start = 4 + topic_len;
                if (buffer[0] & 0x06) { // QoS > 0
                    payload_start += 2;
                }
                
                char payload[256];
                uint16_t payload_len = len - payload_start;
                memcpy(payload, &buffer[payload_start], payload_len);
                payload[payload_len] = '\0';
                
                message_callback(topic, payload);
            }
            break;
            
        case 13: // PINGRESP
            // Ping response received
            break;
    }
}

/* ==================== FUNCȚII PUBLICE =================== */
void mqtt_init(mqtt_callback_t callback) {
    message_callback = callback;
}

uint8_t mqtt_connect(const char* broker, uint16_t port, const char* id) {
    strcpy(client_id, id);
    
    // Conectare TCP la broker
    char packet[MQTT_BUFFER_SIZE];
    uint16_t packet_len = create_connect_packet(packet, client_id);
    
    // Trimite pachetul CONNECT
    wifi_send_tcp(broker, port, packet);
    
    // Aici ar trebui să verificăm CONNACK, dar simplificăm
    _delay_ms(1000);
    
    return 1;
}

void mqtt_disconnect(void) {
    char packet[2] = {0xE0, 0x00}; // DISCONNECT
    // În realitate, ar trebui trimis la broker
}

void mqtt_publish(const char* topic, const char* payload, uint8_t qos, uint8_t retain) {
    char packet[MQTT_BUFFER_SIZE];
    uint16_t packet_len = create_publish_packet(packet, topic, payload, qos, retain);
    
    // Trimite pachetul (simplificat - nu se trimite efectiv)
    // Într-o implementare reală, s-ar trimite prin WiFi
}

void mqtt_subscribe(const char* topic, uint8_t qos) {
    char packet[MQTT_BUFFER_SIZE];
    uint16_t packet_len = create_subscribe_packet(packet, topic, qos);
    
    // Trimite pachetul SUBSCRIBE (simplificat)
}

void mqtt_loop(void) {
    // Într-o implementare reală, aici s-ar procesa mesajele primite
    // Prin simplitate, nu implementăm recepția full
}

void mqtt_ping(void) {
    char packet[2] = {0xC0, 0x00}; // PINGREQ
    // Trimite ping la broker
}