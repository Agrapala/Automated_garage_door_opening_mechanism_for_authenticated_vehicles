#include <ESP8266WiFi.h>

#define SERVER_IP "192.168.1.100"  // Update with Garage NodeMCU IP
#define SERVER_PORT 80

void setup() {
    Serial.begin(115200);
    WiFi.begin("Lonewolf android", "abcd123456");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
}

void loop() {
    WiFiClient client;
    if (client.connect(SERVER_IP, SERVER_PORT)) {
        String macAddress = WiFi.macAddress();
        client.println(macAddress);
        Serial.println("Sent MAC: " + macAddress);
    }

    delay(5000);
}
