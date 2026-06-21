/*
  ESP32 MAIN CONTROLLER — SaniBot
  
  UART PIN CONNECTIONS (ESP32 Main side):
    GPIO 16  →  RX2  →  ESP32-CAM GPIO 14 (TX)
    GPIO 17  →  TX2  →  ESP32-CAM GPIO 15 (RX)
    GND      →  GND  →  ESP32-CAM GND

  MOTOR PINS:
    GPIO 27 → L298N IN1    GPIO 26 → L298N IN2
    GPIO 25 → L298N IN3    GPIO 33 → L298N IN4
    GPIO 32 → L298N ENA    GPIO 14 → L298N ENB

  SENSOR PINS:
    GPIO 5  → Front TRIG   GPIO 18 → Front ECHO
    GPIO 19 → Right TRIG   GPIO 21 → Right ECHO

  RELAY PINS:
    GPIO 22 → Relay IN1 (Mist)
    GPIO 23 → Relay IN2 (UV)
*/

#include <WiFi.h>      // Auto mode (optional, now cam will control)

//  UART2 Pins 
#define UART_RX_PIN   16    // ESP32 RX ← ESP32-CAM TX (GPIO 14)
#define UART_TX_PIN   17    // ESP32 TX → ESP32-CAM RX (GPIO 15)
#define UART_BAUD     9600
 
//  Motor Pins 
#define IN1  27
#define IN2  26
#define IN3  25
#define IN4  33
#define ENA  32
#define ENB  14

//  Sensor Pins 
#define TRIG_FRONT  5
#define ECHO_FRONT  18
#define TRIG_RIGHT  19
#define ECHO_RIGHT  21

//  Relay Pins 
#define RELAY_MIST  22
#define RELAY_UV    23


//  Settings 
#define MOTOR_SPEED  200
#define TURN_SPEED   170
#define SAFE_DIST     30
#define PWM_FREQ    1000
#define PWM_BITS       8

//  Auto Mode States 
#define STATE_FORWARD  0
#define STATE_STOP     1
#define STATE_REVERSE  2
#define STATE_TURN     3

//  Global State 
bool   autoMode   = true;
String curCmd     = "STOP";
int    frontDist  = 999;
int    rightDist  = 999;
int    motorSpd   = MOTOR_SPEED;
bool   mistOn     = true;
bool   uvOn       = true;
//bool freshenerOn = false;

int autoState             = STATE_FORWARD;
unsigned long stateStart  = 0;

//  UART receive buffer 
String uartBuffer = "";

//  DISTANCE SENSOR
int getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 15000);
  if (dur == 0) return 999;
  return (int)(dur / 58.2);
}

//  MOTOR FUNCTIONS (unchanged from original)
void setMotorSpeed(int l, int r) {
  ledcWrite(ENA, constrain(l, 0, 255));
  ledcWrite(ENB, constrain(r, 0, 255));
}

void stopMotors() {
  curCmd = "STOP";
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  setMotorSpeed(0, 0);
}

void turnRight() {
  curCmd = "RIGHT";
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setMotorSpeed(TURN_SPEED, TURN_SPEED);
}

void turnLeft() {
  curCmd = "LEFT";
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  setMotorSpeed(TURN_SPEED, TURN_SPEED);
}

void moveBackward() {
  curCmd = "BACKWARD";
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setMotorSpeed(motorSpd, motorSpd);
}

void moveForward() {
  curCmd = "FORWARD";
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  setMotorSpeed(motorSpd, motorSpd);
}

//  RELAY CONTROL
void setMist(bool on) {
  mistOn = on;
  digitalWrite(RELAY_MIST, on ? LOW : HIGH);  // LOW = relay ON (active low)
  Serial.println(on ? "Mist: ON" : "Mist: OFF");
}

void setUV(bool on) {
  uvOn = on;
  digitalWrite(RELAY_UV, on ? LOW : HIGH);
  Serial.println(on ? "UV: ON" : "UV: OFF");
}

//  AUTO AVOIDANCE (non-blocking, unchanged)
void runAuto() {
  unsigned long now = millis();

  switch (autoState) {
    case STATE_FORWARD:
      frontDist = getDistance(TRIG_FRONT, ECHO_FRONT);
      rightDist = getDistance(TRIG_RIGHT, ECHO_RIGHT);
      if (frontDist > SAFE_DIST) {
        moveForward();
      } else {
        stopMotors();
        autoState  = STATE_STOP;
        stateStart = now;
      }
      break;

    case STATE_STOP:
      if (now - stateStart >= 200) {
        moveBackward();
        autoState  = STATE_REVERSE;
        stateStart = now;
      }
      break;

    case STATE_REVERSE:
      if (now - stateStart >= 500) {
        stopMotors();
        rightDist = getDistance(TRIG_RIGHT, ECHO_RIGHT);
        if (rightDist > SAFE_DIST) turnRight();
        else                       turnLeft();
        autoState  = STATE_TURN;
        stateStart = now;
      }
      break;

    case STATE_TURN:
      if (now - stateStart >= 400) {
        stopMotors();
        autoState  = STATE_FORWARD;
        stateStart = now;
      }
      break;
  }
}

//  UART: Command parse 
//  Formats:
//    CMD:FORWARD
//    CMD:BACKWARD
//    CMD:LEFT
//    CMD:RIGHT
//    CMD:STOP
//    CMD:AUTO
//    CMD:MANUAL
//    CMD:MIST:1  or  CMD:MIST:0
//    CMD:UV:1    or  CMD:UV:0
//    CMD:SPD:200
void processUARTCommand(String line) {
  line.trim();
  Serial.println("UART CMD: " + line);

  if (!line.startsWith("CMD:")) return;
  String payload = line.substring(4);  // "CMD:" ke baad wala part

  if (payload == "FORWARD")  {
    if (!autoMode) moveForward();
  }
  else if (payload == "BACKWARD") {
    if (!autoMode) moveBackward();
  }
  else if (payload == "LEFT") {
    if (!autoMode) turnLeft();
  }
  else if (payload == "RIGHT") {
    if (!autoMode) turnRight();
  }
  else if (payload == "STOP") {
    if (!autoMode) stopMotors();
  }
  else if (payload == "AUTO") {
    autoMode   = true;
    autoState  = STATE_FORWARD;
    stateStart = millis();
    Serial.println("Mode: AUTO");
  }
  else if (payload == "MANUAL") {
    autoMode = false;
    stopMotors();
    Serial.println("Mode: MANUAL");
  }
  else if (payload.startsWith("MIST:")) {
    setMist(payload.substring(5) == "1");
  }
  else if (payload.startsWith("UV:")) {
    setUV(payload.substring(3) == "1");
  }
  else if (payload.startsWith("SPD:")) {
    motorSpd = constrain(payload.substring(4).toInt(), 80, 255);
    Serial.println("Speed: " + String(motorSpd));
  }
}

//  UART: Data read (non-blocking)
void readUART() {
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '\n') {
      if (uartBuffer.length() > 0) {
        processUARTCommand(uartBuffer);
        uartBuffer = "";
      }
    } else if (c != '\r') {
      uartBuffer += c;
    }
  }
}

//  UART: Status data ESP32-CAM ko bhejo
//  Format: DATA:FRONT:45:RIGHT:30:CMD:FORWARD:MODE:AUTO:MIST:0:UV:0
void sendStatusToCAM() {
  String data = "DATA:FRONT:" + String(frontDist) +
                ":RIGHT:"     + String(rightDist) +
                ":CMD:"       + curCmd +
                ":MODE:"      + (autoMode ? "AUTO" : "MANUAL") +
                ":MIST:"      + (mistOn ? "1" : "0") +
                ":UV:"        + (uvOn   ? "1" : "0");
  Serial2.println(data);
}

//  SETUP
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== SaniBot ESP32 Main Starting ===");

  //  Motor pins 
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  ledcAttach(ENA, PWM_FREQ, PWM_BITS);
  ledcAttach(ENB, PWM_FREQ, PWM_BITS);

  //  Sensor pins 
  pinMode(TRIG_FRONT, OUTPUT); pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_RIGHT, OUTPUT); pinMode(ECHO_RIGHT, INPUT);

  //  Relay pins (HIGH = relay OFF, active-low relay) 
  pinMode(RELAY_MIST, OUTPUT); digitalWrite(RELAY_MIST, HIGH);
  pinMode(RELAY_UV,   OUTPUT); digitalWrite(RELAY_UV,   HIGH);
  
  setMist(true);   // Auto mode start = mist ON
  setUV(true);     // Auto mode start = UV ON

  stopMotors();

  //  UART2 init 
  // RX=16, TX=17
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.println("UART2 ready: RX=" + String(UART_RX_PIN) + " TX=" + String(UART_TX_PIN));
  Serial.println("Setup complete! Auto mode active.");
}

//  MAIN LOOP
void loop() {

  // 1. UART commands receive 
  readUART();

  // 2. Auto mode (if auto)
  if (autoMode) {
    runAuto();
  }

  // 3. in every 500ms send status to CAM
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 500) {
    // Sensor data update (manual mode also)
    if (!autoMode) {
      frontDist = getDistance(TRIG_FRONT, ECHO_FRONT);
      rightDist = getDistance(TRIG_RIGHT, ECHO_RIGHT);
    }
    sendStatusToCAM();
    lastStatus = millis();
  }

  // 4. Serial monitor log (2 sec)
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 2000) {
    Serial.printf("[%s] F:%dcm R:%dcm CMD:%s Mist:%s UV:%s\n",
      autoMode ? "AUTO" : "MANUAL",
      frontDist, rightDist, curCmd.c_str(),
      mistOn ? "ON" : "OFF", uvOn ? "ON" : "OFF");
    lastLog = millis();
  }
}

