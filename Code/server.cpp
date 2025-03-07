#include <ESP8266WiFi.h>

const char* ssid = "test";
const char* password = "test1234";
const int serverPort = 8888;
const int connectionTimeout = 5000; 

const int ledPin = LED_BUILTIN;
const int buttonPin = D5;

IPAddress staticIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(serverPort);
bool override = false;

unsigned long lastActivityTime = 0;

void setup() {
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
    pinMode(buttonPin, INPUT);
    digitalWrite(ledPin, HIGH);

    WiFi.softAPConfig(staticIP, gateway, subnet);
    WiFi.softAP(ssid, password);
    server.begin();
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        digitalWrite(ledPin, LOW);
        lastActivityTime = millis();
        
        while (client.connected()) {
            if (digitalRead(buttonPin) == LOW) {
                Serial.println("Overridden!");
                override = true;
            }

            if (client.available()) {
                client.read(); 

                if (override) {
                    override = false;
                    client.println("Keep alive and Open");
                } else {
                    client.println("Keep alive");
                    lastActivityTime = millis();
                }
            }

            if (millis() - lastActivityTime > connectionTimeout) {
                break;
            }
        }

        client.stop();
        digitalWrite(ledPin, HIGH);
    }
}
