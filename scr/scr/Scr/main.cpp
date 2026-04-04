/**
 * @file ESP32_SD_Scraping.cpp
 * @author Arturo_AlonsoLP Linuxmatic
 * @brief Scrap weather data and log in (MISRA C:2012 compliant style).
 * @version 0.2
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <stdint.h>
#include <string.h>

/* Tipos de datos de ancho fijo (MISRA Rule 4.6) */
typedef float           float32_t;
typedef uint32_t        tick_t;

/* Configuración de Red (Constantes con punteros restrictivos) */
static const char* const SSID_WIFI = "TU_SSID_WIFI";
static const char* const PWD_WIFI  = "TU_PASSWORD";
static const char* const URL_BASE  = "http://urban.diau.buap.mx/estaciones/ramm07/ramm07.php";

#define SD_CS_PIN      (5)
#define MAX_BUFFER_WEB (2048U) /* Ajustar según tamaño esperado del HTML */
#define MAX_VAL_LEN    (16U)

/**
 * @brief Extrae un valor numérico de una cadena de texto (Buffer seguro).
 * MISRA: No usa memoria dinámica.
 */
static void extraerDato(const char* src, const char* etiqueta, char* dest, uint32_t destSize) {
    if ((src != NULL) && (etiqueta != NULL) && (dest != NULL)) {
        const char* p_start = strstr(src, etiqueta);
        if (p_start != NULL) {
            p_start += strlen(etiqueta);
            const char* p_end = strchr(p_start, ' ');
            
            size_t len = (p_end != NULL) ? (size_t)(p_end - p_start) : strlen(p_start);
            
            if (len >= (size_t)destSize) {
                len = (size_t)destSize - 1U;
            }
            
            (void)memcpy(dest, p_start, len);
            dest[len] = '\0';
        } else {
            (void)strncpy(dest, "N/A", destSize);
        }
    }
}

void setup(void) {
    Serial.begin(115200);
    
    if (!SD.begin((uint8_t)SD_CS_PIN)) {
        Serial.println(F("Error: SD Fail"));
    }

    WiFi.begin(SSID_WIFI, PWD_WIFI);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(F("\nWiFi OK"));
}

void loop(void) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        (void)http.begin(URL_BASE);
        
        int16_t httpCode = (int16_t)http.GET();

        if (httpCode == (int16_t)HTTP_CODE_OK) {
            /* Uso de buffer estático para evitar fragmentación de heap */
            static char payload[MAX_BUFFER_WEB];
            static char temp[MAX_VAL_LEN];
            static char pres[MAX_VAL_LEN];
            static char hum[MAX_VAL_LEN];
            static char dataLog[128];

            (void)strncpy(payload, http.getString().c_str(), sizeof(payload) - 1U);

            extraerDato(payload, "Temperatura: ", temp, (uint32_t)sizeof(temp));
            extraerDato(payload, "Presion: ", pres, (uint32_t)sizeof(pres));
            extraerDato(payload, "Humedad: ", hum, (uint32_t)sizeof(hum));

            /* Construcción segura de línea CSV */
            (void)snprintf(dataLog, sizeof(dataLog), "%lu,%s,%s,%s", 
                           (unsigned long)millis(), temp, pres, hum);

            File dataFile = SD.open("/clima_buap.csv", FILE_APPEND);
            if (dataFile) {
                (void)dataFile.println(dataLog);
                dataFile.close();
                Serial.println(dataLog);
            }
        }
        http.end();
    }
    
    delay(300000UL); 
}
