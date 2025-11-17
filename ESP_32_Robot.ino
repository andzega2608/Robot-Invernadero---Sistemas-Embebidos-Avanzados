// Declaración de librerías
#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

// Declaración de SSID y Broker
const char* ssid = "AndresProyecto";
const char* password = "andresZT8";
const char* mqtt_server = "192.168.246.15";

// Declaración de Tópicos
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

// Declaración de sensores ultrasónicos
const int pinTrigIzq = 13;
const int pinEchoIzq = 14;
const int pinTrigDer = 26;
const int pinEchoDer = 33;

// Variables de distancia y control autónomo
float distanciaIzq = 0;
float distanciaDer = 0;
const float margen = 5.0;
int velocidad = 100;
const int velMin = 60;
const int velMax = 140;
bool modoAutomatico = false;

// Control PWM
const int canalA = 7;
const int canalB = 8;
const int frecuencia = 1000;
const int resolucion = 8;

const int motorSpeed = 255;

const int relePin = 4;

// Función para conexión con WiFi
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

// Función para limitar velocidad
int speedLimit(int v) {
  if (v < velMin) return velMin;
  if (v > velMax) return velMax;
  return v;
}

// Funciones de movimiento
void foward(int v) {
  v = speedLimit(v);
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, HIGH);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, HIGH);
  ledcWrite(canalA, v);
  ledcWrite(canalB, v);
}

void stopMotors() {
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, LOW);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalA, 0);
  ledcWrite(canalB, 0);
}

void turnLeft(int v) {
  v = speedLimit(v);
  digitalWrite(pinIN1, HIGH);
  digitalWrite(pinIN2, LOW);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, HIGH);
  ledcWrite(canalA, v / 2);
  ledcWrite(canalB, v);
}

void turnRight(int v) {
  v = speedLimit(v);
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, HIGH);
  digitalWrite(pinIN3, HIGH);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalA, v);
  ledcWrite(canalB, v / 2);
}

// Funciones ultrasónicas
unsigned long leerDistanciaUltrasonido(int pinTrigger, int pinEcho) {
  pinMode(pinTrigger, OUTPUT);
  digitalWrite(pinTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);
  pinMode(pinEcho, INPUT);
  return pulseIn(pinEcho, HIGH, 30000);
}

float distanciaCM(int pinTrig, int pinEcho) {
  unsigned long duracion = leerDistanciaUltrasonido(pinTrig, pinEcho);
  return 0.01723 * duracion;
}

// Función de navegación autónoma
void controlCentrado() {
  distanciaIzq = distanciaCM(pinTrigIzq, pinEchoIzq);
  distanciaDer = distanciaCM(pinTrigDer, pinEchoDer);
  Serial.print("Izquierda: ");
  Serial.print(distanciaIzq);
  Serial.print(" cm | Derecha: ");
  Serial.print(distanciaDer);
  Serial.println(" cm");

  if (distanciaIzq < 5 && distanciaDer < 5) {
    stopMotors();
    Serial.println("Deteniendo");
  }
  else if (abs(distanciaIzq - distanciaDer) <= margen) {
    foward(velocidad);
    Serial.println("Avanza recto");
  }
  else if (distanciaIzq < distanciaDer) {
    turnRight(velocidad);
    Serial.println("Girando derecha");
  }
  else {
    turnLeft(velocidad);
    Serial.println("Girando izquierda");
  }
}

// Control manual desde MQTT
void applyMotorControl(String direction) {
  if (direction == "A") {
    modoAutomatico = true;
    Serial.println("Modo automático");
    return;
  }
  if (direction == "S") {
    modoAutomatico = false;
    stopMotors();
    return;
  }

  if (!modoAutomatico) {
    if (direction == "B") {
      digitalWrite(pinIN1, HIGH);
      digitalWrite(pinIN2, LOW);
      digitalWrite(pinIN3, HIGH);
      digitalWrite(pinIN4, LOW);
      ledcWrite(canalA, motorSpeed);
      ledcWrite(canalB, motorSpeed);
    }
    else if (direction == "F") {
      digitalWrite(pinIN1, LOW);
      digitalWrite(pinIN2, HIGH);
      digitalWrite(pinIN3, LOW);
      digitalWrite(pinIN4, HIGH);
      ledcWrite(canalA, motorSpeed);
      ledcWrite(canalB, motorSpeed);
    }
    else if (direction == "L") {
      digitalWrite(pinIN1, HIGH);
      digitalWrite(pinIN2, LOW);
      digitalWrite(pinIN3, LOW);
      digitalWrite(pinIN4, HIGH);
      ledcWrite(canalA, motorSpeed);
      ledcWrite(canalB, motorSpeed);
    }
    else if (direction == "R") {
      digitalWrite(pinIN1, LOW);
      digitalWrite(pinIN2, HIGH);
      digitalWrite(pinIN3, HIGH);
      digitalWrite(pinIN4, LOW);
      ledcWrite(canalA, motorSpeed);
      ledcWrite(canalB, motorSpeed);
    }
    else if (direction == "P") {
      digitalWrite(relePin, LOW);
      delay(2000);
      digitalWrite(relePin, HIGH);
    }
  }
}

// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  if (String(topic) == topicDirection) {
    applyMotorControl(message);
  }
}

// Función de reconexión con el broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando al broker...");
    if (client.connect("ESP32Client2")) {
      Serial.println("Conectado");
      client.subscribe(topicDirection);
      Serial.println("Suscrito a tópicos");
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// SETUP
void setup() {
  Serial.begin(115200);
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  pinMode(pinIN3, OUTPUT);
  pinMode(pinIN4, OUTPUT);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH);
  ledcSetup(canalA, frecuencia, resolucion);
  ledcSetup(canalB, frecuencia, resolucion);
  ledcAttachPin(pinENA, canalA);
  ledcAttachPin(pinENB, canalB);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// LOOP PRINCIPAL
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  if (modoAutomatico) {
    controlCentrado();
  }
}
