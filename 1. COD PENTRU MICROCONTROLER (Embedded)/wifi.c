/**
 * @file wifi.c
 * @brief Manager WiFi cu ESP8266
 */

#include "wifi.h"
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#define WIFI_BAUD 115200
#define BUFFER_SIZE 256

static uint8_t wifi_tx_pin;
static uint8_t wifi_rx_pin;
static char buffer[BUFFER_SIZE];
static uint16_t buffer_idx = 0;

/* ==================== FUNCȚII PRIVATE =================== */
static void uart_init(uint32_t baud) {
    uint16_t ubrr = F_CPU / 16 / baud - 1;
    
    // Set baud rate
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    
    // Set frame format: 8 data bits, 1 stop bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

static void uart_send_char(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

static void uart_send_string(const char* str) {
    while (*str) {
        uart_send_char(*str++);
    }
}

static char uart_receive_char(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

static void wifi_send_command(const char* cmd) {
    uart_send_string(cmd);
    uart_send_string("\r\n");
    _delay_ms(100);
}

static uint8_t wifi_wait_response(const char* expected, uint16_t timeout) {
    uint32_t start_time = 0;
    buffer_idx = 0;
    buffer[0] = '\0';
    
    while (1) {
        if (UCSR0A & (1 << RXC0)) {
            char c = uart_receive_char();
            
            if (buffer_idx < BUFFER_SIZE - 1) {
                buffer[buffer_idx++] = c;
                buffer[buffer_idx] = '\0';
            }
            
            // Verifică dacă avem răspunsul așteptat
            if (strstr(buffer, expected) != NULL) {
                return 1;
            }
            
            // Verifică timeout
            if (start_time++ > timeout * 10) {
                return 0;
            }
        }
        _delay_ms(1);
    }
}

/* ==================== FUNCȚII PUBLICE =================== */
void wifi_init(uint8_t tx_pin, uint8_t rx_pin) {
    wifi_tx_pin = tx_pin;
    wifi_rx_pin = rx_pin;
    
    uart_init(WIFI_BAUD);
    _delay_ms(2000); // Așteptare inițializare ESP8266
}

uint8_t wifi_connect(const char* ssid, const char* password) {
    // Reset ESP8266
    wifi_send_command("AT+RST");
    if (!wifi_wait_response("ready", 5000)) return 0;
    
    // Setare mod stație
    wifi_send_command("AT+CWMODE=1");
    if (!wifi_wait_response("OK", 1000)) return 0;
    
    // Conectare la WiFi
    char cmd[128];
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    wifi_send_command(cmd);
    
    if (wifi_wait_response("WIFI CONNECTED", 10000)) {
        // Obține IP
        wifi_send_command("AT+CIFSR");
        if (wifi_wait_response("OK", 1000)) {
            return 1;
        }
    }
    
    return 0;
}

uint8_t wifi_disconnect(void) {
    wifi_send_command("AT+CWQAP");
    return wifi_wait_response("OK", 1000);
}

uint8_t wifi_get_ip(char* ip_buffer) {
    wifi_send_command("AT+CIFSR");
    if (wifi_wait_response("+CIFSR:STAIP", 1000)) {
        // Parsează IP din buffer
        char* ip_start = strstr(buffer, "\"");
        if (ip_start) {
            ip_start++;
            char* ip_end = strstr(ip_start, "\"");
            if (ip_end) {
                strncpy(ip_buffer, ip_start, ip_end - ip_start);
                ip_buffer[ip_end - ip_start] = '\0';
                return 1;
            }
        }
    }
    return 0;
}

void wifi_send_tcp(const char* host, uint16_t port, const char* data) {
    char cmd[128];
    
    // Închide orice conexiune existentă
    wifi_send_command("AT+CIPCLOSE");
    _delay_ms(100);
    
    // Creează conexiune TCP
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d", host, port);
    wifi_send_command(cmd);
    if (!wifi_wait_response("CONNECT", 5000)) return;
    
    // Trimite date
    sprintf(cmd, "AT+CIPSEND=%d", (uint16_t)strlen(data));
    wifi_send_command(cmd);
    if (wifi_wait_response(">", 1000)) {
        uart_send_string(data);
        _delay_ms(100);
        wifi_send_command("");
    }
    
    // Închide conexiunea
    _delay_ms(100);
    wifi_send_command("AT+CIPCLOSE");
}