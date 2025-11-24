#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>

const char* WIFI_SSID = "AndresProyecto";
const char* WIFI_PASS = "andresZT8";

WebServer server(80);

// Usar solo UNA resolución baja para minimizar procesamiento
static auto lowRes = esp32cam::Resolution::find(320, 240);

void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    server.send(503, "", "");
    return;
  }
  
  WiFiClient client = server.client();
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  frame->writeTo(client);
}

void handleJpg() {
  serveJpg();
}

void setup() {
  Serial.begin(115200);
  
  // Configuración mínima de cámara
  esp32cam::Config cfg;
  cfg.setPins(esp32cam::pins::AiThinker);
  cfg.setResolution(lowRes);
  cfg.setBufferCount(2);      // Mínimo de buffers
  cfg.setJpeg(30);            // Calidad JPEG muy baja para menos datos
  
  esp32cam::Camera.begin(cfg);  // ✅ CORREGIDO: Namespace agregado

  // WiFi con máxima optimización
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE); // Sin power saving
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  // Solo un endpoint simple
  server.on("/cam.jpg", handleJpg);
  server.begin();

  Serial.print("URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/cam.jpg");
}

void loop() {
  server.handleClient();
}