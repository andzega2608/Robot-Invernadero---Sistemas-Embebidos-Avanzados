const express = require("express");
const mqtt = require("mqtt");
const http = require("http");
const socketIo = require("socket.io");
const { spawn } = require("child_process");

const app = express();
app.use( "/socket.io", express.static(__dirname + "/node_modules/socket.io/client-dist")
);
const server = http.createServer(app);

const io = socketIo(server);

app.use(express.static("pub"));
app.use(express.json());

const client = mqtt.connect("mqtt://10.147.13.15");

client.on("connect", () => {
  console.log("Conectado al broker MQTT");
});
//Cámara
app.get("/iniciar-cam", (req, res) => {
  try {
    const cam = spawn("python", ["cam.py"], {
      cwd: __dirname,
      detached: false,
      stdio: ["ignore", "pipe", "pipe"],
    });
    cam.unref();
    console.log("Cámara iniciada");
    res.send("Cámara iniciada correctamente.");
  } 
});

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


  socket.on("autoSensor", (status) => {
    client.publish("Sensor/Estado", status);
  });

  socket.on("fanSpeed", (valor) => {
    client.publish("Sensor/Velocidad", valor);
  });

  socket.on("disconnect", () => {
    console.log("Desconectado");
  });
});

client.on("message", (topic, message) => {
  const data = message.toString();
  console.log(`MQTT recibido ${topic}: ${data}`);

  if (topic === "Sensor/Temperatura") {
    io.emit("sensor", { tipo: "temp", valor: data });
  } else if (topic === "Sensor/Humedad") {
    io.emit("sensor", { tipo: "hum", valor: data });
  } else if (topic === "Sensor/Ventilador") {
    io.emit("sensor", { tipo: "fan", valor: data });
  } else if (topic === "Sensor/Estado") {
    io.emit("autoStatus", { status: data });
  } else if (topic === "Sensor/Velocidad") {
    io.emit("fanSpeedUpdate", { speed: data });
  } else if (topic === "Camara/Live") {
    io.emit("videoFrame", data);
  } else if (topic === "Camara/Lectura") {
    io.emit("qrDetectado", data);
  }
});

client.subscribe("Sensor/Temperatura");
client.subscribe("Sensor/Humedad");
client.subscribe("Sensor/Ventilador");
client.subscribe("Sensor/Estado");
client.subscribe("Sensor/Velocidad");
client.subscribe("Camara/Lectura");
client.subscribe("Camara/Live");
client.subscribe("Motor/Direction");

server.listen(3000, () => {
  console.log("Servidor en http://localhost:3000");
});
