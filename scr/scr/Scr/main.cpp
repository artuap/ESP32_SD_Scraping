/**
 * @file ESP32_SD_Scraping.cpp
 * @author Arturo_AlonsoLP Linuxmatic
 * @brief Srap weather data and log in.
 * @version 0.1
 * @date 2026-03-30
 * * @copyright Copyright (c) 2026 MIT licence
 * */


#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>

// Configuración de Red
const char* ssid = "TU_SSID_WIFI";
const char* password = "TU_PASSWORD";
const char* url = "http://urban.diau.buap.mx/estaciones/ramm07/ramm07.php";

// Pines SD para ESP32 (Ajustar según tu placa)
#define SD_CS 5

// Función para extraer datos específicos del HTML
String extraerDato(String html, String etiqueta) {
  int posEtiqueta = html.indexOf(etiqueta);
  if (posEtiqueta == -1) return "N/A";

  // Buscamos el primer número después de la etiqueta (ej: "Temperatura: ")
  int inicio = posEtiqueta + etiqueta.length();
  int fin = html.indexOf(" ", inicio); // Buscamos el espacio o unidad (ºC, hPa, %)
  
  return html.substring(inicio, fin);
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar SD
  if(!SD.begin(SD_CS)){
    Serial.println("Error: Tarjeta SD no detectada.");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();

      // Extracción de los 3 valores específicos
      // Nota: Estas etiquetas deben coincidir exactas con el texto de la web
      String temp = extraerDato(payload, "Temperatura: ");
      String pres = extraerDato(payload, "Presion: ");
      String hum  = extraerDato(payload, "Humedad: ");

      // Formatear línea para el CSV
      String dataLog = String(millis()) + "," + temp + "," + pres + "," + hum;

      // Guardar en SD
      File dataFile = SD.open("/clima_buap.csv", FILE_APPEND);
      if (dataFile) {
        dataFile.println(dataLog);
        dataFile.close();
        Serial.println("Guardado: " + dataLog);
      } else {
        Serial.println("Error abriendo el archivo en SD");
      }
    }
    http.end();
  }
  
  delay(300000); // Esperar 5 minutos (300,000 ms) entre lecturas
}

