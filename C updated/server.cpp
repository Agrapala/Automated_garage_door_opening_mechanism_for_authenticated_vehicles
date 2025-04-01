#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <FirebaseESP32.h>
//#include <FirebaseESP8266.h>


#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"
#define FIREBASE_HOST "your-project-id.firebaseio.com"
#define FIREBASE_AUTH "your-firebase-secret"

#define TRIG_PIN D1
#define ECHO_PIN D2
#define DOOR_RELAY D3

FirebaseData fbdo;
WiFiServer server(80);

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(DOOR_RELAY, OUTPUT);
    
    server.begin();
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        String macAddress = client.readStringUntil('\n');  // Read MAC from vehicle

        Serial.println("Received MAC: " + macAddress);

        // Call Firebase Cloud Function
        Firebase.getBool(fbdo, "/garage_system/authorized_macs/" + macAddress);
        bool isAuthorized = fbdo.boolData();

        if (isAuthorized) {
            Serial.println("Vehicle Authorized");

            long duration;
            digitalWrite(TRIG_PIN, LOW);
            delayMicroseconds(2);
            digitalWrite(TRIG_PIN, HIGH);
            delayMicroseconds(10);
            digitalWrite(TRIG_PIN, LOW);
            duration = pulseIn(ECHO_PIN, HIGH);
            float distance = (duration * 0.0343) / 2;  // Convert to cm

            if (distance < 50) {  // Threshold
                Firebase.setBool(fbdo, "/garage_system/vehicle_detected", true);
                Serial.println("Vehicle Detected. Waiting for PIN...");
            }
        } else {
            Serial.println("Unauthorized Vehicle!");
        }

        client.stop();
    }
}
