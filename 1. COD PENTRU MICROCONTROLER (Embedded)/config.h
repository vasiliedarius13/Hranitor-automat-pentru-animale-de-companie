/**
 * @file config.h
 * @brief Configurații sistem embedded
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>
#include <stdint.h>

/* ==================== DEFINIȚII PINI ==================== */
#define PIN_SERVO         5   // PD5 - Servomotor
#define PIN_BUZZER        2   // PD2 - Buzzer
#define PIN_LED           3   // PD3 - LED RGB
#define PIN_BUTTON        4   // PD4 - Buton manual
#define PIN_TRIG          7   // PD7 - HC-SR04 Trigger
#define PIN_ECHO          8   // PB0 - HC-SR04 Echo
#define PIN_TEMP          6   // PD6 - DS18B20 Data
#define PIN_WIFI_TX       1   // PD1 - TX către ESP8266
#define PIN_WIFI_RX       0   // PD0 - RX de la ESP8266

/* ==================== CONFIGURAȚII MQTT ================= */
#define MQTT_BROKER       "broker.hivemq.com"
#define MQTT_PORT         1883
#define MQTT_CLIENT_ID    "petfeeder_01"
#define MQTT_KEEPALIVE    60

/* ==================== TOPICURI MQTT ===================== */
#define TOPIC_STATUS      "petfeeder/01/status"
#define TOPIC_SENSORS     "petfeeder/01/sensors"
#define TOPIC_COMMANDS    "petfeeder/01/commands"
#define TOPIC_LOGS        "petfeeder/01/logs"

/* ==================== TIMPI SISTEM ====================== */
#define FEED_TIME_AM      8    // Ora 8:00 dimineața
#define FEED_TIME_PM      18   // Ora 18:00 seara
#define FEED_DURATION     2000 // 2 secunde
#define SENSOR_INTERVAL   60000 // 60 secunde
#define STATUS_INTERVAL   300000 // 5 minute

/* ==================== PARAMETRI SENZORI ================= */
#define TEMP_MIN          15.0
#define TEMP_MAX          30.0
#define FOOD_LEVEL_MIN    5.0  // Nivel minim hrană (cm)
#define FOOD_LEVEL_MAX    30.0 // Nivel maxim hrană (cm)

/* ==================== TIPURI DE DATE ==================== */
typedef struct {
    float temperature;
    float humidity;
    float food_level;
    uint16_t battery;
} sensor_data_t;

typedef enum {
    STATE_IDLE = 0,
    STATE_FEEDING = 1,
    STATE_ERROR = 2,
    STATE_CONNECTING = 3
} system_state_t;

#endif /* CONFIG_H */