#define BLYNK_TEMPLATE_ID "TMPL3ZqOlNU43"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "WTTy8EhFFgyaaWtjoJL4-2pWvcmJyHcH"

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <BlynkSimpleEsp32.h>
#include "FS.h"
#include "SPIFFS.h"

char ssid[] = "realme GT 6";
char pass[] = "11221122";

// Pin Definitions
#define PIR_PIN 13
#define REED_SWITCH_PIN 12
#define RED_LED 14
#define BUZZER_PIN 4

// Camera Pin Configuration
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);
WidgetTerminal terminal(V3);

bool systemArmed = true;
bool lastMotion = false;
bool reedTriggered = false;

void printBoth(String msg) {
  Serial.println(msg);
  terminal.println(msg);
  terminal.flush();
}

// === BLYNK CONTROL ===

BLYNK_WRITE(V1) {
  int val = param.asInt();
  systemArmed = (val == 1);
  if (systemArmed) {
    printBoth("System Armed");
    Blynk.logEvent("system_armed", "System Armed!");
  } else {
    printBoth("System Disarmed");
    Blynk.logEvent("system_disarmed", "System Disarmed!");
  }
}

BLYNK_WRITE(V5) {
  int val = param.asInt();
  if (val == 1) {
    capturePhoto("Manual photo requested.");
    Blynk.virtualWrite(V5, 0);
  }
}

// === SETUP ===

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  pinMode(PIR_PIN, INPUT);
  pinMode(REED_SWITCH_PIN, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  printBoth("WiFi Connected: " + WiFi.localIP().toString());
  Blynk.virtualWrite(V4, "http://" + WiFi.localIP().toString() + "/stream");

  // SPIFFS init
  if (!SPIFFS.begin(true)) {
    printBoth("SPIFFS Mount Failed");
  } else {
    printBoth("SPIFFS Initialized");
  }

  // Camera init
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    printBoth("Camera init failed");
    while (true);
  }

  printBoth("Camera ready");
  startCameraServer();
}

// === MAIN LOOP ===

void loop() {
  Blynk.run();
  server.handleClient();

  bool motionDetected = digitalRead(PIR_PIN) == HIGH;
  bool reedOpen = digitalRead(REED_SWITCH_PIN) == HIGH;

  // Notify reed open only once
  if (reedOpen && !reedTriggered) {
    reedTriggered = true;
    if (systemArmed) {
      printBoth("Reed Switch Open");
      Blynk.logEvent("reed_open", "Door/Window opened");
    }
  }
  if (!reedOpen) reedTriggered = false;

  // Motion-only when disarmed
  if (!systemArmed && motionDetected && !lastMotion) {
    lastMotion = true;
    printBoth("Motion detected (disarmed)");
    Blynk.logEvent("motion_only", "Motion detected");
  } else if (!motionDetected) {
    lastMotion = false;
  }

  // Intrusion = reed open + motion while armed
  if (systemArmed && reedOpen && motionDetected) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    printBoth("Intrusion detected. Capturing...");
    Blynk.virtualWrite(V3, "INTRUSION at " + String(millis() / 1000) + "s");
    Blynk.logEvent("intrusion", "Intruder!");

    capturePhoto("Intrusion detected");

    delay(8000);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// === CAPTURE FUNCTION ===

void capturePhoto(String label) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    printBoth("Photo capture failed");
    return;
  }

  String path = "/photo_" + String(millis()) + ".jpg";
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    printBoth("Failed to open file in write mode");
  } else {
    file.write(fb->buf, fb->len);
    file.close();
    printBoth("Saved photo: " + path);
  }

  printBoth("Photo captured: " + label);
  Blynk.virtualWrite(V2, label + " at " + String(millis() / 1000) + "s");
  Blynk.virtualWrite(V6, "Fake photo preview [Saved to SPIFFS]");
  esp_camera_fb_return(fb);
}

// === CAMERA SERVER ===

void startCameraServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html",
      "<html><body><h2>ESP32-CAM Stream</h2><img src='/stream'></body></html>");
  });

  server.on("/stream", HTTP_GET, []() {
    WiFiClient client = server.client();
    String response = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    server.sendContent(response);

    while (client.connected()) {
      camera_fb_t * fb = esp_camera_fb_get();
      if (!fb) continue;
      server.sendContent("--frame\r\nContent-Type: image/jpeg\r\n\r\n");
      server.sendContent((const char*)fb->buf, fb->len);
      server.sendContent("\r\n");
      esp_camera_fb_return(fb);
      delay(150);
    }
  });

  server.on("/capture", HTTP_GET, []() {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      server.send(500, "text/plain", "Capture failed");
      return;
    }
    server.sendHeader("Content-Type", "image/jpeg");
    server.send_P(200, "image/jpeg", (char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  });

  server.on("/list", HTTP_GET, []() {
    File root = SPIFFS.open("/");
    String output = "<h2>Saved Photos:</h2>";
    File file = root.openNextFile();
    while (file) {
      output += "<a href='/photo?name=" + String(file.name()) + "'>" + String(file.name()) + "</a><br>";
      file = root.openNextFile();
    }
    server.send(200, "text/html", output);
  });

  server.on("/photo", HTTP_GET, []() {
    if (!server.hasArg("name")) {
      server.send(400, "text/plain", "Missing filename");
      return;
    }
    String path = server.arg("name");
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
      server.send(404, "text/plain", "File not found");
      return;
    }
    server.streamFile(file, "image/jpeg");
    file.close();
  });

  server.begin();
}
