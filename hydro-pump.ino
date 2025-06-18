#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <time.h>

// WiFi credentials
const char* ssid = "CHANGESSID";
const char* password = "changePSWRD";

// Relay pin
const int relayPin = 13;

// Variables to store time intervals (in milliseconds)
unsigned long relayOnTime = 1000; // Default: 1 second ON
unsigned long relayOffTime = 1000; // Default: 1 second OFF

// Timer variables
unsigned long previousMillis = 0;
bool relayState = false;
bool overrideMode = false; // Flag to override automatic control
bool overrideState = false; // State of the relay when in override mode

// Web server
AsyncWebServer server(80);

void setup() {
  // Setup relay as an output and turn it off
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Assume LOW is ON for relay
  
  // Connect to Wi-Fi
  connectToWiFi();

  // Setup OTA (Over-The-Air updates)
  ArduinoOTA.begin();

  // Start the web server
  startWebServer();

  // Initialize time (NTP)
  configTime(0, 0, "pool.ntp.org");  // Set time from NTP server
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();

  // If override mode is active, the relay state is controlled manually
  if (overrideMode) {
    digitalWrite(relayPin, overrideState ? LOW : HIGH);  // LOW = ON, HIGH = OFF
    return;
  }

  // Handle relay timing (automatic mode)
  unsigned long currentMillis = millis();
  if (relayState && currentMillis - previousMillis >= relayOnTime) {
    previousMillis = currentMillis;
    digitalWrite(relayPin, HIGH); // Turn OFF relay
    relayState = false;
  } else if (!relayState && currentMillis - previousMillis >= relayOffTime) {
    previousMillis = currentMillis;
    digitalWrite(relayPin, LOW);  // Turn ON relay
    relayState = true;
  }
}

// WiFi connection
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
}

// Web server interface to display relay states, input time intervals, and toggle manual control
void startWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String htmlPage = "<html><head><title>ESP32 Relay Control</title></head><body>";
    
    // Display current time
    time_t now = time(nullptr);
    struct tm *currentTime = localtime(&now);
    String currentTimeStr = String(currentTime->tm_hour) + ":" + String(currentTime->tm_min) + ":" + String(currentTime->tm_sec);
    htmlPage += "<h1>ESP32 Relay Control</h1>";
    htmlPage += "<p><strong>Current Time:</strong> " + currentTimeStr + "</p>";
    
    // Display relay status
    String relayStateStr = digitalRead(relayPin) == LOW ? "ON" : "OFF";
    htmlPage += "<p><strong>Relay Status:</strong> " + relayStateStr + "</p>";
    
    // Form to input relay ON/OFF intervals
    htmlPage += "<form action='/setIntervals' method='POST'>";
    htmlPage += "<p><label>Relay ON time (in seconds): </label><input type='number' name='onTime' value='" + String(relayOnTime / 1000) + "'></p>";
    htmlPage += "<p><label>Relay OFF time (in seconds): </label><input type='number' name='offTime' value='" + String(relayOffTime / 1000) + "'></p>";
    htmlPage += "<p><input type='submit' value='Set Intervals'></p>";
    htmlPage += "</form>";

    // Toggle buttons for manual control
    htmlPage += "<form action='/toggleRelay' method='POST'>";
    htmlPage += "<p><input type='submit' name='toggle' value='" + String(overrideMode ? (overrideState ? "Force OFF" : "Force ON") : "Override ON") + "'></p>";
    htmlPage += "</form>";
    
    htmlPage += "</body></html>";
    
    request->send(200, "text/html", htmlPage);
  });

  // Handle form submission for setting intervals
  server.on("/setIntervals", HTTP_POST, [](AsyncWebServerRequest *request) {
    String onTimeParam = request->getParam("onTime", true)->value();
    String offTimeParam = request->getParam("offTime", true)->value();

    relayOnTime = onTimeParam.toInt() * 1000; // Convert seconds to milliseconds
    relayOffTime = offTimeParam.toInt() * 1000; // Convert seconds to milliseconds

    request->send(200, "text/html", "<html><body><h1>Intervals Updated!</h1><p>Relay ON time: " + onTimeParam + " seconds</p><p>Relay OFF time: " + offTimeParam + " seconds</p><p><a href='/'>Go Back</a></p></body></html>");
  });

  // Handle toggle relay request for manual override
  server.on("/toggleRelay", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (overrideMode) {
      // Toggle between ON and OFF in override mode
      overrideState = !overrideState;
    } else {
      // Enable override mode
      overrideMode = true;
      overrideState = false;  // Initially off in override mode
    }

    String toggleMessage = overrideMode ? (overrideState ? "Relay forced ON" : "Relay forced OFF") : "Relay back to auto control";
    request->send(200, "text/html", "<html><body><h1>" + toggleMessage + "</h1><p><a href='/'>Go Back</a></p></body></html>");
  });

  server.begin();
}
