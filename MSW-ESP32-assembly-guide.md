## 🧰 Components You Need

- 1x ESP32 development board
- 1x L298N Motor Driver
- 2x TT DC gear motors
- Custom motor mounts
- Jumper wires (male-to-female and male-to-male)
- Power source (e.g., 2x 18650 batteries or 6V–12V battery pack)
- Breadboard or soldered connections (optional)
- Chassis or frame for mounting (can be 3D-printed or DIY)

---

## ⚙️ Wiring Instructions

### 🔌 Power

|L298N Pin|Connect To|
|---|---|
|`12V`|Positive terminal of battery|
|`GND`|Battery ground **AND** ESP32 GND|
|`5V`|**Do NOT connect to ESP32 5V** if using external battery|
|`ENB` (Right)|Connect to +5V (or jumper to enable)|
|`ENA` (Left)|Connect to +5V (or jumper to enable)|

### 🎮 Motor Control Pins (match your code)

|L298N Pin|ESP32 GPIO Pin|Function|
|---|---|---|
|IN1|GPIO 26|Right motor forward|
|IN2|GPIO 27|Right motor backward|
|IN3|GPIO 14|Left motor forward|
|IN4|GPIO 12|Left motor backward|

### 🔩 Motors

- Connect **right motor** to **OUT1 and OUT2**
- Connect **left motor** to **OUT3 and OUT4**

Make sure your motor polarity matches the desired direction—swap wires if it's reversed.

---

## 🛠 Assembly Steps

1. **Mount Motors** using your custom 3D-printed mounts or brackets onto the chassis.
2. **Attach Wheels** to the motors.
3. **Secure the L298N** motor driver and **ESP32** to the chassis.
4. **Wire everything** as per the wiring diagram above.
5. **Connect power**:
    - If using a battery pack, connect + to `12V` and - to `GND`.


---

## 🌐 Uploading the Code

1. Open Arduino IDE.
2. Install **ESP32 board support** (via Board Manager).
3. Select your ESP32 board and COM port.
4. Paste your code into the IDE.
5. Click **Upload**.

---

## 📶 Wi-Fi Control (WASD keys)

1. Connect your computer to the same Wi-Fi network (`magne`, `12345678`).
2. After uploading, open the Serial Monitor to get the **ESP32's IP address**.
3. Open a browser and navigate to `http://<your-ESP32-IP>` (e.g., `http://192.168.1.42`)
4. Use your keyboard **W/A/S/D** to control the robot.

---

## 🧪 Troubleshooting Tips

- If motors don’t spin, check if `ENA` and `ENB` are enabled (jumpered or connected to 5V).
- Use a **separate power supply** for motors if the ESP32 resets when they start.
- Double-check that **grounds are all connected**.
- If the ESP32 won’t connect to Wi-Fi, ensure SSID/password are correct.

---

Let me know if you want a **diagram**, **3D printable chassis file**, or **diagonal movement support** added to the code.
