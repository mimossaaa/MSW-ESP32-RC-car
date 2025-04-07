#include <WiFi.h>
#include <WebServer.h>

// Motor control pins
#define IN1 26  // Right motor forward
#define IN2 27  // Right motor backward
#define IN3 14  // Left motor forward
#define IN4 12  // Left motor backward

// WiFi credentials
const char* ssid = "magne";
const char* password = "12345678";

WebServer server(80);

void setup() {
  Serial.begin(115200);

  // Initialize motor control pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  // Configure server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/forward", HTTP_POST, handleForward);
  server.on("/backward", HTTP_POST, handleBackward);
  server.on("/left", HTTP_POST, handleLeft);
  server.on("/right", HTTP_POST, handleRight);
  server.on("/stop", HTTP_POST, handleStop);

  server.begin();
}

void loop() {
  server.handleClient();
}

/**********************
 * Motor Control Functions
 **********************/
void moveForward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  // Right motor forward, left motor stopped
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  // Left motor forward, right motor stopped
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
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
      <title>WASD Robot Control</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body {
          display: flex;
          flex-direction: column;
          align-items: center;
          background-color: #f0f0f0;
          font-family: Arial, sans-serif;
        }
        #status {
          margin: 20px;
          padding: 10px 20px;
          background-color: white;
          border-radius: 5px;
          box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
      </style>
    </head>
    <body>
      <h1>WASD Robot Control</h1>
      <div>🤖</div>
      <div id="status">Status: Ready</div>

      <script>
        const keys = {
          w: false,
          a: false,
          s: false,
          d: false
        };

        function updateCommand() {
          let command = 'stop';
          if (keys.w) command = 'forward';
          if (keys.s) command = 'backward';
          if (keys.a) command = 'right';
          if (keys.d) command = 'left';
          
          // Handle diagonal combinations
          if (keys.w && keys.a) command = 'forward-left';
          if (keys.w && keys.d) command = 'forward-right';
          if (keys.s && keys.a) command = 'backward-left';
          if (keys.s && keys.d) command = 'backward-right';

          sendCommand(command);
        }

        document.addEventListener('keydown', (e) => {
          const key = e.key.toLowerCase();
          if (['w','a','s','d'].includes(key) && !keys[key]) {
            keys[key] = true;
            updateCommand();
            e.preventDefault();
          }
        });

        document.addEventListener('keyup', (e) => {
          const key = e.key.toLowerCase();
          if (['w','a','s','d'].includes(key)) {
            keys[key] = false;
            updateCommand();
            e.preventDefault();
          }
        });

        function sendCommand(command) {
          const statusElement = document.getElementById('status');
          statusElement.textContent = `Status: ${command.charAt(0).toUpperCase() + command.slice(1)}`;
          
          fetch(`/${command}`, { method: 'POST' })
            .catch(error => {
              statusElement.textContent = 'Error sending command';
              console.error('Error:', error);
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
  server.send(200, "text/plain", "OK");
}
void handleBackward() {
  moveBackward();
  server.send(200, "text/plain", "OK");
}
void handleLeft() {
  turnLeft();
  server.send(200, "text/plain", "OK");
}
void handleRight() {
  turnRight();
  server.send(200, "text/plain", "OK");
}
void handleStop() {
  stopMotors();
  server.send(200, "text/plain", "OK");
}