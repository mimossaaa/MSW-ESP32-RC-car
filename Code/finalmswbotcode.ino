// Include necessary libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <sstream> // Required for std::istringstream

// Motor control pins from Program 2
#define IN1 26  // Right motor forward control pin
#define IN2 27  // Right motor backward control pin
#define IN3 14  // Left motor forward control pin
#define IN4 12  // Left motor backward control pin

// Define WiFi AP credentials (using credentials from Program 1)
const char* ssid      = "MSW_minibot";   // WiFi SSID
const char* password  = "12345678";      // WiFi Password

// Define command constants for directions (from Program 1)
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

// Note: FORWARD_LEFT, FORWARD_RIGHT, BACKWARD_LEFT, BACKWARD_RIGHT
// are defined but not currently used by the provided HTML.
// If the HTML were updated to send these, the moveCar function
// would need to be extended to handle them with the new pins.
#define FORWARD_LEFT 5
#define FORWARD_RIGHT 6
#define BACKWARD_LEFT 7
#define BACKWARD_RIGHT 8


// Define motor indexes (conceptual for clarity in functions)
#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

// Define motor directions (from Program 1)
#define FORWARD 1
#define BACKWARD -1
// STOP is already defined as 0

// Create AsyncWebServer on port 80 and WebSocket endpoint
AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

// HTML page served to clients (from Program 1)
// Note: The speed slider in this HTML will not function as PWM
// control is removed to be compatible with the new pin logic.
const char* htmlHomePage PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    /* Styling the control UI */
    body {
      background-color: #fdfd96;
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      overflow: hidden;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
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
  <div class="layout">
    <div class="column">
      <div class="button" ontouchstart='send("MoveCar","1")' ontouchend='send("MoveCar","0")'></div>
      <div class="button" ontouchstart='send("MoveCar","2")' ontouchend='send("MoveCar","0")'></div>
    </div>
    <div class="right-controls">
      <div class="slider-container">
        <label for="Speed">Speed:</label><br>
        <input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='send("Speed",value)'>
      </div>
      <div class="row">
        <div class="button" ontouchstart='send("MoveCar","3")' ontouchend='send("MoveCar","0")'></div>
        <div class="button" ontouchstart='send("MoveCar","4")' ontouchend='send("MoveCar","0")'></div>
      </div>
    </div>
  </div>

  <script>
    var socket;
    // Establish WebSocket connection
    function connect() {
      socket = new WebSocket("ws://" + location.host + "/CarInput");
      socket.onopen = () => {
        // Initial speed setting is sent, but currently ignored by the ESP32 code
        send("Speed", document.getElementById("Speed").value);
      };
      socket.onclose = () => setTimeout(connect, 2000); // Reconnect if disconnected
       socket.onerror = (error) => console.error('WebSocket Error:', error); // Log errors
    }

    // Send key-value pair through WebSocket
    function send(key, value) {
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(key + "," + value);
         // Optional: Add visual feedback or log
        // console.log("Sent:", key, value);
      } else {
         console.warn("WebSocket not open. Ready state:", socket ? socket.readyState : 'null');
      }
    }

    window.onload = connect; // Connect on page load
  </script>
</body>
</html>
)HTML";

// Function to set the direction of a single motor using the new pins
// This implements standard H-bridge logic for forward, backward, stop.
void setMotorDirection(int motorIndex, int direction) {
    if (motorIndex == RIGHT_MOTOR) {
        if (direction == FORWARD) {
            digitalWrite(IN1, HIGH); // IN1 HIGH, IN2 LOW for Right Forward
            digitalWrite(IN2, LOW);
        } else if (direction == BACKWARD) {
            digitalWrite(IN1, LOW);  // IN1 LOW, IN2 HIGH for Right Backward
            digitalWrite(IN2, HIGH);
        } else { // STOP
            digitalWrite(IN1, LOW);  // Both LOW for Stop
            digitalWrite(IN2, LOW);
        }
    } else if (motorIndex == LEFT_MOTOR) {
        if (direction == FORWARD) {
            digitalWrite(IN3, HIGH); // IN3 HIGH, IN4 LOW for Left Forward
            digitalWrite(IN4, LOW);
        } else if (direction == BACKWARD) {
            digitalWrite(IN3, LOW);  // IN3 LOW, IN4 HIGH for Left Backward
            digitalWrite(IN4, HIGH);
        } else { // STOP
            digitalWrite(IN3, LOW);  // Both LOW for Stop
            digitalWrite(IN4, LOW);
        }
    }
    // Ignore invalid motorIndex
}


// Function to control the car's movement based on input command
// This implements tank steering using the new setMotorDirection function.
void moveCar(int cmd) {
    Serial.printf("Received MoveCar command: %d\n", cmd); // Debug print
    switch (cmd) {
        case UP: // Both motors forward
            setMotorDirection(RIGHT_MOTOR, FORWARD);
            setMotorDirection(LEFT_MOTOR, FORWARD);
            break;
        case DOWN: // Both motors backward
            setMotorDirection(RIGHT_MOTOR, BACKWARD);
            setMotorDirection(LEFT_MOTOR, BACKWARD);
            break;
        case LEFT: // Pivot Left (Right forward, Left backward)
            setMotorDirection(RIGHT_MOTOR, FORWARD);
            setMotorDirection(LEFT_MOTOR, BACKWARD);
            break;
        case RIGHT: // Pivot Right (Right backward, Left forward)
            setMotorDirection(RIGHT_MOTOR, BACKWARD);
            setMotorDirection(LEFT_MOTOR, FORWARD);
            break;
        // Note: The HTML sends 1-4 for directional buttons, and 0 on button release.
        // The following cases (5-8) are defined but not triggered by the default HTML.
        case FORWARD_LEFT: // Right motor forward, Left stopped
             setMotorDirection(RIGHT_MOTOR, FORWARD);
             setMotorDirection(LEFT_MOTOR, STOP);
             break;
        case FORWARD_RIGHT: // Right stopped, Left motor forward
             setMotorDirection(RIGHT_MOTOR, STOP);
             setMotorDirection(LEFT_MOTOR, FORWARD);
             break;
        case BACKWARD_LEFT: // Right motor backward, Left stopped
             setMotorDirection(RIGHT_MOTOR, BACKWARD);
             setMotorDirection(LEFT_MOTOR, STOP);
             break;
        case BACKWARD_RIGHT: // Right stopped, Left motor backward
             setMotorDirection(RIGHT_MOTOR, STOP);
             setMotorDirection(LEFT_MOTOR, BACKWARD);
             break;
        case STOP: // Both motors stop
        default:
            setMotorDirection(RIGHT_MOTOR, STOP);
            setMotorDirection(LEFT_MOTOR, STOP);
            break;
    }
}

// Function to serve the main HTML page
void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}

// WebSocket event handler
void onCarInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                              AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client #%u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client #%u disconnected\n", client->id());
    moveCar(STOP); // Stop car if client disconnects
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      // Null-terminate the data to treat it as a string
      data[len] = 0;
      std::string msg((char *)data); // Parse message

      std::istringstream ss(msg);
      std::string key, value_str;
      getline(ss, key, ',');
      getline(ss, value_str, ',');

      if (key == "MoveCar") {
        int val = atoi(value_str.c_str()); // Convert value to int
        moveCar(val);
      } else if (key == "Speed") {
        // Speed control is not implemented with the new pins using simple digital writes.
        // This command is ignored.
        Serial.printf("Received Speed command with value %s, ignored.\n", value_str.c_str());
      }
    }
  }
}

// Setup function
void setup() {
  Serial.begin(115200); // Start serial communication
  Serial.println("Booting...");

  // Set motor pins as outputs from Program 2
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Ensure motors are stopped initially
  setMotorDirection(RIGHT_MOTOR, STOP);
  setMotorDirection(LEFT_MOTOR, STOP);


  // Start WiFi in AP mode (from Program 1)
  Serial.printf("Setting up WiFi Access Point '%s'\n", ssid);
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started");
  Serial.print("Connect to: ");
  Serial.println(WiFi.softAPIP()); // Print AP IP address

  // Setup WebSocket and HTTP handlers (from Program 1)
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);
  server.on("/", handleRoot); // Serve homepage
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found"); // 404 handler
  });

  server.begin(); // Start server
  Serial.println("HTTP server started");
}

// Loop function (empty as everything is event-driven)
void loop() {
  // The AsyncWebServer handles client requests in the background
  // The WebSocket handles incoming messages asynchronously
  // No blocking code here, so loop can be empty.
}