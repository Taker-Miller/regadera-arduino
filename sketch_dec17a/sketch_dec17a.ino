#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// Configuración de red Wi-Fi
const char* ssid = "Susana 2,4GHZ";
const char* password = "73909106";

// Pines de la bomba y sensor de humedad
#define SENSOR_PIN A0
#define IN1 D1 // Pin de control para encender
#define IN2 D2 // Pin de control para apagar

// Configuración de ThingSpeak
const char* writeApiUrl = "http://api.thingspeak.com/update";  // URL para enviar datos
const char* readApiUrl = "http://api.thingspeak.com/channels/2788742/fields/2/last.json?api_key=HT1VIZBAEPM96DD5";
const char* apiKey = "VOPIXI0UYLESDSSK";                      // Write API Key

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Bomba apagada por defecto
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  Serial.println("Bomba apagada al iniciar.");

  // Conexión a Wi-Fi
  Serial.println("Conectando al Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado.");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    // **1. Leer humedad del sensor y enviar datos a ThingSpeak (field1)**
    int valorHumedad = analogRead(SENSOR_PIN);
    float porcentajeHumedad = map(valorHumedad, 1023, 0, 0, 100);
    porcentajeHumedad = constrain(porcentajeHumedad, 0, 100);
    Serial.print("Humedad del suelo: ");
    Serial.print(porcentajeHumedad);
    Serial.println("%");

    // Enviar datos de humedad a ThingSpeak
    String url = String(writeApiUrl) + "?api_key=" + apiKey + "&field1=" + String(porcentajeHumedad, 2);
    http.begin(client, url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Humedad enviada correctamente a ThingSpeak.");
    } else {
      Serial.println("Error al enviar los datos de humedad.");
    }
    http.end();

    // **2. Leer estado de la bomba desde ThingSpeak (field2)**
    http.begin(client, readApiUrl);
    int responseCode = http.GET();

    if (responseCode > 0) {
      String payload = http.getString();
      Serial.println("Respuesta de ThingSpeak: " + payload);

      // Parsear el JSON
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Error al parsear JSON: ");
        Serial.println(error.c_str());
      } else {
        String fieldValue = doc["field2"] | "0";  // Leer field2
        Serial.print("Valor recibido (field2): ");
        Serial.println(fieldValue);

        // **Controlar la bomba manualmente**
        if (fieldValue == "1") {
          Serial.println("Encendiendo la bomba. Agua fluyendo...");
          // Asegurar que la bomba esté en modo de encendido
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          delay(1000);  // Ajustar duración del flujo de agua según necesidad
        } else if (fieldValue == "0") {
          Serial.println("Apagando la bomba.");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
        } else {
          Serial.println("Valor inválido, apagando la bomba por seguridad.");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
        }
      }
    } else {
      Serial.println("Error al obtener datos de ThingSpeak.");
      // Seguridad: Apagar la bomba si hay error
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    http.end();
  } else {
    Serial.println("Error: Wi-Fi desconectado. Intentando reconectar...");
    WiFi.begin(ssid, password);  // Intentar reconexión Wi-Fi
  }

  delay(15000);  // Espera 15 segundos antes de la siguiente iteración
}
