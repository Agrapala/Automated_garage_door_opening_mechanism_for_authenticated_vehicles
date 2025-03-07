#include <ESP8266WiFi.h>

const char *ssid = "GarageAccess";
const char *password = "12345678";
WiFiServer server(80);

const uint8_t allowedMAC[][6] = {
    {0xA4, 0xCF, 0x12, 0x34, 0x56, 0x78},  // Example MAC Address 1
    {0xB4, 0xE6, 0x2D, 0x90, 0xAB, 0xCD}   // Example MAC Address 2
};

const int relayPin = D1;  // Garage door relay pin

bool isAllowed(uint8_t *mac) {
    for (int i = 0; i < sizeof(allowedMAC) / sizeof(allowedMAC[0]); i++) {
        if (memcmp(mac, allowedMAC[i], 6) == 0) {
            return true;
        }
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    WiFi.softAP(ssid, password);
    server.begin();
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    Serial.println("Server Started");
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        Serial.println("Client connected");
        uint8_t mac[6];
        client.read(mac, 6);

        Serial.printf("Received MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        if (isAllowed(mac)) {
            Serial.println("Access Granted! Opening Garage.");
            digitalWrite(relayPin, HIGH);
            delay(2000);
            digitalWrite(relayPin, LOW);
            client.print("Access Granted");
        } else {
            Serial.println("Unknown Device Connected");
            client.print("Access Denied");
        }
        client.stop();
    }
}
