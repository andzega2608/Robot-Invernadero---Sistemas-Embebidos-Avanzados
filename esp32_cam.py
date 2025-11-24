import cv2
import numpy as np
import pyzbar.pyzbar as pyzbar
import urllib.request
import paho.mqtt.client as mqtt
import base64
import time

# Configuración MQTT
broker_ip = "192.168.112.15"  # Reemplaza con la IP del broker MQTT
broker_port = 1883
topic_qr = "Camara/Lectura"
topic_live = "Camara/Live"

# Variables globales
url ="http://192.168.112.149/cam.jpg"
prev = ""
last_frame_time = 0
frame_interval = 0.1  # Enviar frame cada 100 ms (10 FPS)

# Configuración MQTT Client
client = mqtt.Client()
client.connect(broker_ip, broker_port, 60)
client.loop_start()

def send_frame(frame):
    """Codifica y envía el frame por MQTT"""
    # Redimensionar para reducir ancho de banda
    resized = cv2.resize(frame, (320, 240))
    
    # Codificar en JPEG con baja calidad para reducir tamaño
    encode_params = [cv2.IMWRITE_JPEG_QUALITY, 50]
    _, buffer = cv2.imencode('.jpg', resized, encode_params)
    
    # Convertir a base64 y enviar
    jpg_as_text = base64.b64encode(buffer).decode('utf-8')
    client.publish(topic_live, jpg_as_text)

cv2.namedWindow("QR Scanner", cv2.WINDOW_NORMAL)
cv2.resizeWindow("QR Scanner", 320, 240)

try:
    while True:
        # Capturar frame
        img_resp = urllib.request.urlopen(url, timeout=8)
        imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
        frame = cv2.imdecode(imgnp, -1)
        
        # Procesar QR
        decoded_objects = pyzbar.decode(frame)
        for obj in decoded_objects:
            current_data = obj.data.decode('utf-8')
            if prev != current_data:
                print(f"QR Detectado: {current_data}")
                client.publish(topic_qr, current_data)
                prev = current_data
        
        # Enviar frame cada intervalo (control de FPS)
        current_time = time.time()
        if current_time - last_frame_time > frame_interval:
            send_frame(frame)
            last_frame_time = current_time
        
        # Mostrar ventana
        cv2.imshow("QR Scanner", frame)
        
        if cv2.waitKey(1) & 0xFF == 27:
            break

finally:
    client.loop_stop()
    cv2.destroyAllWindows()