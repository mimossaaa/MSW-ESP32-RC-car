#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Hardware Pin definitions
#define IN1 26  // Right motor forward
#define IN2 27  // Right motor backward
#define IN3 14  // Left motor forward
#define IN4 12  // Left motor backward

// WiFi AP credentials
const char* ssid = "MSW_minibot";
const char* password = "12345678";

// Define command constants
#define UP 1
#define DOWN 2
#define COMMAND_LEFT_PIVOT 3  // Maps to '/left' path (e.g., 'd' key or Left button)
#define COMMAND_RIGHT_PIVOT 4 // Maps to '/right' path (e.g., 'a' key or Right button)

// Define diagonal movement commands
#define FORWARD_LEFT 5  // Maps to '/forward-left' path
#define FORWARD_RIGHT 6 // Maps to '/forward-right' path
#define BACKWARD_LEFT 7 // Maps to '/backward-left' path
#define BACKWARD_RIGHT 8 // Maps to '/backward-right' path

#define STOP 0

// Define motor directions
#define MOTOR_FORWARD 1
#define MOTOR_BACKWARD -1
#define MOTOR_STOP 0

// Define motor indexes
#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

// Async Web Server
AsyncWebServer server(80);

// HTML content (updated with new styles, joystick, and text)
const char* htmlHomePage PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>MSW Robotics Control Booth</title>
      <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
      <style>
        /* General Styling for a Kid-Friendly, Mars Rover Theme */
        body {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: flex-start; /* Align items from the top */
          min-height: 100vh; /* Full viewport height */
          margin: 0;
          background: linear-gradient(135deg, #FFD700 0%, #FF8C00 50%, #DC143C 100%); /* Fiery Mars-like gradient */
          font-family: 'Comic Sans MS', cursive, sans-serif; /* Playful font */
          color: #333;
          padding: 20px;
          box-sizing: border-box; /* Include padding in element's total width and height */
        }
        h1 {
          color: #2C3E50; /* Dark blue for contrast */
          text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
          margin-bottom: 15px;
          font-size: 2.5em; /* Larger title */
          text-align: center;
        }
        #status {
          margin: 20px auto; /* Center horizontally */
          padding: 15px 25px;
          background-color: #FFF8DC; /* Creamy white, like a control panel screen */
          border: 3px solid #D35400; /* Orange border */
          border-radius: 10px;
          box-shadow: 0 4px 8px rgba(0,0,0,0.2);
          font-weight: bold;
          font-size: 1.2em;
          color: #C0392B; /* Reddish text for warnings/status */
          text-align: center;
          max-width: 300px; /* Limit width for readability */
        }

        /* Styles for the Control Buttons */
        .controls {
          margin-top: 30px;
          display: flex;
          flex-direction: column;
          align-items: center;
          gap: 15px; /* Space between rows */
          width: 100%;
          max-width: 350px; /* Max width for button layout */
        }
        .button-row {
          display: flex;
          justify-content: center;
          gap: 15px; /* Space between buttons in a row */
          width: 100%;
        }
        .control-button {
          flex: 1; /* Allows buttons to grow and shrink */
          min-width: 90px; /* Minimum width for buttons */
          height: 90px;
          font-size: 1.5em; /* Larger text for kids */
          font-weight: bold;
          border: none;
          border-radius: 15px; /* More rounded corners */
          background-color: #3498DB; /* Blue */
          color: white;
          cursor: pointer;
          box-shadow: 5px 5px #2980B9; /* Darker blue shadow */
          display: flex;
          justify-content: center;
          align-items: center;
          user-select: none;
          -webkit-tap-highlight-color: rgba(0,0,0,0);
          transition: background-color 0.1s ease, transform 0.1s ease, box-shadow 0.1s ease;
        }
        .control-button:active {
          transform: translate(5px, 5px);
          box-shadow: none;
        }
        /* Specific button colors */
        #btn-forward, #btn-backward, #btn-left, #btn-right {
            background-color: #27AE60; /* Green for movement */
            box-shadow: 5px 5px #219653;
        }
        #btn-forward:active, #btn-backward:active, #btn-left:active, #btn-right:active {
            background-color: #219653;
        }
        #btn-stop {
            background-color: #E74C3C; /* Red for stop */
            box-shadow: 5px 5px #C0392B;
        }
        #btn-stop:active {
            background-color: #C0392B;
        }

        /* Joystick Styles */
        #joystick-container {
            margin-top: 40px;
            width: 200px; /* Size of the joystick area */
            height: 200px;
            position: relative;
            user-select: none;
            -webkit-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            touch-action: none; /* Prevents scrolling on touch devices */
        }
        #joystick-base {
            width: 100%;
            height: 100%;
            border-radius: 50%;
            background-color: rgba(255, 255, 255, 0.3); /* Translucent white base */
            border: 5px solid #2C3E50; /* Dark blue border */
            box-sizing: border-box;
            display: flex;
            justify-content: center;
            align-items: center;
            box-shadow: inset 0 0 15px rgba(0,0,0,0.3); /* Inner shadow for depth */
        }
        #joystick-knob {
            width: 80px; /* Size of the draggable knob */
            height: 80px;
            border-radius: 50%;
            background-color: #E67E22; /* Orange knob */
            position: absolute;
            left: 50%;
            top: 50%;
            transform: translate(-50%, -50%); /* Center the knob */
            cursor: grab;
            box-shadow: 0 5px 10px rgba(0,0,0,0.3);
            transition: background-color 0.1s ease, transform 0.05s ease;
            z-index: 10; /* Ensure knob is above base */
        }
        #joystick-knob.active {
            cursor: grabbing;
            background-color: #D35400; /* Darker orange when active */
            box-shadow: 0 2px 5px rgba(0,0,0,0.3);
        }
        .joystick-label {
            text-align: center;
            margin-top: 20px;
            font-size: 1.3em;
            font-weight: bold;
            color: #2C3E50;
            text-shadow: 1px 1px 2px rgba(0,0,0,0.1);
        }

        /* Media Queries for Responsiveness */
        @media (max-width: 600px) {
            h1 {
                font-size: 2em;
            }
            #status {
                padding: 10px 15px;
                font-size: 1em;
            }
            .control-button {
                height: 70px;
                font-size: 1.2em;
                border-radius: 10px;
            }
            .button-row {
                gap: 10px;
            }
            #joystick-container {
                width: 160px;
                height: 160px;
            }
            #joystick-knob {
                width: 60px;
                height: 60px;
            }
        }
      </style>
    </head>
    <body>
      <h1>🛸 MSW Robotics Control Booth 🤖</h1>
      <div id="status">Status: Ready</div>

      <div class="controls">
        <div class="button-row">
            <button class="control-button" id="btn-forward">FORWARD</button>
        </div>
        <div class="button-row">
            <button class="control-button" id="btn-left">LEFT</button>
            <button class="control-button" id="btn-stop">STOP</button>
            <button class="control-button" id="btn-right">RIGHT</button>
        </div>
        <div class="button-row">
            <button class="control-button" id="btn-backward">BACKWARD</button>
        </div>
      </div>

      <div class="joystick-label">Joystick Control</div>
      <div id="joystick-container">
          <div id="joystick-base">
              <div id="joystick-knob"></div>
          </div>
      </div>

      <script>
        const statusElement = document.getElementById('status');
        let currentCommand = 'stop'; // To track the currently sent command

        // State for WASD keys (used only for keyboard input)
        const keys = {
          w: false,
          a: false,
          s: false,
          d: false
        };

        // Determines the command based on the state of the WASD keys
        function updateKeyboardCommand() {
          let command = 'stop'; // Default command

          // Prioritize based on simultaneous key presses
          if (keys.w && keys.a) command = 'forward-right';
          else if (keys.w && keys.d) command = 'forward-left';
          else if (keys.s && keys.a) command = 'backward-right';
          else if (keys.s && keys.d) command = 'backward-left';
          else if (keys.w) command = 'forward';
          else if (keys.s) command = 'backward';
          else if (keys.a) command = 'right'; // A key -> maps to /right (pivot right)
          else if (keys.d) command = 'left';  // D key -> maps to /left (pivot left)

          sendCommand(command); // Send the determined command
        }

        // Keyboard event listeners
        document.addEventListener('keydown', (e) => {
          const key = e.key.toLowerCase();
          if (['w','a','s','d'].includes(key) && !keys[key]) {
            keys[key] = true; // Mark key as pressed
            updateKeyboardCommand(); // Re-evaluate and send command
            e.preventDefault(); // Prevent default browser actions (like scrolling)
          }
        });

        document.addEventListener('keyup', (e) => {
          const key = e.key.toLowerCase();
          if (['w','a','s','d'].includes(key)) {
            keys[key] = false; // Mark key as released
            updateKeyboardCommand(); // Re-evaluate and send command (will send 'stop' if no keys are left pressed)
            e.preventDefault();
          }
        });

        // Function to send command to the ESP32 server
        function sendCommand(command) {
          if (command === currentCommand) {
            return; // Avoid sending the same command repeatedly
          }
          currentCommand = command;

          const displayCommand = command.replace('-', ' ').split(' ').map(word => word.charAt(0).toUpperCase() + word.slice(1)).join(' ');
          statusElement.textContent = `Status: ${displayCommand}`;

          fetch(`/${command}`, { method: 'POST' })
            .then(response => {
              if (!response.ok) {
                console.error(`HTTP error! status: ${response.status} for command: ${command}`);
              }
            })
            .catch(error => {
              console.error('Network error sending command:', command, error);
              statusElement.textContent = `Network Error: ${command}`;
            });
        }

        // --- Button Event Listeners ---
        function addButtonListeners(buttonId, command) {
            const button = document.getElementById(buttonId);
            if (button) {
                // Touch events (for mobile)
                button.addEventListener('touchstart', (e) => {
                    e.preventDefault();
                    sendCommand(command);
                });
                button.addEventListener('touchend', (e) => {
                    e.preventDefault();
                    sendCommand('stop');
                });
                button.addEventListener('touchcancel', (e) => {
                    e.preventDefault();
                    sendCommand('stop');
                });

                // Mouse events (for desktop)
                button.addEventListener('mousedown', () => {
                    sendCommand(command);
                });
                button.addEventListener('mouseup', () => {
                    sendCommand('stop');
                });
                button.addEventListener('mouseleave', () => { // Stop if mouse leaves while pressed
                    sendCommand('stop');
                });
            }
        }

        // Add listeners for each directional button
        addButtonListeners('btn-forward', 'forward');
        addButtonListeners('btn-backward', 'backward');
        addButtonListeners('btn-left', 'left');  // 'left' path maps to COMMAND_LEFT_PIVOT (Program 2: Right Fwd, Left Bwd)
        addButtonListeners('btn-right', 'right'); // 'right' path maps to COMMAND_RIGHT_PIVOT (Program 2: Right Bwd, Left Fwd)
        addButtonListeners('btn-stop', 'stop');

        // --- Joystick Logic ---
        const joystickContainer = document.getElementById('joystick-container');
        const joystickBase = document.getElementById('joystick-base');
        const joystickKnob = document.getElementById('joystick-knob');

        let isDraggingJoystick = false;
        let joystickCenterX, joystickCenterY;
        let joystickRadius;

        function getJoystickPosition(e) {
            let clientX, clientY;
            if (e.touches && e.touches[0]) {
                clientX = e.touches[0].clientX;
                clientY = e.touches[0].clientY;
            } else {
                clientX = e.clientX;
                clientY = e.clientY;
            }
            return { clientX, clientY };
        }

        function startJoystickDrag(e) {
            e.preventDefault();
            isDraggingJoystick = true;
            joystickKnob.classList.add('active');

            const rect = joystickBase.getBoundingClientRect();
            joystickCenterX = rect.left + rect.width / 2;
            joystickCenterY = rect.top + rect.height / 2;
            joystickRadius = rect.width / 2;

            // Disable text selection while dragging
            document.body.style.userSelect = 'none';
            document.body.style.webkitUserSelect = 'none';
            document.body.style.mozUserSelect = 'none';
            document.body.style.msUserSelect = 'none';

            document.addEventListener('mousemove', doJoystickDrag);
            document.addEventListener('touchmove', doJoystickDrag, { passive: false });
            document.addEventListener('mouseup', stopJoystickDrag);
            document.addEventListener('touchend', stopJoystickDrag);
            document.addEventListener('touchcancel', stopJoystickDrag);
        }

        function doJoystickDrag(e) {
            if (!isDraggingJoystick) return;
            e.preventDefault();

            const { clientX, clientY } = getJoystickPosition(e);

            let deltaX = clientX - joystickCenterX;
            let deltaY = clientY - joystickCenterY;

            // Calculate distance from center
            let distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);

            // Constrain knob within the base circle
            if (distance > joystickRadius) {
                deltaX = (deltaX / distance) * joystickRadius;
                deltaY = (deltaY / distance) * joystickRadius;
                distance = joystickRadius; // Update distance to the max radius
            }

            // Update knob position (relative to its own center)
            joystickKnob.style.left = `${deltaX + joystickBase.offsetWidth / 2}px`;
            joystickKnob.style.top = `${deltaY + joystickBase.offsetHeight / 2}px`;

            // Map joystick position to robot commands
            processJoystickInput(deltaX, deltaY, joystickRadius);
        }

        function processJoystickInput(x, y, maxRadius) {
            const threshold = maxRadius * 0.3; // Sensitivity threshold for movement
            let cmd = 'stop';

            // Normalize X and Y to a -1 to 1 range based on maxRadius
            const normalizedX = x / maxRadius;
            const normalizedY = y / maxRadius;

            // Adjust sensitivity for diagonals/pivots
            const forwardBackwardStrength = -normalizedY; // Y is inverted for screen coords
            const leftRightStrength = normalizedX;

            // Prioritize forward/backward if strong enough
            if (forwardBackwardStrength > 0.5) { // Moving Forward
                if (leftRightStrength > 0.3) cmd = 'forward-right'; // W+A -> Right Fwd, Left Stop
                else if (leftRightStrength < -0.3) cmd = 'forward-left'; // W+D -> Right Stop, Left Fwd
                else cmd = 'forward';
            } else if (forwardBackwardStrength < -0.5) { // Moving Backward
                if (leftRightStrength > 0.3) cmd = 'backward-right'; // S+A -> Right Bwd, Left Stop
                else if (leftRightStrength < -0.3) cmd = 'backward-left'; // S+D -> Right Stop, Left Bwd
                else cmd = 'backward';
            } else if (Math.abs(leftRightStrength) > 0.5) { // Turning Left/Right (Pivoting)
                if (leftRightStrength > 0) cmd = 'right'; // A key -> Right Bwd, Left Fwd
                else cmd = 'left'; // D key -> Right Fwd, Left Bwd
            } else {
                cmd = 'stop'; // Within dead zone
            }

            sendCommand(cmd);
        }

        function stopJoystickDrag() {
            isDraggingJoystick = false;
            joystickKnob.classList.remove('active');

            // Reset knob position to center
            joystickKnob.style.left = '50%';
            joystickKnob.style.top = '50%';
            joystickKnob.style.transform = 'translate(-50%, -50%)';

            // Re-enable text selection
            document.body.style.userSelect = 'auto';
            document.body.style.webkitUserSelect = 'auto';
            document.body.style.mozUserSelect = 'auto';
            document.body.style.msUserSelect = 'auto';

            sendCommand('stop'); // Send stop command when joystick is released

            document.removeEventListener('mousemove', doJoystickDrag);
            document.removeEventListener('touchmove', doJoystickDrag);
            document.removeEventListener('mouseup', stopJoystickDrag);
            document.removeEventListener('touchend', stopJoystickDrag);
            document.removeEventListener('touchcancel', stopJoystickDrag);
        }

        // Add joystick event listeners
        joystickKnob.addEventListener('mousedown', startJoystickDrag);
        joystickKnob.addEventListener('touchstart', startJoystickDrag, { passive: false });

      </script>
    </body>
    </html>
)rawliteral";


// Function to set the direction of a single motor using the H-bridge pins
void setMotorDirection(int motorIndex, int direction) {
    if (motorIndex == RIGHT_MOTOR) {
        if (direction == MOTOR_FORWARD) {
            digitalWrite(IN1, HIGH); // IN1 HIGH, IN2 LOW for Right Forward
            digitalWrite(IN2, LOW);
        } else if (direction == MOTOR_BACKWARD) {
            digitalWrite(IN1, LOW);  // IN1 LOW, IN2 HIGH for Right Backward
            digitalWrite(IN2, HIGH);
        } else { // MOTOR_STOP
            digitalWrite(IN1, LOW);  // Both LOW for Stop
            digitalWrite(IN2, LOW);
        }
    } else if (motorIndex == LEFT_MOTOR) {
        if (direction == MOTOR_FORWARD) {
            digitalWrite(IN3, HIGH); // IN3 HIGH, IN4 LOW for Left Forward
            digitalWrite(IN4, LOW);
        } else if (direction == MOTOR_BACKWARD) {
            digitalWrite(IN3, LOW);  // IN3 LOW, IN4 HIGH for Left Backward
            digitalWrite(IN4, HIGH);
        } else { // MOTOR_STOP
            digitalWrite(IN3, LOW);  // Both LOW for Stop
            digitalWrite(IN4, LOW);
        }
    }
    // Ignore invalid motorIndex
}

// Function to control the car's movement based on command code
void moveCar(int cmd) {
    Serial.printf("Received MoveCar command code: %d\n", cmd); // Debug print
    switch (cmd) {
        case UP: // Both motors forward
            setMotorDirection(RIGHT_MOTOR, MOTOR_FORWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_FORWARD);
            break;
        case DOWN: // Both motors backward
            setMotorDirection(RIGHT_MOTOR, MOTOR_BACKWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_BACKWARD);
            break;
        case COMMAND_LEFT_PIVOT: // Pivot Left (Right forward, Left backward)
            setMotorDirection(RIGHT_MOTOR, MOTOR_FORWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_BACKWARD);
            break;
        case COMMAND_RIGHT_PIVOT: // Pivot Right (Right backward, Left forward)
            setMotorDirection(RIGHT_MOTOR, MOTOR_BACKWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_FORWARD);
            break;
        // Diagonal movements
        case FORWARD_LEFT: // Right Stop, Left Fwd
            setMotorDirection(RIGHT_MOTOR, MOTOR_STOP);
            setMotorDirection(LEFT_MOTOR, MOTOR_FORWARD);
            break;
        case FORWARD_RIGHT: // Right Fwd, Left Stop
            setMotorDirection(RIGHT_MOTOR, MOTOR_FORWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_STOP);
            break;
        case BACKWARD_LEFT: // Right Stop, Left Bwd
            setMotorDirection(RIGHT_MOTOR, MOTOR_STOP);
            setMotorDirection(LEFT_MOTOR, MOTOR_BACKWARD);
            break;
        case BACKWARD_RIGHT: // Right Bwd, Left Stop
            setMotorDirection(RIGHT_MOTOR, MOTOR_BACKWARD);
            setMotorDirection(LEFT_MOTOR, MOTOR_STOP);
            break;
        case STOP: // Both motors stop
        default:
            setMotorDirection(RIGHT_MOTOR, MOTOR_STOP);
            setMotorDirection(LEFT_MOTOR, MOTOR_STOP);
            break;
    }
}


// HTTP Handlers
void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", htmlHomePage);
}

void handleForward(AsyncWebServerRequest *request) {
    moveCar(UP);
    request->send(200, "text/plain", "OK");
}

void handleBackward(AsyncWebServerRequest *request) {
    moveCar(DOWN);
    request->send(200, "text/plain", "OK");
}

void handleLeft(AsyncWebServerRequest *request) {
    moveCar(COMMAND_LEFT_PIVOT);
    request->send(200, "text/plain", "OK");
}

void handleRight(AsyncWebServerRequest *request) {
    moveCar(COMMAND_RIGHT_PIVOT);
    request->send(200, "text/plain", "OK");
}

void handleForwardLeft(AsyncWebServerRequest *request) {
    moveCar(FORWARD_LEFT);
    request->send(200, "text/plain", "OK");
}

void handleForwardRight(AsyncWebServerRequest *request) {
    moveCar(FORWARD_RIGHT);
    request->send(200, "text/plain", "OK");
}

void handleBackwardLeft(AsyncWebServerRequest *request) {
    moveCar(BACKWARD_LEFT);
    request->send(200, "text/plain", "OK");
}

void handleBackwardRight(AsyncWebServerRequest *request) {
    moveCar(BACKWARD_RIGHT);
    request->send(200, "text/plain", "OK");
}

void handleStop(AsyncWebServerRequest *request) {
    moveCar(STOP);
    request->send(200, "text/plain", "OK");
}


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
  Serial.println("WiFi AP started");
  Serial.print("Connect to: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);

  server.on("/forward", HTTP_POST, handleForward);
  server.on("/backward", HTTP_POST, handleBackward);
  server.on("/left", HTTP_POST, handleLeft);
  server.on("/right", HTTP_POST, handleRight);
  server.on("/stop", HTTP_POST, handleStop);

  server.on("/forward-left", HTTP_POST, handleForwardLeft);
  server.on("/forward-right", HTTP_POST, handleForwardRight);
  server.on("/backward-left", HTTP_POST, handleBackwardLeft);
  server.on("/backward-right", HTTP_POST, handleBackwardRight);

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("HTTP server started");
}

// Loop function
void loop() {
  // AsyncWebServer handles requests in the background.
}
