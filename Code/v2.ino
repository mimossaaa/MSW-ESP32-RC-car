#include <WiFi.h>
#include <WebServer.h>

// Motor control pins
#define IN1 26  // Right motor control 1
#define IN2 27  // Right motor control 2
#define IN3 14  // Left motor control 1
#define IN4 12  // Left motor control 2

// WiFi AP credentials
const char* ssid = "MSW Minibot";
const char* password = "12345678";

WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Initialize motor control pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();
  Serial.println("Motor pins initialized.");

  // Setup ESP32 as an Access Point
  Serial.print("Setting up AP: ");
  Serial.println(ssid);
  if (WiFi.softAP(ssid, password)) {
    Serial.println("AP Created!");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP()); // IP address will usually be 192.168.4.1
  } else {
    Serial.println("Failed to create AP!");
    // Loop forever if AP creation fails
    while(1);
  }

  // Configure server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/forward", HTTP_POST, handleForward);
  server.on("/backward", HTTP_POST, handleBackward);
  server.on("/left", HTTP_POST, handleLeft);     // Corresponds to 'd' key in JS
  server.on("/right", HTTP_POST, handleRight);    // Corresponds to 'a' key in JS
  server.on("/stop", HTTP_POST, handleStop);
  // Note: The JS also sends diagonal commands like /forward-left.
  // To handle these, you'd need to add more routes and motor functions.
  // For now, they will result in a 404, and the robot will likely continue
  // with the last non-diagonal command or stop if no keys are pressed.

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}

/**********************
 * Motor Control Functions
 * Assumes IN1/IN2 for Right Motor, IN3/IN4 for Left Motor
 * For L298N or similar:
 *  - (LOW, HIGH) = Forward
 *  - (HIGH, LOW) = Backward
 *  - (LOW, LOW) or (HIGH, HIGH) = Stop/Brake (depending on driver)
 **********************/
void moveForward() {
  Serial.println("Moving Forward");
  // Right motor forward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  // Left motor forward
  digitalWrite(IN3, HIGH); // Assuming HIGH on IN3 is forward for left motor based on original code
  digitalWrite(IN4, LOW);  // Assuming LOW on IN4 is forward for left motor
}

void moveBackward() {
  Serial.println("Moving Backward");
  // Right motor backward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  // Left motor backward
  digitalWrite(IN3, LOW);  // Assuming LOW on IN3 is backward for left motor
  digitalWrite(IN4, HIGH); // Assuming HIGH on IN4 is backward for left motor
}

// This function is called when JS sends "/left" (mapped from 'd' key)
// Original code for turnLeft makes robot pivot RIGHT
void turnLeft_RobotActionRight() {
  Serial.println("Action: Robot Pivots Right (called by JS /left command)");
  // Right motor backward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  // Left motor forward
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// This function is called when JS sends "/right" (mapped from 'a' key)
// Original code for turnRight makes robot pivot LEFT
void turnRight_RobotActionLeft() {
  Serial.println("Action: Robot Pivots Left (called by JS /right command)");
  // Right motor forward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  // Left motor backward
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  Serial.println("Stopping Motors");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

/**********************
 * HTTP Handlers
 **********************/
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>MSW Minibot Control</title>
      <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
      <style>
        body {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          height: 100vh;
          margin: 0;
          background-color: #2c3e50;
          color: #ecf0f1;
          font-family: Arial, sans-serif;
          text-align: center;
        }
        h1 {
          margin-bottom: 10px;
        }
        #robot-icon {
          font-size: 50px;
          margin-bottom: 20px;
        }
        #status {
          margin-bottom: 20px;
          padding: 10px 20px;
          background-color: #34495e;
          border-radius: 5px;
          box-shadow: 0 2px 5px rgba(0,0,0,0.1);
          min-width: 200px;
        }
        .controls-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            grid-template-rows: repeat(3, 1fr);
            gap: 10px;
            width: 240px; /* Adjust as needed */
            height: 240px; /* Adjust as needed */
        }
        .control-button {
            display: flex;
            justify-content: center;
            align-items: center;
            background-color: #3498db;
            color: white;
            font-size: 24px;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            user-select: none; /* Prevent text selection on touch */
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            transition: background-color 0.2s, transform 0.1s;
        }
        .control-button:active {
            background-color: #2980b9;
            transform: scale(0.95);
        }
        .grid-placeholder { /* Empty cell */ }
        #btn-forward { grid-area: 1 / 2 / 2 / 3; }
        #btn-left    { grid-area: 2 / 1 / 3 / 2; }
        #btn-stop    { grid-area: 2 / 2 / 3 / 3; background-color: #e74c3c; }
        #btn-stop:active { background-color: #c0392b; }
        #btn-right   { grid-area: 2 / 3 / 3 / 4; }
        #btn-backward{ grid-area: 3 / 2 / 4 / 3; }
        .instructions { margin-top: 20px; font-size: 0.9em; color: #bdc3c7; }
      </style>
    </head>
    <body>
      <h1>MSW Minibot Control</h1>
      <div id="robot-icon">🤖</div>
      <div id="status">Status: Ready</div>

      <div class="controls-grid">
        <div class="grid-placeholder"></div>
        <button class="control-button" id="btn-forward" ontouchstart="sendCommand('forward')" onmousedown="isTouchDevice() || sendCommand('forward')" ontouchend="sendCommand('stop')" onmouseup="isTouchDevice() || sendCommand('stop')">▲</button>
        <div class="grid-placeholder"></div>
        <button class="control-button" id="btn-left" ontouchstart="sendCommand('right')" onmousedown="isTouchDevice() || sendCommand('right')" ontouchend="sendCommand('stop')" onmouseup="isTouchDevice() || sendCommand('stop')">◀</button>
        <button class="control-button" id="btn-stop" ontouchstart="sendCommand('stop')" onmousedown="isTouchDevice() || sendCommand('stop')">■</button>
        <button class="control-button" id="btn-right" ontouchstart="sendCommand('left')" onmousedown="isTouchDevice() || sendCommand('left')" ontouchend="sendCommand('stop')" onmouseup="isTouchDevice() || sendCommand('stop')">▶</button>
        <div class="grid-placeholder"></div>
        <button class="control-button" id="btn-backward" ontouchstart="sendCommand('backward')" onmousedown="isTouchDevice() || sendCommand('backward')" ontouchend="sendCommand('stop')" onmouseup="isTouchDevice() || sendCommand('stop')">▼</button>
        <div class="grid-placeholder"></div>
      </div>
      <div class="instructions">
        Use WASD keys on Desktop or Tap buttons on Mobile.
      </div>


      <script>
        const statusElement = document.getElementById('status');
        let _isTouchDevice = false; // To prevent double events on touch devices for mouse events

        function isTouchDevice() {
          try {
            document.createEvent("TouchEvent");
            _isTouchDevice = true; // Store this so we don't keep checking
            return true;
          } catch (e) {
            return false;
          }
        }
        isTouchDevice(); // Check once on load


        const keys = {
          w: false, a: false, s: false, d: false
        };
        let lastCommand = 'stop'; // To avoid sending redundant stop commands

        function updateCommandFromKeys() {
          let command = 'stop'; // Default to stop
          // Prioritize forward/backward
          if (keys.w) command = 'forward';
          else if (keys.s) command = 'backward';
          
          // Turning overrides forward/backward if both are pressed,
          // or applies if only turning key is pressed.
          if (keys.a) { // 'a' key for turning left (sends /right to server due to current mapping)
            if (command === 'forward') command = 'forward-right'; // JS perspective
            else if (command === 'backward') command = 'backward-right'; // JS perspective
            else command = 'right'; // JS perspective ('a' -> 'right' command)
          } else if (keys.d) { // 'd' key for turning right (sends /left to server)
            if (command === 'forward') command = 'forward-left'; // JS perspective
            else if (command === 'backward') command = 'backward-left'; // JS perspective
            else command = 'left'; // JS perspective ('d' -> 'left' command)
          }
          
          if (command !== lastCommand) {
            sendCommand(command);
            lastCommand = command;
          }
        }

        document.addEventListener('keydown', (e) => {
          const key = e.key.toLowerCase();
          if (['w','a','s','d'].includes(key)) {
            if (!keys[key]) { // Only update if state changes
              keys[key] = true;
              updateCommandFromKeys();
            }
            e.preventDefault(); // Prevent page scrolling
          }
        });

        document.addEventListener('keyup', (e) => {
          const key = e.key.toLowerCase();
          if (['w','a','s','d'].includes(key)) {
            if (keys[key]) { // Only update if state changes
              keys[key] = false;
              updateCommandFromKeys();
            }
            e.preventDefault();
          }
        });
        
        // Touch/Mouse button sendCommand
        function sendCommand(command) {
          // For touch, the keyup logic won't fire if a button is held,
          // so we ensure 'stop' is sent when a button is released.
          // For key presses, updateCommandFromKeys handles continuous state.
          lastCommand = command; // Update lastCommand for button presses too

          let statusText = command.charAt(0).toUpperCase() + command.slice(1);
          if (command === 'right') statusText = "Turning Left (A)"; // User sees 'Turning Left'
          else if (command === 'left') statusText = "Turning Right (D)"; // User sees 'Turning Right'

          statusElement.textContent = `Status: ${statusText}`;
          
          fetch(`/${command}`, { method: 'POST' })
            .then(response => {
              if (!response.ok) {
                statusElement.textContent = 'Error: Command failed';
                console.error('Command failed:', response);
              }
            })
            .catch(error => {
              statusElement.textContent = 'Error: No connection';
              console.error('Error sending command:', error);
            });
        }
      </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleForward() {
  moveForward();
  server.send(200, "text/plain", "OK: Forward");
}
void handleBackward() {
  moveBackward();
  server.send(200, "text/plain", "OK: Backward");
}
// Called by JS when 'd' is pressed (JS sends /left)
// Robot should turn RIGHT
void handleLeft() {
  turnLeft_RobotActionRight(); // This function makes the robot turn RIGHT
  server.send(200, "text/plain", "OK: Turning Right (D)");
}
// Called by JS when 'a' is pressed (JS sends /right)
// Robot should turn LEFT
void handleRight() {
  turnRight_RobotActionLeft(); // This function makes the robot turn LEFT
  server.send(200, "text/plain", "OK: Turning Left (A)");
}
void handleStop() {
  stopMotors();
  server.send(200, "text/plain", "OK: Stop");
}
