/**
 * @file ESP32_SD_Scraping.cpp
 * @author Arturo_Alonso_P Linuxmatic & Asistente Progra
 * @brief Scrap weather data with Time Validation (MISRA C:2012 compliant style).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <stdint.h>
#include <string.h>
#include "time.h" // Librería estándar de tiempo

typedef float         float32_t;
typedef uint32_t      tick_t;

static const char* const SSID_WIFI = "TU_SSID_WIFI";
static const char* const PWD_WIFI  = "TU_PASSWORD";
static const char* const URL_BASE  = "http://urban.diau.buap.mx/estaciones/ramm07/ramm07.php";

/* Configuración NTP para México (UTC-6) */
static const char* const NTP_SERVER = "pool.ntp.org";
static const long  GMT_OFFSET_SEC   = -21600; // UTC -6 horas
static const int   DAYLIGHT_OFFSET  = 0;

#define SD_CS_PIN      (5)
#define MAX_BUFFER_WEB (2048U)
#define MAX_VAL_LEN    (24U) // Incrementado para fechas largas

static void extraerDato(const char* src, const char* etiqueta, char* dest, uint32_t destSize) {
    if ((src != NULL) && (etiqueta != NULL) && (dest != NULL)) {
        const char* p_start = strstr(src, etiqueta);
        if (p_start != NULL) {
            p_start += strlen(etiqueta);
            // Buscamos fin de línea o espacio según el formato del HTML
            const char* p_end = strpbrk(p_start, "< \r\n"); 
            
            size_t len = (p_end != NULL) ? (size_t)(p_end - p_start) : strlen(p_start);
            if (len >= (size_t)destSize) { len = (size_t)destSize - 1U; }
            
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

    /* Iniciar sincronización de tiempo con el sistema */
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);
}

void loop(void) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        (void)http.begin(URL_BASE);
        int16_t httpCode = (int16_t)http.GET();

        if (httpCode == (int16_t)HTTP_CODE_OK) {
            static char payload[MAX_BUFFER_WEB];
            static char temp[MAX_VAL_LEN];
            static char fecha_scrap[MAX_VAL_LEN];
            static char fecha_sys[MAX_VAL_LEN];
            static char dataLog[128];

            (void)strncpy(payload, http.getString().c_str(), sizeof(payload) - 1U);

            // 1. Extraer datos del Clima
            extraerDato(payload, "Temperatura: ", temp, (uint32_t)sizeof(temp));
            
            // 2. Extraer Fecha de la web (Ajusta "Actualizado: " según el HTML real)
            extraerDato(payload, "Actualizacion: ", fecha_scrap, (uint32_t)sizeof(fecha_scrap));

            // 3. Obtener fecha del sistema
            struct tm timeinfo;
            if(!getLocalTime(&timeinfo)){
                Serial.println("Error: System Time Fail");
            } else {
                // Formateamos fecha del sistema igual a la de la web (ej: DD/MM/YYYY)
                strftime(fecha_sys, sizeof(fecha_sys), "%d/%m/%Y", &timeinfo);

                // 4. VALIDACIÓN SOLICITADA
                // Si la fecha extraída no coincide con la del sistema
                if (strstr(fecha_scrap, fecha_sys) == NULL) {
                    Serial.println(F("Error Update")); 
                }
            }

            (void)snprintf(dataLog, sizeof(dataLog), "%s,%s,%s", 
                           fecha_sys, temp, fecha_scrap);

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
