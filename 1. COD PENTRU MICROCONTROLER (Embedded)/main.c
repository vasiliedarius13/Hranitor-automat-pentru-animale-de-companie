/**
 * @file main.c
 * @brief Program principal sistem embedded
 */

#include "config.h"
#include "wifi.h"
#include "mqtt.h"
#include "sensors.h"
#include "servo.h"
#include "led.h"
#include "rtc.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

/* ==================== VARIABILE GLOBALE ================= */
static volatile system_state_t current_state = STATE_CONNECTING;
static volatile uint8_t wifi_connected = 0;
static volatile uint8_t mqtt_connected = 0;
static volatile uint32_t system_tick = 0;
static volatile uint8_t feed_morning_done = 0;
static volatile uint8_t feed_evening_done = 0;
static sensor_data_t last_sensor_data;

/* ==================== PROTOTIPURI FUNCȚII =============== */
static void system_init(void);
static void check_schedule(void);
static void feed_now(uint8_t amount);
static void update_status(void);
static void process_mqtt_message(const char* topic, const char* payload);
static void button_handler(void);

/* ==================== INIȚIALIZARE SISTEM =============== */
static void system_init(void) {
    // Dezactivează întreruperile temporar
    cli();
    
    // Configurare pini I/O
    DDRD |= (1 << PIN_SERVO) | (1 << PIN_BUZZER) | (1 << PIN_LED) | (1 << PIN_TRIG);
    DDRD &= ~((1 << PIN_BUTTON) | (1 << PIN_TEMP) | (1 << PIN_ECHO));
    PORTD |= (1 << PIN_BUTTON); // Pull-up pentru buton
    
    // Inițializare componente
    servo_init(PIN_SERVO, 20, 150);
    led_init(PIN_LED);
    rtc_init();
    sensors_init(PIN_TEMP, PIN_TRIG, PIN_ECHO);
    
    // Inițializare WiFi și MQTT
    wifi_init(PIN_WIFI_TX, PIN_WIFI_RX);
    mqtt_init(process_mqtt_message);
    
    // Semnal start
    led_set_color(0, 255, 0); // Verde
    _delay_ms(500);
    led_set_color(0, 0, 0);   // Stins
    
    // Activează întreruperi
    sei();
    
    // Conectare la WiFi
    if (wifi_connect("SSID", "PASSWORD")) {
        wifi_connected = 1;
        current_state = STATE_CONNECTING;
        
        // Conectare MQTT
        if (mqtt_connect(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID)) {
            mqtt_connected = 1;
            current_state = STATE_IDLE;
            mqtt_subscribe(TOPIC_COMMANDS, 1);
        }
    }
    
    // Publică stare inițială
    update_status();
}

/* ==================== FUNCȚIA DE HRĂNIRE ================ */
static void feed_now(uint8_t amount) {
    if (current_state == STATE_FEEDING) return;
    
    current_state = STATE_FEEDING;
    
    // Semnalizare vizuală și sonoră
    led_set_color(0, 255, 0); // Verde
    PORTD |= (1 << PIN_BUZZER); // Buzzer ON
    
    // Distribuie hrană
    servo_feed();
    
    // Publică eveniment
    char payload[128];
    sprintf(payload, "{\"event\":\"feeding\",\"amount\":%d,\"time\":%lu}",
            amount, system_tick);
    mqtt_publish(TOPIC_LOGS, payload, 1, 0);
    
    // Oprește semnalizare
    _delay_ms(500);
    PORTD &= ~(1 << PIN_BUZZER); // Buzzer OFF
    led_set_color(0, 0, 0); // Stins
    
    current_state = STATE_IDLE;
    update_status();
}

/* ==================== VERIFICARE PROGRAM ================ */
static void check_schedule(void) {
    rtc_time_t now = rtc_get_time();
    
    // Verifică hrănire dimineața
    if (now.hours == FEED_TIME_AM && now.minutes == 0 && !feed_morning_done) {
        feed_now(25); // 25g hrană
        feed_morning_done = 1;
    }
    
    // Verifică hrănire seara
    if (now.hours == FEED_TIME_PM && now.minutes == 0 && !feed_evening_done) {
        feed_now(25); // 25g hrană
        feed_evening_done = 1;
    }
    
    // Reset flag-uri la miezul nopții
    if (now.hours == 0 && now.minutes == 0) {
        feed_morning_done = 0;
        feed_evening_done = 0;
    }
}

/* ==================== ACTUALIZARE STARE ================= */
static void update_status(void) {
    char payload[256];
    sprintf(payload, 
        "{\"state\":%d,\"battery\":%d,\"wifi\":%d,\"mqtt\":%d,\"time\":%lu}",
        current_state, 
        last_sensor_data.battery,
        wifi_connected,
        mqtt_connected,
        system_tick
    );
    
    mqtt_publish(TOPIC_STATUS, payload, 1, 1); // QoS 1, retain
}

/* ==================== PROCESARE MQTT ==================== */
static void process_mqtt_message(const char* topic, const char* payload) {
    if (strcmp(topic, TOPIC_COMMANDS) == 0) {
        // Parsează comanda JSON simplificat
        if (strstr(payload, "\"cmd\":\"feed\"") != NULL) {
            feed_now(25); // Hrănire manuală remote
        }
        else if (strstr(payload, "\"cmd\":\"status\"") != NULL) {
            update_status();
        }
    }
}

/* ==================== HANDLER BUTON ===================== */
static void button_handler(void) {
    static uint32_t last_press = 0;
    
    // Debouncing și anti-repeat
    if ((system_tick - last_press) > 20) { // 2 secunde
        feed_now(25); // Hrănire manuală local
        last_press = system_tick;
    }
}

/* ==================== FUNCȚIA PRINCIPALĂ ================ */
int main(void) {
    uint32_t last_sensor_update = 0;
    uint32_t last_status_update = 0;
    
    // Inițializare sistem
    system_init();
    
    // Buclă principală
    while (1) {
        // Incrementare timer
        system_tick++;
        
        // Verifică buton la fiecare 100ms
        if ((system_tick % 1) == 0) { // 100ms
            if ((PIND & (1 << PIN_BUTTON)) == 0) {
                _delay_ms(50); // Debouncing
                if ((PIND & (1 << PIN_BUTTON)) == 0) {
                    button_handler();
                }
            }
        }
        
        // Verifică program hrănire la fiecare secundă
        if ((system_tick % 10) == 0) { // 1 secundă
            check_schedule();
        }
        
        // Actualizează senzorii la fiecare 60 secunde
        if ((system_tick - last_sensor_update) >= 600) { // 60 secunde
            last_sensor_data = sensors_read_all();
            last_sensor_update = system_tick;
            
            // Publică date senzori
            char payload[256];
            sprintf(payload,
                "{\"temp\":%.2f,\"food\":%.2f,\"battery\":%d,\"time\":%lu}",
                last_sensor_data.temperature,
                last_sensor_data.food_level,
                last_sensor_data.battery,
                system_tick
            );
            mqtt_publish(TOPIC_SENSORS, payload, 0, 0);
        }
        
        // Actualizează stare la fiecare 5 minute
        if ((system_tick - last_status_update) >= 3000) { // 5 minute
            update_status();
            last_status_update = system_tick;
        }
        
        // Process MQTT messages
        mqtt_loop();
        
        // Pauză pentru consum redus
        _delay_ms(100);
    }
    
    return 0;
}