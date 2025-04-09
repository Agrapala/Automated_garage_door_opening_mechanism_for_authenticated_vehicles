#include <ESP8266WiFi.h>

#define WIFI_SSID "FOE-Student"
#define WIFI_PASSWORD "abcd@1234"
#define SERVER_IP "10.102.19.140"  // Replace with your server NodeMCU's IP
#define SERVER_PORT 80
#define CLIENT_MAC WiFi.macAddress()

WiFiClient client;

void setup() {
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nConnected to WiFi");
}

void loop() {
    Serial.println("Attempting to connect to server...");

    if (client.connect(SERVER_IP, SERVER_PORT)) {  // Check if connection is successful
        Serial.println("✅ Connected to Server!");
        client.println(CLIENT_MAC);  // Send MAC address
        client.flush();
        Serial.println("MAC Sent: " + CLIENT_MAC);
        client.stop();  // Close the connection after sending
    } else {
        Serial.println("❌ Failed to connect to server!");
    }

    delay(5000);  // Retry every 5 seconds
}