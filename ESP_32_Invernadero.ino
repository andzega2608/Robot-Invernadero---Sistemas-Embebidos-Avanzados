// Declaración de librerias
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

// Declaración de SSID y Broker
const char* ssid = "AndresProyecto";
const char* password = "andresZT8";
const char* mqtt_server = "192.168.84.15";

// Declaracion de Tópicos
const char* topicTemperatura = "Sensor/Temperatura";
const char* topicHumedad = "Sensor/Humedad";
const char* topicVentilador = "Sensor/Ventilador";

// Declaración de pines
const int pinDHT = 23;
const int PIN_ENA = 18;
const int PIN_IN1 = 22;
const int PIN_IN2 = 21;

// Declaración de control de puente H
const int canalA = 0;
const int frecuencia = 1000;
const int resolucion = 8;

// Variables límites de temperatura
const float hot_temp  = 90.0;
const float semi_temp = 70.0;
String estadoVentilador = "APAGADO";
float lastTemp = 0;
float lastHum  = 0;
String lastEstadoVent = "";

WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;

// Función de Conexión a Wifi
void setup_wifi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Función callback
void callback(char* topic, byte* payload, unsigned int length) {}

// Funnción de reconexión MQTT con ESP32 
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando al broker...");
    if (client.connect("ESP32Client1")) {
      Serial.println("Conectado");
      client.subscribe(topicTemperatura);
      client.subscribe(topicHumedad);
      client.subscribe(topicVentilador);
      Serial.println("Suscrito a tópicos");
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// Función de control, para no saturar al broker con información, solo se mandan aquellos datos que cambian significativamente
void applyMotorCooler() {
  TempAndHumidity data = dht.getTempAndHumidity();
  if (isnan(data.temperature) || isnan(data.humidity)) return;
  // Solo se manda al broker si la temperatura varia 0.5 unidades
  if (abs(data.temperature - lastTemp) >= 0.5) {
    client.publish(topicTemperatura, String(data.temperature, 1).c_str());
    lastTemp = data.temperature;
  }
  // Solo se manda al broker si la humedad varia en 1 unidad
  if (abs(data.humidity - lastHum) >= 1.0) {
    client.publish(topicHumedad, String(data.humidity, 1).c_str());
    lastHum = data.humidity;
  }
  String nuevoEstado;
  if (data.humidity >= hot_temp) { // Si se supera la temperatura maxima
    ledcWrite(canalA, 255);
    nuevoEstado = "MAXIMO";
  } else if (data.humidity >= semi_temp) { // Si se supera la temperatura media
    ledcWrite(canalA, 150);
    nuevoEstado = "MEDIO";
  } else {
    ledcWrite(canalA, 80); // Si se mantiene la temperatura, se mantiene constante
    nuevoEstado = "BAJO";
  }
  // Mnadamos el nuevo estado del motor
  if (nuevoEstado != lastEstadoVent) {
    estadoVentilador = nuevoEstado;
    client.publish(topicVentilador, estadoVentilador.c_str());
    lastEstadoVent = nuevoEstado;
  }
  Serial.println("Temperatura: " + String(data.temperature,1));
  Serial.println("Humedad: " + String(data.humidity,1));
  Serial.println("Ventilador: " + estadoVentilador);
}

void setup() {
  // Declaración de variables iniciales
  Serial.begin(115200);
  dht.setup(pinDHT, DHTesp::DHT11);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  ledcSetup(canalA, frecuencia, resolucion);
  ledcAttachPin(PIN_ENA, canalA);
  // Establecemos conexión con Wifi y servidor
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Se llama a la función de control del ventilador
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  applyMotorCooler();
}
