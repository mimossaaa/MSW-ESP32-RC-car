// Include necessary libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// #include <sstream> // No longer needed due to manual string parsing

// Motor control pins
#define IN1 26  // Right motor forward control pin
#define IN2 27  // Right motor backward control pin
#define IN3 14  // Left motor forward control pin
#define IN4 12  // Left motor backward control pin

// Define WiFi AP credentials
const char* ssid      = "MSW_minibot";   // WiFi SSID
const char* password  = "12345678";      // WiFi Password

// Define command constants for directions
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define FORWARD_LEFT 5
#define FORWARD_RIGHT 6
#define BACKWARD_LEFT 7
#define BACKWARD_RIGHT 8

// Define motor indexes
#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

// Define motor directions (for setMotorDirection function)
#define MOTOR_FORWARD 1  // Renamed to avoid conflict if FORWARD was globally something else
#define MOTOR_BACKWARD -1
#define MOTOR_STOP 0     // Explicit stop for motor direction

// Create AsyncWebServer on port 80 and WebSocket endpoint
AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

// HTML page served to clients - (No changes here from previous version you liked)
const char* htmlHomePage PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <title>MSW Robot Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    body {
      background-color: #fdfd96;
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      overflow: hidden;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      box-sizing: border-box;
    }
    .title-header {
      font-size: 28px;
      color: #4A4A4A;
      margin-bottom: 20px;
      text-align: center;
    }
    .layout {
      display: flex;
      flex-direction: row;
      justify-content: space-between;
      width: 90%;
      max-width: 600px;
      align-items: center;
    }
    .column {
      display: flex;
      flex-direction: column;
      gap: 20px;
    }
    .right-controls {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 20px;
    }
    .row {
      display: flex;
      flex-direction: row;
      gap: 20px;
    }
    .button {
      width: 120px;
      height: 120px;
      background-color: white;
      border-radius: 10px;
      box-shadow: 5px 5px #888888;
      display: flex;
      justify-content: center;
      align-items: center;
      user-select: none;
      font-size: 18px;
      font-weight: bold;
      text-align: center;
      cursor: pointer;
    }
    .button:active {
      transform: translate(5px, 5px);
      box-shadow: none;
    }
    .slider-container {
      width: 100%;
      text-align: center;
    }
    .slider {
      width: 100%;
    }
  </style>
</head>
<body>
  <h1 class="title-header">Mission Science Workshop</h1>

  <div class="layout">
    <div class="column">
      <div class="button" ontouchstart='send("MoveCar","1")' ontouchend='send("MoveCar","0")'>Forward</div>
      <div class="button" ontouchstart='send("MoveCar","2")' ontouchend='send("MoveCar","0")'>Backward</div>
    </div>
    <div class="right-controls">
      <div class="slider-container">
        <label for="Speed">Speed:</label><br>
        <input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='send("Speed",value)'>
      </div>
      <div class="row">
        <div class="button" ontouchstart='send("MoveCar","3")' ontouchend='send("MoveCar","0")'>Left</div>
        <div class="button" ontouchstart='send("MoveCar","4")' ontouchend='send("MoveCar","0")'>Right</div>
      </div>
    </div>
  </div>

  <script>
    var socket;
    function connect() {
      socket = new WebSocket("ws://" + location.host + "/CarInput");
      socket.onopen = () => {
        console.log("WebSocket connected");
        send("Speed", document.getElementById("Speed").value);
      };
      socket.onclose = (event) => {
        console.log("WebSocket disconnected. Code:", event.code, "Reason:", event.reason, "Will attempt to reconnect.");
        setTimeout(connect, 2000);
      };
      socket.onerror = (error) => {
        console.error('WebSocket Error:', error);
      };
    }
    function send(key, value) {
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(key + "," + value);
      } else {
         console.warn("WebSocket not open. Ready state:", socket ? socket.readyState : 'null');
      }
    }
    window.onload = connect;
    document.addEventListener('keydown', function(event) {
      let commandSent = false;
      switch (event.key.toLowerCase()) {
        case 'w': send("MoveCar", "1"); commandSent = true; break;
        case 's': send("MoveCar", "2"); commandSent = true; break;
        case 'a': send("MoveCar", "3"); commandSent = true; break;
        case 'd': send("MoveCar", "4"); commandSent = true; break;
      }
      if (commandSent) event.preventDefault();
    });
    document.addEventListener('keyup', function(event) {
      let stopCommandNeeded = false;
      switch (event.key.toLowerCase()) {
        case 'w': case 's': case 'a': case 'd':
          send("MoveCar", "0"); // STOP
          stopCommandNeeded = true;
          break;
      }
      if (stopCommandNeeded) event.preventDefault();
    });
  </script>
</body>
</html>
)HTML";

// Function to set the direction of a single motor
void setMotorDirection(int motorIndex, int direction) {
    if (motorIndex == RIGHT_MOTOR) {
        if (direction == MOTOR_FORWARD) {
            digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
        } else if (direction == MOTOR_BACKWARD) {
            digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
        } else { // MOTOR_STOP or any other value
            digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
        }
    } else if (motorIndex == LEFT_MOTOR) {
        if (direction == MOTOR_FORWARD) {
            digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
        } else if (direction == MOTOR_BACKWARD) {
            digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
        } else { // MOTOR_STOP or any other value
            digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
        }
    }
}

// Function to control the car's movement
void moveCar(int cmd) {
    Serial.printf("Received MoveCar command: %d\n", cmd);
    switch (cmd) {
        case UP: // Command for Forward
            setMotorDirection(RIGHT_MOTOR, MOTOR_FORWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_FORWARD);
            break;
        case DOWN: // Command for Backward
            setMotorDirection(RIGHT_MOTOR, MOTOR_BACKWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_BACKWARD);
            break;
        case LEFT: // Command for Pivot Left
            setMotorDirection(RIGHT_MOTOR, MOTOR_FORWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_BACKWARD);
            break;
        case RIGHT: // Command for Pivot Right
            setMotorDirection(RIGHT_MOTOR, MOTOR_BACKWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_FORWARD);
            break;
        case FORWARD_LEFT:
             setMotorDirection(RIGHT_MOTOR, MOTOR_FORWARD);
             setMotorDirection(LEFT_MOTOR, MOTOR_STOP);
             break;
        case FORWARD_RIGHT:
             setMotorDirection(RIGHT_MOTOR, MOTOR_STOP);
             setMotorDirection(LEFT_MOTOR, MOTOR_FORWARD);
             break;
        case BACKWARD_LEFT:
             setMotorDirection(RIGHT_MOTOR, MOTOR_BACKWARD);
             setMotorDirection(LEFT_MOTOR, MOTOR_STOP);
             break;
        case BACKWARD_RIGHT:
             setMotorDirection(RIGHT_MOTOR, MOTOR_STOP);
             setMotorDirection(LEFT_MOTOR, MOTOR_BACKWARD);
             break;
        case STOP: // Command for Stop
        default:
            setMotorDirection(RIGHT_MOTOR, MOTOR_STOP);
            setMotorDirection(LEFT_MOTOR, MOTOR_STOP);
            break;
    }
}

// Function to serve the main HTML page
void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

// WebSocket event handler --- MODIFIED SECTION ---
void onCarInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                              AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client #%u disconnected\n", client->id());
    moveCar(STOP); // Stop car if client disconnects
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      // Create a std::string from the received data with explicit length
      std::string payload(reinterpret_cast<char*>(data), len);
      // Serial.printf("WS Data from client #%u: %s\n", client->id(), payload.c_str()); // For debugging

      // Manually parse the "key,value" string
      size_t comma_pos = payload.find(',');
      if (comma_pos != std::string::npos && comma_pos > 0 && comma_pos < payload.length() - 1) {
        std::string key = payload.substr(0, comma_pos);
        std::string value_str = payload.substr(comma_pos + 1);

        if (key == "MoveCar") {
          int val = atoi(value_str.c_tostr()); 
          moveCar(val);
        } else if (key == "Speed") {
          Serial.printf("Received Speed command with value %s, ignored.\n", value_str.c_str());
        } else {
          Serial.printf("Unknown WS key: %s\n", key.c_str());
        }
      } else {
        Serial.printf("Malformed WS message from client #%u: %s\n", client->id(), payload.c_str());
      }
    }
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("WebSocket client #%u error #%u: %s\n", client->id(), *((uint16_t*)arg), (char*)data);
  }
}
// --- END OF MODIFIED SECTION ---

// Setup function
void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  setMotorDirection(RIGHT_MOTOR, MOTOR_STOP);
  setMotorDirection(LEFT_MOTOR, MOTOR_STOP);

  Serial.printf("Setting up WiFi Access Point '%s'\n", ssid);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("HTTP server started");
}

// Loop function
void loop() {
  // AsyncWebServer and WebSocket handle things in the background
  // wsCarInput.cleanupClients(); // Optional: uncomment if you experience client disconnect issues over time
}
