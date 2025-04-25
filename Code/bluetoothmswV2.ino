
// Include necessary libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>

// Define a struct to hold motor control pin assignments
struct MOTOR_PINS {
  int pinEn;   // Enable pin (PWM)
  int pinIN1;  // Direction control pin 1
  int pinIN2;  // Direction control pin 2
};

// Create a vector holding pin configurations for both motors
std::vector<MOTOR_PINS> motorPins = {
  {14, 27, 26},  // RIGHT_MOTOR Pins (EnA, IN1, IN2)
  {32, 25, 33},  // LEFT_MOTOR  Pins (EnB, IN3, IN4)
};

// Define WiFi AP credentials
const char* ssid     = "MSW_minibot";  // WiFi SSID
const char* password = "12345678";         // WiFi Password

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

// Define motor directions
#define FORWARD 1
#define BACKWARD -1

// PWM configuration
const int PWMFreq = 1000;            // PWM frequency in Hz
const int PWMResolution = 8;         // 8-bit resolution (0-255)
const int PWMSpeedChannel = 2;       // PWM channel used for motor speed

// Create AsyncWebServer on port 80 and WebSocket endpoint
AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

// HTML page served to clients
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
  <!-- Main layout with buttons and slider -->
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
        send("Speed", document.getElementById("Speed").value); // Set initial speed
      };
      socket.onclose = () => setTimeout(connect, 2000); // Reconnect if disconnected
    }

    // Send key-value pair through WebSocket
    function send(key, value) {
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(key + "," + value);
      }
    }

    window.onload = connect; // Connect on page load
  </script>
</body>
</html>
)HTML";

// Function to rotate a motor in specified direction
void rotateMotor(int motorNumber, int direction) {
  if (direction == FORWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  } else if (direction == BACKWARD) {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  } else {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW); // Stop
  }
}

// Function to control the car's movement based on input command
void moveCar(int cmd) {
  switch (cmd) {
    case UP: rotateMotor(RIGHT_MOTOR, FORWARD); rotateMotor(LEFT_MOTOR, FORWARD); break;
    case DOWN: rotateMotor(RIGHT_MOTOR, BACKWARD); rotateMotor(LEFT_MOTOR, BACKWARD); break;
    case LEFT: rotateMotor(RIGHT_MOTOR, FORWARD); rotateMotor(LEFT_MOTOR, BACKWARD); break;
    case RIGHT: rotateMotor(RIGHT_MOTOR, BACKWARD); rotateMotor(LEFT_MOTOR, FORWARD); break;
    case FORWARD_LEFT: rotateMotor(RIGHT_MOTOR, FORWARD); rotateMotor(LEFT_MOTOR, STOP); break;
    case FORWARD_RIGHT: rotateMotor(RIGHT_MOTOR, STOP); rotateMotor(LEFT_MOTOR, FORWARD); break;
    case BACKWARD_LEFT: rotateMotor(RIGHT_MOTOR, BACKWARD); rotateMotor(LEFT_MOTOR, STOP); break;
    case BACKWARD_RIGHT: rotateMotor(RIGHT_MOTOR, STOP); rotateMotor(LEFT_MOTOR, BACKWARD); break;
    case STOP: default: rotateMotor(RIGHT_MOTOR, STOP); rotateMotor(LEFT_MOTOR, STOP); break;
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
      std::string msg((char *)data, len); // Parse message
      std::istringstream ss(msg);
      std::string key, value;
      getline(ss, key, ',');
      getline(ss, value, ',');
      int val = atoi(value.c_str()); // Convert value to int
      if (key == "MoveCar") {
        moveCar(val);
      } else if (key == "Speed") {
        ledcWrite(PWMSpeedChannel, val); // Set motor speed
      }
    }
  }
}

// Setup function
void setup() {
  Serial.begin(115200); // Start serial communication

  // Set motor pins as outputs
  for (auto &motor : motorPins) {
    pinMode(motor.pinEn, OUTPUT);
    pinMode(motor.pinIN1, OUTPUT);
    pinMode(motor.pinIN2, OUTPUT);
    digitalWrite(motor.pinIN1, LOW);
    digitalWrite(motor.pinIN2, LOW);
  }

  // Setup PWM for motor speed control
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);
  ledcAttachPin(motorPins[RIGHT_MOTOR].pinEn, PWMSpeedChannel);
  ledcAttachPin(motorPins[LEFT_MOTOR].pinEn, PWMSpeedChannel);

  // Start WiFi in AP mode
  WiFi.softAP(ssid, password);
  Serial.println("WiFi started");
  Serial.println(WiFi.softAPIP()); // Print AP IP address

  // Setup WebSocket and HTTP handlers
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);
  server.on("/", handleRoot); // Serve homepage
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found"); // 404 handler
  });

  server.begin(); // Start server
}

// Loop function (empty as everything is event-driven)
void loop() {
  //
}