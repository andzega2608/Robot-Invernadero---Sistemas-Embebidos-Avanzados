#include <Arduino.h>

const int pinENA = 5;   // PWM Motor A
const int pinENB = 23;  // PWM Motor B

const int pinIN1 = 22;  // Motor A
const int pinIN2 = 21;  // Motor A
const int pinIN3 = 19;  // Motor B
const int pinIN4 = 18;  // Motor B

const int canalA = 0;
const int canalB = 1;
const int frecuencia = 1000; 
const int resolucion = 8;      

const int pinTrigIzq = 26;  
const int pinEchoIzq = 33;
const int pinTrigDer = 13;  
const int pinEchoDer = 14;

int infrarrojo = 32;

float distanciaIzq = 0;
float distanciaDer = 0;
const float ref = 20.0;   // Distancia de referencia (cm)
const float margen = 5.0; // Margen de tolerancia

int velocidad = 140;       // Velocidad inicial
const int velMin = 100;    // Límite inferior
const int velMax = 180;    // Límite superior

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
  return 0.01723 * duracion; // cm
}

int limitarVelocidad(int v) {
  if (v < velMin) 
    return velMin;
  if (v > velMax) 
    return velMax;
  return v;
}

void adelante(int v) {
  v = limitarVelocidad(v);
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, HIGH);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, HIGH);
  ledcWrite(canalA, v);
  ledcWrite(canalB, v);
}

void parar() {
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, LOW);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalA, 0);
  ledcWrite(canalB, 0);
}

void girarIzquierda(int v) {
  v = limitarVelocidad(v);
  digitalWrite(pinIN1, HIGH);
  digitalWrite(pinIN2, LOW);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, HIGH);
  ledcWrite(canalA, v / 2);
  ledcWrite(canalB, v);
}

void girarDerecha(int v) {
  v = limitarVelocidad(v);
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, HIGH);
  digitalWrite(pinIN3, HIGH);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalA, v);
  ledcWrite(canalB, v / 2);
}

void recto(){
  if (distanciaIzq < 5 && distanciaDer < 5) {
    parar();
    Serial.println("Detenido.");
  } 
  else if (abs(distanciaIzq - distanciaDer) <= margen) {
    adelante(velocidad);
    Serial.println("Avanza recto.");
  } 
  else if (distanciaIzq < distanciaDer) {
    girarDerecha(velocidad);
    Serial.println("Girando derecha.");
  } 
  else if (distanciaDer < distanciaIzq) {
    girarIzquierda(velocidad);
    Serial.println("Girando izquierda.");
  }
  delay(200);
}

void giro(){
  int valor = digitalRead(infrarrojo);
  while(valor != HIGH){
    digitalWrite(pinIN1, HIGH);
    digitalWrite(pinIN2, LOW);
    digitalWrite(pinIN3, LOW);
    digitalWrite(pinIN4, HIGH);
    ledcWrite(canalA, velocidad);
    ledcWrite(canalB, velocidad);
    valor = digitalRead(infrarrojo);
  }
  parar();
  delay(1000);
  digitalWrite(pinIN1, HIGH);
  digitalWrite(pinIN2, LOW);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, HIGH);
  ledcWrite(canalA, velocidad);
  ledcWrite(canalB, velocidad);
  delay(500);  
  parar();
  delay(1000);
}

void giroDer(int v){
  v = limitarVelocidad(v);
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, HIGH);
  digitalWrite(pinIN3, HIGH);
  digitalWrite(pinIN4, LOW);
  ledcWrite(canalA, v);
  ledcWrite(canalB, v);
}

void giroIzq(int v){
  v = limitarVelocidad(v);
  digitalWrite(pinIN1, HIGH);
  digitalWrite(pinIN2, LOW);
  digitalWrite(pinIN3, LOW);
  digitalWrite(pinIN4, HIGH);
  ledcWrite(canalA, v);
  ledcWrite(canalB, v);
}

void setup() {
  Serial.begin(115200);
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  pinMode(pinIN3, OUTPUT);
  pinMode(pinIN4, OUTPUT);

  ledcSetup(canalA, frecuencia, resolucion);
  ledcSetup(canalB, frecuencia, resolucion);
  ledcAttachPin(pinENA, canalA);
  ledcAttachPin(pinENB, canalB);

  pinMode(infrarrojo, INPUT_PULLUP);
  delay(3000);
}

void loop() {
  distanciaIzq = distanciaCM(pinTrigIzq, pinEchoIzq);
  distanciaDer = distanciaCM(pinTrigDer, pinEchoDer);
  Serial.print("Izquierda: ");
  Serial.print(distanciaIzq);
  Serial.print(" cm | Derecha: ");
  Serial.print(distanciaDer);
  Serial.println(" cm");
  if (digitalRead(infrarrojo) == LOW){
    Serial.println("Detectado");
    parar();
    delay(1000);
    if ((distanciaIzq > 20) || (distanciaDer > 20)){
      Serial.println("Interseccion");
      distanciaIzq = distanciaCM(pinTrigIzq, pinEchoIzq);
      distanciaDer = distanciaCM(pinTrigDer, pinEchoDer);
      if ((distanciaDer > 20) && (distanciaIzq < 20)){
        Serial.println("Solo derecha");
        giroDer(velocidad);
        delay(500);
        parar();
        delay(1000);
        adelante(velocidad);
        delay(250);
      }
      else if ((distanciaDer < 20) && (distanciaIzq > 20)){
        Serial.println("Solo izquierda");
        giroIzq(velocidad);
        delay(500);
        parar();
        delay(1000);
        adelante(velocidad);
        delay(250);
      }
      else {
        Serial.println("Interseccion T");
        giroDer(velocidad);
        delay(500);
        parar();
        delay(1000);
        adelante(velocidad);
        delay(250);
      } 
    }
    else {
      Serial.println("Punto Muerto");
      giro();
      parar();
      delay(1000);
      giroDer(velocidad);
      delay(250);
    }
  }
  else {
    if ((distanciaIzq > 20) || (distanciaDer > 20)){
      Serial.println("Interseccion");
      parar();
      delay(1000);
      adelante(velocidad);
      delay(250);
      parar();
      delay(1000);
      distanciaIzq = distanciaCM(pinTrigIzq, pinEchoIzq);
      distanciaDer = distanciaCM(pinTrigDer, pinEchoDer);
      if ((distanciaDer > 20) && (distanciaIzq < 20)){
        Serial.println("Adelante o derecha");
        giroDer(velocidad);
        delay(500);
        parar();
        delay(1000);
      }
      else if ((distanciaDer < 20) && (distanciaIzq > 20)){
        Serial.println("Adelante o izquierda");
        adelante(velocidad);
        delay(500);
        parar();
        delay(1000);
      }
      else {
        Serial.println("Cruz");
        giroDer(velocidad);
        delay(500);
        parar();
        delay(1000);
      } 
    }
    else {
      recto();
    }
  }
}
