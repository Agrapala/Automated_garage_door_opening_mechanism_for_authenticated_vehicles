#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// WiFi credentials
const char* ssid = "FOE-Student";
const char* password = "abcd@1234";

// Server details
const char* serverIP = "10.102.19.139"; // Replace with your server's IP
const int serverPort = 80;
const String endpoint = "/update_mac";

String macAddress = ""; // Global variable

// 🔥 Add this function prototype (declaration) before setup()
void sendMACToServer(String mac);

void setup() {
  Serial.begin(9600);
  delay(1000);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Get and print MAC address
  macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
  Serial.print("MAC Address: ");
  Serial.println(macAddress);

  sendMACToServer(macAddress); // Now this will work!
}

void loop() {
  static unsigned long lastSendTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastSendTime >= 10000) { // Every 10 seconds
    if (WiFi.status() == WL_CONNECTED) {
      sendMACToServer(macAddress);
    } else {
      Serial.println("WiFi disconnected, attempting to reconnect...");
      WiFi.begin(ssid, password);
      unsigned long startAttemptTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
        delay(500);
        Serial.print(".");
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nReconnected to WiFi!");
      } else {
        Serial.println("\nFailed to reconnect");
      }
    }
    lastSendTime = currentTime;
  }
}

// Function definition (implementation)
void sendMACToServer(String mac) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    String url = "http://" + String(serverIP) + endpoint + "?mac=" + mac;
    Serial.print("Sending request to: ");
    Serial.println(url);

    if (http.begin(client, url)) {
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        Serial.printf("HTTP response code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println("Server response: " + payload);
        }
      } else {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.println("Unable to connect to server");
    }
  } else {
    Serial.println("WiFi not connected");
  }
}
https://smart-garage-door-6bed5-default-rtdb.firebaseio.com/
