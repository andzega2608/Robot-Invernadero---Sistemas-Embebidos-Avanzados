const express = require("express");
const mqtt = require("mqtt");
const http = require("http");
const socketIo = require("socket.io");

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

//Express para almacenar .html
app.use(express.static("pub"));
app.use(express.json());

//Conexión Mqtt
const client = mqtt.connect("mqtt://192.168.246.15");

client.on("connect", () => {
  console.log("Conectado al broker MQTT");
});

// Conexión Socket.io
io.on("connection", (socket) => {
  console.log("Cliente conectado via Socket.io");

  // Movimiento
  socket.on("comando", (cmd) => {
    console.log("Acción recibido:", cmd);
    client.publish("Motor/Direction", cmd);
  });

  // Plaguicida
  socket.on("servo", (action) => {
    console.log("Acción recibida:", action);
    client.publish("Motor/Direction", action);
  });

  // Navegación Automática
  socket.on("navegar", (autom) => {
    console.log("Acción recibida", autom);
    client.publish("Motor/Direction", autom);
  });
  //Estado ventilador
  socket.on("autoSensor", (status) => {
    console.log("Acción recibida:", status);
    client.publish("Sensor/Estado", status);
  });
  //Veloidad Ventilador
  socket.on("fanSpeed", (valor) => {
    console.log("Acción recibida:", valor);
    client.publish("Sensor/Estado", valor);
  });

  socket.on("disconnect", () => {
    console.log("Desconectado");
  });
});

client.on("message", (topic, message) => {
  const data = message.toString();
  console.log(`MQTT recibido  ${topic}: ${data}`);

  //Senosres
  if (topic === "Sensor/Temperatura") {
    io.emit("sensor", { tipo: "temp", valor: data });
  } else if (topic === "Sensor/Humedad") {
    io.emit("sensor", { tipo: "hum", valor: data });
  } else if (topic === "Sensor/Ventilador") {
    io.emit("sensor", { tipo: "fan", valor: data });
  } else if (topic === topicAuto) {
    io.emit("autoStatus", { status: data });
    console.log("Estado automático:", data);
  } else if (topic === topicVelocidad) {
    io.emit("fanSpeedUpdate", { speed: data });
    console.log("Velocidad del ventilador:", data);
  }
});

// Subscribirse a los tópicos
client.subscribe("Sensor/Temperatura");
client.subscribe("Sensor/Humedad");
client.subscribe("Sensor/Ventilador");
client.subscribe("Sensor/Estado");
client.subscribe("Sensor/Velocidad");

//Iniciar el servidor
server.listen(3000, () => {
  console.log("Servidor en http://localhost:3000");
});
