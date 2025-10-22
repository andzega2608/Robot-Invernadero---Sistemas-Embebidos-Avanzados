// Declaración de librerias
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ESP32Servo.h>

// Declaración de SSID y Broker
const char* ssid = "AndresProyecto";
const char* password = "andresZT8";
const char* mqtt_server = "192.168.84.15";

// Declaracion de Tópicos
const char* topicDirection = "Motor/Direction";

WiFiClient espClient;
PubSubClient client(espClient);

// Declaración de pines
const int pinENA = 5;
const int pinENB = 23;
const int pinIN1 = 22;
const int pinIN2 = 21;
const int pinIN3 = 19;
const int pinIN4 = 18;

// Declaración de control de puente H
const int canalA = 0;
const int canalB = 1;
const int frecuencia = 1000;
const int resolucion = 8;

const int motorSpeed = 145;  // Velocidad fija

const int servoPin = 13;
Servo servo;

// Función para conexión con Wifi
void setup_wifi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Conectado a WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Función para control del motor
void applyMotorControl(String direction) {
  if (direction == "B") { // Si recibe una B, retrocede
    digitalWrite(pinIN1, HIGH);
    digitalWrite(pinIN2, LOW);
    digitalWrite(pinIN3, HIGH);
    digitalWrite(pinIN4, LOW);
    ledcWrite(canalA, motorSpeed);
    ledcWrite(canalB, motorSpeed);
  } 
  else if (direction == "F") { // Si recibe una F, avanza
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, HIGH);
    digitalWrite(pinIN3, LOW);
    digitalWrite(pinIN4, HIGH);
    ledcWrite(canalA, motorSpeed);
    ledcWrite(canalB, motorSpeed);
  } 
  else if (direction == "L") { // Si recibe una L, gira a la izquierda
    digitalWrite(pinIN1, HIGH);
    digitalWrite(pinIN2, LOW);
    digitalWrite(pinIN3, LOW);
    digitalWrite(pinIN4, HIGH);
    ledcWrite(canalA, motorSpeed);
    ledcWrite(canalB, motorSpeed);
  } 
  else if (direction == "R") { // Si recibe una R, gira a la derecha
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, HIGH);
    digitalWrite(pinIN3, HIGH);
    digitalWrite(pinIN4, LOW);
    ledcWrite(canalA, motorSpeed);
    ledcWrite(canalB, motorSpeed);
  }
  else if (direction == "STOP") { // Caso contrario se detiene
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, LOW);
    digitalWrite(pinIN3, LOW);
    digitalWrite(pinIN4, LOW);
    ledcWrite(canalA, 0);
    ledcWrite(canalB, 0);
    servo.write(180);
    delay(1000);
  }
}

// Función que llama al tópico
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  applyMotorControl(message);
}

// Función de reconexión con el broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando al broker...");
    if (client.connect("ESP32Client2")) {
      Serial.println("Conectado");
      client.subscribe(topicSpeed);
      client.subscribe(topicDirection);
      Serial.println("Suscrito a tópicos");
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  // Declaración de variables iniciales
  Serial.begin(115200);
  servo.attach(servoPin);
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  pinMode(pinIN3, OUTPUT);
  pinMode(pinIN4, OUTPUT);
  ledcSetup(canalA, frecuencia, resolucion);
  ledcSetup(canalB, frecuencia, resolucion);
  ledcAttachPin(pinENA, canalA);
  ledcAttachPin(pinENB, canalB);
  // Establecemos conexión con Wifi y servidor
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Se llama a la función de control de los motores
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
}
 