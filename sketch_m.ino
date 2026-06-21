//  ESP32-CAM  —  Merged Code
//  Object Detection (original) + SaniBot Control UI
//  UART2 se ESP32 Main ke saath communicate karta hai
//
//  UART PIN CONNECTIONS (ESP32-CAM side):
//    GPIO 14  →  TX2  →  ESP32 GPIO 16 (RX)
//    GPIO 15  →  RX2  →  ESP32 GPIO 17 (TX)
//    GND      →  GND  →  ESP32 GND
//
//  NOTE: GPIO 14 aur 15 ESP32-CAM pe free pins hain
//        (SD card use nahi kar rahe toh) 

const char* ssid     = "abcd";
const char* password = "1234";

const char* apssid = "abcde";
const char* appassword = "12345";

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// UART2 for ESP32 communication 
#define UART_TX_PIN   14    // ESP32-CAM TX → ESP32 RX (GPIO 16)
#define UART_RX_PIN   15    // ESP32-CAM RX → ESP32 TX (GPIO 17)
#define UART_BAUD     9600

//  Camera Pins (unchanged) 
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//  Original variables (unchanged) 
String Feedback = "";
String Command = "", cmd = "", P1 = "", P2 = "", P3 = "", P4 = "", P5 = "", P6 = "", P7 = "", P8 = "", P9 = "";
byte ReceiveState = 0, cmdState = 1, strState = 1, questionstate = 0, equalstate = 0, semicolonstate = 0;

//  SaniBot state (from UART) 
String saniCmd    = "STOP";
String saniMode   = "AUTO";
int    frontDist  = 0;
int    rightDist  = 0;
bool   mistOn     = true;
bool   uvOn       = true;

WiFiServer server(80);

//  UART: send command to ESP32
void sendToESP32(String command) {
  Serial2.println(command);
  Serial.println("UART Sent: " + command);
}

//  UART: data receive from ESP32
//  Format: DATA:FRONT:45:RIGHT:30:CMD:FORWARD:MODE:AUTO:MIST:0:UV:0
void readFromESP32() {
  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    Serial.println("UART Recv: " + line);

    if (line.startsWith("DATA:")) {
      int f = line.indexOf("FRONT:") + 6;
      int fEnd = line.indexOf(":", f);
      if (f > 5 && fEnd > f) frontDist = line.substring(f, fEnd).toInt();

      int r = line.indexOf("RIGHT:") + 6;
      int rEnd = line.indexOf(":", r);
      if (r > 5 && rEnd > r) rightDist = line.substring(r, rEnd).toInt();

      int c = line.indexOf("CMD:") + 4;
      int cEnd = line.indexOf(":", c);
      if (c > 3 && cEnd > c) saniCmd = line.substring(c, cEnd);

      int m = line.indexOf("MODE:") + 5;
      int mEnd = line.indexOf(":", m);
      if (m > 4 && mEnd > m) saniMode = line.substring(m, mEnd);

      int mi = line.indexOf("MIST:") + 5;
      int miEnd = line.indexOf(":", mi);
      if (mi > 4 && miEnd > mi) mistOn = (line.substring(mi, miEnd) == "1");

      //int uv = line.indexOf("UV:") + 3;
      //if (uv > 2) uvOn = (line.substring(uv) == "1");
      int uv = line.indexOf("UV:") + 3;
      int uvEnd = line.indexOf(":", uv);
      if (uv > 2) {
       String uvVal = (uvEnd > uv) ? line.substring(uv, uvEnd) : line.substring(uv);
       uvVal.trim();
       uvOn = (uvVal == "1");
       }
    }
  }
}


//  Original ExecuteCommand (unchanged + sani cmds added)
//  Forward declarations 
void getCommand(char c);
String tcp_http(String domain, String request, int port, byte wait);
String tcp_https(String domain, String request, int port, byte wait);
String LineNotify(String token, String request, byte wait);
String sendCapturedImageToLineNotify(String token);
void sendToESP32(String command);
void readFromESP32();
void ExecuteCommand();
void ExecuteCommand() {
  if (cmd != "getstill") {
    Serial.println("cmd= " + cmd + " ,P1= " + P1 + " ,P2= " + P2);
    Serial.println("");
  }

  //  SaniBot commands —  ESP32 to UART 
  if (cmd == "sani") {    
    String saniCommand = "CMD:" + P1;
    sendToESP32(saniCommand);
    Feedback = "OK";
  }
  
  else if (cmd == "your cmd") {
    // custom
  }
  else if (cmd == "ip") {
    Feedback = "AP IP: " + WiFi.softAPIP().toString();
    Feedback += ", ";
    Feedback += "STA IP: " + WiFi.localIP().toString();
  }
  else if (cmd == "mac") {
    Feedback = "STA MAC: " + WiFi.macAddress();
  }
  else if (cmd == "resetwifi") {
    WiFi.begin(P1.c_str(), P2.c_str());
    Serial.print("Connecting to "); Serial.println(P1);
    long int StartTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((StartTime + 5000) < millis()) break;
    }
    Feedback = "STAIP: " + WiFi.localIP().toString();
  }
  else if (cmd == "restart") {
    ESP.restart();
  }
  else if (cmd == "digitalwrite") {
    ledcDetach(P1.toInt());
    pinMode(P1.toInt(), OUTPUT);
    digitalWrite(P1.toInt(), P2.toInt());
  }
  else if (cmd == "analogwrite") {
    if (P1 = "4") {
      ledcAttach(4, 5000, 8);
      ledcWrite(4, P2.toInt());
    } else {
      ledcAttach(5, 5000, 8);
      ledcWrite(5, P2.toInt());
    }
  }
  else if (cmd == "flash") {
    ledcAttach(4, 5000, 8);
    int val = P1.toInt();
    ledcWrite(4, val);
  }
  else if (cmd == "framesize") {
    sensor_t* s = esp_camera_sensor_get();
    if      (P1 == "QQVGA") s->set_framesize(s, FRAMESIZE_QQVGA);
    else if (P1 == "HQVGA") s->set_framesize(s, FRAMESIZE_HQVGA);
    else if (P1 == "QVGA")  s->set_framesize(s, FRAMESIZE_QVGA);
    else if (P1 == "CIF")   s->set_framesize(s, FRAMESIZE_CIF);
    else if (P1 == "VGA")   s->set_framesize(s, FRAMESIZE_VGA);
    else if (P1 == "SVGA")  s->set_framesize(s, FRAMESIZE_SVGA);
    else if (P1 == "XGA")   s->set_framesize(s, FRAMESIZE_XGA);
    else if (P1 == "SXGA")  s->set_framesize(s, FRAMESIZE_SXGA);
    else if (P1 == "UXGA")  s->set_framesize(s, FRAMESIZE_UXGA);
    else                    s->set_framesize(s, FRAMESIZE_QVGA);
  }
  else if (cmd == "quality") {
    sensor_t* s = esp_camera_sensor_get();
    s->set_quality(s, P1.toInt());
  }
  else if (cmd == "contrast") {
    sensor_t* s = esp_camera_sensor_get();
    s->set_contrast(s, P1.toInt());
  }
  else if (cmd == "brightness") {
    sensor_t* s = esp_camera_sensor_get();
    s->set_brightness(s, P1.toInt());
  }
  else if (cmd == "serial") {
    Serial.println(P1);
  }
  else if (cmd == "detectCount") {
    Serial.println(P1 + " = " + P2);
  }
  else if (cmd == "tcp") {
    String domain = P1;
    int port = P2.toInt();
    String request = P3;
    int wait = P4.toInt();
    if ((port == 443) || (domain.indexOf("https") == 0))
      Feedback = tcp_https(domain, request, port, wait);
    else
      Feedback = tcp_http(domain, request, port, wait);
  }
  else if (cmd == "linenotify") {
    Feedback = LineNotify(P1, P2, 1);
  }
  else if (cmd == "sendCapturedImageToLineNotify") {
    Feedback = sendCapturedImageToLineNotify(P1);
    if (Feedback == "") Feedback = "The image failed to send.";
  }
  else {
    Feedback = "Command is not defined.";
  }
  if (Feedback == "") Feedback = Command;
}

// //  MERGED HTML — Object Detection + SaniBot UI
// static const char PROGMEM INDEX_HTML[] = R"rawliteral(

// <script></script>

// )rawliteral";

server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
});

server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
});



void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  //  UART2 init (ESP32 se communication) 
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.println("UART2 started on TX:" + String(UART_TX_PIN) + " RX:" + String(UART_RX_PIN));

  //  Camera init (unchanged) 
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size    = FRAMESIZE_UXGA;
    config.jpeg_quality  = 10;
    config.fb_count      = 2;
  } else {
    config.frame_size    = FRAMESIZE_SVGA;
    config.jpeg_quality  = 12;
    config.fb_count      = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    delay(1000);
    ESP.restart();
  }

  sensor_t* s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);

  ledcAttach(4, 5000, 8);

  //  WiFi (unchanged) 
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  delay(1000);
  Serial.print("Connecting to "); Serial.println(ssid);

  long int StartTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if ((StartTime + 10000) < millis()) break;
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.softAP((WiFi.localIP().toString() + "_" + (String)apssid).c_str(), appassword);
    Serial.println("STAIP: " + WiFi.localIP().toString());
    for (int i = 0; i < 5; i++) {
      ledcWrite(4, 10); delay(200);
      ledcWrite(4, 0);  delay(200);
    }
  } else {
    WiFi.softAP((WiFi.softAPIP().toString() + "_" + (String)apssid).c_str(), appassword);
    for (int i = 0; i < 2; i++) {
      ledcWrite(4, 10); delay(1000);
      ledcWrite(4, 0);  delay(1000);
    }
  }

  Serial.println("APIP: " + WiFi.softAPIP().toString());
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  server.begin();
}


void loop() {
    readFromESP32();
  //  Web server (original logic) 
  Feedback = ""; Command = ""; cmd = ""; P1 = ""; P2 = ""; P3 = ""; P4 = "";
  P5 = ""; P6 = ""; P7 = ""; P8 = ""; P9 = "";
  ReceiveState = 0; cmdState = 1; strState = 1;
  questionstate = 0; equalstate = 0; semicolonstate = 0;

  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        getCommand(c);

        if (c == '\n') {
          if (currentLine.length() == 0) {

            if (cmd == "getstill") {
              camera_fb_t* fb = esp_camera_fb_get();
              if (!fb) {
                Serial.println("Camera capture failed");
                delay(1000); ESP.restart();
              }
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: image/jpeg");
              client.println("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\"");
              client.println("Content-Length: " + String(fb->len));
              client.println("Connection: close");
              client.println();
              uint8_t* fbBuf = fb->buf;
              size_t fbLen = fb->len;
              for (size_t n = 0; n < fbLen; n = n + 1024) {
                if (n + 1024 < fbLen) { client.write(fbBuf, 1024); fbBuf += 1024; }
                else if (fbLen % 1024 > 0) { client.write(fbBuf, fbLen % 1024); }
              }
              esp_camera_fb_return(fb);
              pinMode(4, OUTPUT); digitalWrite(4, LOW);

            } else if (cmd == "sanistatus") {                           
              String resp = "FRONT:" + String(frontDist) +
                            ":RIGHT:" + String(rightDist) +
                            ":CMD:"   + saniCmd +
                            ":MODE:"  + saniMode +
                            ":MIST:"  + (mistOn ? "1" : "0") +
                            ":UV:"    + (uvOn   ? "1" : "0");
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Content-Type: text/plain");
              client.println("Connection: close");
              client.println();
              client.print(resp);

            } else {
              //  Normal HTML / command response 
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: text/html; charset=utf-8");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Connection: close");
              client.println();

              String Data = "";
              if (cmd != "")
                Data = Feedback;
              else
                Data = String((const char*)INDEX_HTML);

              int Index;
              for (Index = 0; Index < Data.length(); Index = Index + 1000)
                client.print(Data.substring(Index, Index + 1000));
              client.println();
            }

            Feedback = "";
            break;

          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        if ((currentLine.indexOf("/?") != -1) && (currentLine.indexOf(" HTTP") != -1)) {
          if (Command.indexOf("stop") != -1) {
            client.println(); client.println(); client.stop();
          }
          currentLine = ""; Feedback = "";
          ExecuteCommand();
        }
      }
    }
    delay(1);
    client.stop();
  }
}
  
//  Original helper functions (unchanged)
void getCommand(char c) {
  if (c == '?') ReceiveState = 1;
  if ((c == ' ') || (c == '\r') || (c == '\n')) ReceiveState = 0;

  if (ReceiveState == 1) {
    Command = Command + String(c);
    if (c == '=') cmdState = 0;
    if (c == ';') strState++;
    if ((cmdState == 1) && ((c != '?') || (questionstate == 1))) cmd = cmd + String(c);
    if ((cmdState == 0) && (strState == 1) && ((c != '=') || (equalstate == 1))) P1 = P1 + String(c);
    if ((cmdState == 0) && (strState == 2) && (c != ';')) P2 = P2 + String(c);
    if ((cmdState == 0) && (strState == 3) && (c != ';')) P3 = P3 + String(c);
    if ((cmdState == 0) && (strState == 4) && (c != ';')) P4 = P4 + String(c);
    if ((cmdState == 0) && (strState == 5) && (c != ';')) P5 = P5 + String(c);
    if ((cmdState == 0) && (strState == 6) && (c != ';')) P6 = P6 + String(c);
    if ((cmdState == 0) && (strState == 7) && (c != ';')) P7 = P7 + String(c);
    if ((cmdState == 0) && (strState == 8) && (c != ';')) P8 = P8 + String(c);
    if ((cmdState == 0) && (strState >= 9) && ((c != ';') || (semicolonstate == 1))) P9 = P9 + String(c);
    if (c == '?') questionstate = 1;
    if (c == '=') equalstate = 1;
    if ((strState >= 9) && (c == ';')) semicolonstate = 1;
  }
}

String tcp_http(String domain, String request, int port, byte wait) {
  WiFiClient client_tcp;
  if (client_tcp.connect(domain.c_str(), port)) {
    client_tcp.println("GET " + request + " HTTP/1.1");
    client_tcp.println("Host: " + domain);
    client_tcp.println("Connection: close");
    client_tcp.println();
    String getResponse = "", Feedback = "";
    boolean state = false;
    long startTime = millis();
    while ((startTime + 3000) > millis()) {
      while (client_tcp.available()) {
        char c = client_tcp.read();
        if (state == true) Feedback += String(c);
        if (c == '\n') { if (getResponse.length() == 0) state = true; getResponse = ""; }
        else if (c != '\r') getResponse += String(c);
        if (wait == 1) startTime = millis();
      }
      if (wait == 0 && state && Feedback.length() != 0) break;
    }
    client_tcp.stop(); return Feedback;
  }
  return "Connection failed";
}

String tcp_https(String domain, String request, int port, byte wait) {
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();
  if (client_tcp.connect(domain.c_str(), port)) {
    client_tcp.println("GET " + request + " HTTP/1.1");
    client_tcp.println("Host: " + domain);
    client_tcp.println("Connection: close");
    client_tcp.println();
    String getResponse = "", Feedback = "";
    boolean state = false;
    long startTime = millis();
    while ((startTime + 3000) > millis()) {
      while (client_tcp.available()) {
        char c = client_tcp.read();
        if (state == true) Feedback += String(c);
        if (c == '\n') { if (getResponse.length() == 0) state = true; getResponse = ""; }
        else if (c != '\r') getResponse += String(c);
        if (wait == 1) startTime = millis();
      }
      if (wait == 0 && state && Feedback.length() != 0) break;
    }
    client_tcp.stop(); return Feedback;
  }
  return "Connection failed";
}

String LineNotify(String token, String request, byte wait) {
  request.replace("%", "%25"); request.replace(" ", "%20");
  request.replace("&", "%20"); request.replace("#", "%20");
  request.replace("\"", "%22"); request.replace("\n", "%0D%0A");
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();
  if (client_tcp.connect("notify-api.line.me", 443)) {
    client_tcp.println("POST /api/notify HTTP/1.1");
    client_tcp.println("Connection: close");
    client_tcp.println("Host: notify-api.line.me");
    client_tcp.println("Authorization: Bearer " + token);
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println("Content-Length: " + String(request.length()));
    client_tcp.println(); client_tcp.println(request); client_tcp.println();
    String getResponse = "", Feedback = "";
    boolean state = false;
    long startTime = millis();
    while ((startTime + 3000) > millis()) {
      while (client_tcp.available()) {
        char c = client_tcp.read();
        if (state) Feedback += String(c);
        if (c == '\n') { if (getResponse.length() == 0) state = true; getResponse = ""; }
        else if (c != '\r') getResponse += String(c);
        if (wait == 1) startTime = millis();
      }
      if (wait == 0 && state && Feedback.length() != 0) break;
    }
    client_tcp.stop(); return Feedback;
  }
  return "Connection failed";
}

String sendCapturedImageToLineNotify(String token) {
  String getAll = "", getBody = "";
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { Serial.println("Camera capture failed"); delay(1000); ESP.restart(); return ""; }
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();
  if (client_tcp.connect("notify-api.line.me", 443)) {
    String message = "Welcome to Taiwan.";
    String head = "--Taiwan\r\nContent-Disposition: form-data; name=\"message\"; \r\n\r\n" + message +
                  "\r\n--Taiwan\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Taiwan--\r\n";
    uint16_t totalLen = fb->len + head.length() + tail.length();
    client_tcp.println("POST /api/notify HTTP/1.1");
    client_tcp.println("Connection: close");
    client_tcp.println("Host: notify-api.line.me");
    client_tcp.println("Authorization: Bearer " + token);
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=Taiwan");
    client_tcp.println(); client_tcp.print(head);
    uint8_t* fbBuf = fb->buf; size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) { client_tcp.write(fbBuf, 1024); fbBuf += 1024; }
      else if (fbLen % 1024 > 0) { client_tcp.write(fbBuf, fbLen % 1024); }
    }
    client_tcp.print(tail);
    esp_camera_fb_return(fb);
    long startTime = millis(); boolean state = false;
    while ((startTime + 10000) > millis()) {
      delay(100);
      while (client_tcp.available()) {
        char c = client_tcp.read();
        if (state) getBody += String(c);
        if (c == '\n') { if (getAll.length() == 0) state = true; getAll = ""; }
        else if (c != '\r') getAll += String(c);
        startTime = millis();
      }
      if (getBody.length() > 0) break;
    }
    client_tcp.stop();
  }
  return getBody;
}


