#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

// === CONFIG ===
const char* ssid = "CHANGESSID";
const char* password = "ChangePASS";

#define PIN_D4 D4  // On NodeMCU, D4 = GPIO2

// === SERVER & BLINK ===
ESP8266WebServer server(80);
unsigned long blinkInterval = 300000; // default 1s
unsigned long lastToggle = 0;
bool ledState = false;

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8" />
      <title>ESP8266 Blink Control</title>
      <style>
        body {
          background-color: #121212;
          color: #e0e0e0;
          font-family: Arial, sans-serif;
          text-align: center;
          padding: 50px;
        }
        input {
          padding: 10px;
          font-size: 1em;
        }
        button {
          padding: 10px 20px;
          font-size: 1em;
          margin-top: 10px;
        }
      </style>
    </head>
    <body>
      <h1>NFT Tower Controll</h1>
      <p>Current interval: )rawliteral" + String(blinkInterval) + R"rawliteral(ms)</p>
      <form method='POST' action='/set'>
        <input type='number' name='interval' min='100' value=')rawliteral" + String(blinkInterval) + R"rawliteral(' required>
        <button type='submit'>Set Interval</button>
      </form>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("interval")) {
    blinkInterval = server.arg("interval").toInt();
    if (blinkInterval < 100) blinkInterval = 100; // minimum 100ms
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_D4, OUTPUT);

  // === WiFi ===
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // === OTA ===
  ArduinoOTA.setHostname("ESP8266-Blink");
  ArduinoOTA.begin();
  Serial.println("OTA Ready!");

  // === Web Server ===
  server.on("/", handleRoot);
  server.on("/set", HTTP_POST, handleSet);
  server.begin();
  Serial.println("HTTP server started.");

}

void loop() {
  // === OTA ===
  ArduinoOTA.handle();

  // === Server ===
  server.handleClient();

  // === Blink Control ===
  unsigned long now = millis();
  if (now - lastToggle >= blinkInterval) {
    ledState = !ledState;
    digitalWrite(PIN_D4, ledState ? HIGH : LOW);
    lastToggle = now;
  }
}
