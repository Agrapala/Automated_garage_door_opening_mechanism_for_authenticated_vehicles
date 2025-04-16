#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include "addons/TokenHelper.h"

// WiFi credentials
const char* ssid = "FOE-Student";
const char* password = "abcd@1234";

// Firebase settings
const char* FIREBASE_HOST = "smart-garage-door-6bed5-default-rtdb.firebaseio.com";
const char* FIREBASE_AUTH = "YnO9U41SPHDTJ3lYD46UABVGFZdqlKtPqqWsM80a";

// Firebase paths
const char* DOOR_STATE_PATH = "/device/door_state";
const char* DISTANCE_PATH = "/device/distance_validated";
const char* MAC_PATH = "/device/mac_address";
const char* MAC_VALIDATED_PATH = "/device/mac_validated";
const char* PIN_VALIDATED_PATH = "/device/pin_validated";
const char* DRIVER_PIN_PATH = "/pins/driver_pin";
const char* SYSTEM_DISABLED_PATH = "/device/system_disabled";

// Hardware pins
const int TRIG_PIN = D1;   // GPIO5
const int ECHO_PIN = D2;   // GPIO4
const int SERVO_PIN = D4;  // GPIO2

// LED pins for distance indication
const int LED1_PIN = D5;  // 0-10cm
const int LED2_PIN = D6;  // 10-20cm
const int LED3_PIN = D7;  // 20-30cm
const int LED4_PIN = D8;  // 30+ cm

// Status LED pins
const int WIFI_LED = D3;    // WiFi status (GPIO0)
const int FIREBASE_LED = D0; // Firebase status (GPIO16)

// Distance threshold
const float DISTANCE_THRESHOLD = 20.0;

// Servo angles
const int SERVO_CLOSED_ANGLE = 0;
const int SERVO_OPEN_ANGLE = 180;

// MAC address management
String VALID_MAC = "";
unsigned long lastMacCheckTime = 0;
const unsigned long macCheckInterval = 10000;

// Validation variables
bool distanceValidated = false;
bool pinValidated = false;
bool macValidated = false;
bool lastValidationState = false;
bool wasPreviouslyValidated = false;
bool systemDisabled = false;
String lastDoorState = "";

ESP8266WebServer server(80);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Servo garageDoorServo;

// Function prototypes
void validateMAC();
void handleClientMAC();
void checkForExpiredMAC();
void updateFirebaseDistanceStatus();
void checkPinValidation();
float measureDistance();
void controlServo(bool openDoor);
void generateDriverPIN();
void updateDistanceLEDs(float distance);
void checkConnectionStatus();
void printConnectionStatus();
void checkSystemStatus();
void checkDoorState();

void handleClientMAC() {
  if (server.hasArg("mac")) {
    VALID_MAC = server.arg("mac");
    Serial.println("Received MAC: " + VALID_MAC);
    lastMacCheckTime = millis();
    server.send(200, "text/plain", "MAC received: " + VALID_MAC);
    validateMAC();
  } else {
    server.send(400, "text/plain", "MAC missing");
  }
}

void validateMAC() {
  if (VALID_MAC.length() == 0) return;
  
  if (Firebase.getString(fbdo, MAC_PATH)) {
    if (fbdo.dataType() == "string") {
      macValidated = VALID_MAC.equalsIgnoreCase(fbdo.stringData());
      Firebase.setBool(fbdo, MAC_VALIDATED_PATH, macValidated);
    }
  }
}

void checkForExpiredMAC() {
  if (VALID_MAC.length() > 0 && (millis() - lastMacCheckTime) > macCheckInterval) {
    VALID_MAC = "";
    Firebase.setBool(fbdo, MAC_VALIDATED_PATH, false);
    macValidated = false;
  }
}

void updateFirebaseDistanceStatus() {
  Firebase.setBool(fbdo, DISTANCE_PATH, distanceValidated);
}

void checkPinValidation() {
  if (Firebase.getBool(fbdo, PIN_VALIDATED_PATH) && fbdo.dataType() == "boolean") {
    pinValidated = fbdo.boolData();
  }
}

void checkSystemStatus() {
  if (Firebase.getBool(fbdo, SYSTEM_DISABLED_PATH) && fbdo.dataType() == "boolean") {
    systemDisabled = fbdo.boolData();
  }
}

void checkDoorState() {
  if (Firebase.getString(fbdo, DOOR_STATE_PATH)) {
    if (fbdo.dataType() == "string") {
      String currentDoorState = fbdo.stringData();
      if (currentDoorState != lastDoorState) {
        lastDoorState = currentDoorState;
        if (systemDisabled) {
          // When system is disabled, directly control servo based on door state
          if (currentDoorState == "open") {
            garageDoorServo.write(SERVO_OPEN_ANGLE);
            Serial.println("System disabled - door forced open");
          } else {
            garageDoorServo.write(SERVO_CLOSED_ANGLE);
            Serial.println("System disabled - door forced closed");
          }
        }
      }
    }
  }
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;
  
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  return distance;
}

void controlServo(bool openDoor) {
  if (systemDisabled) {
    // Don't control servo based on validations when system is disabled
    return;
  }
  
  if (openDoor) {
    garageDoorServo.write(SERVO_OPEN_ANGLE);
    Firebase.setString(fbdo, DOOR_STATE_PATH, "open");
    Serial.println("Door opened");
  } else {
    garageDoorServo.write(SERVO_CLOSED_ANGLE);
    Firebase.setString(fbdo, DOOR_STATE_PATH, "closed");
    Serial.println("Door closed");
  }
}

void generateDriverPIN() {
  if (systemDisabled) return;
  
  int newPin = random(1000, 10000);
  if (Firebase.setString(fbdo, DRIVER_PIN_PATH, String(newPin))) {
    Serial.println("Updated driver PIN: " + String(newPin));
  }
}

void updateDistanceLEDs(float distance) {
  // Turn off all LEDs first
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  digitalWrite(LED4_PIN, LOW);
  
  // Light up LEDs based on distance ranges
  if (distance <= 10.0) {
    digitalWrite(LED1_PIN, HIGH);
  } else if (distance <= 20.0) {
    digitalWrite(LED2_PIN, HIGH);
  } else if (distance <= 30.0) {
    digitalWrite(LED3_PIN, HIGH);
  } else {
    digitalWrite(LED4_PIN, HIGH);
  }
}

void checkConnectionStatus() {
  // WiFi status
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFI_LED, HIGH);
  } else {
    digitalWrite(WIFI_LED, LOW);
  }
  
  // Firebase status
  if (Firebase.ready()) {
    digitalWrite(FIREBASE_LED, HIGH);
  } else {
    digitalWrite(FIREBASE_LED, LOW);
  }
}

void printConnectionStatus() {
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 1000) { // Print every 5 seconds
    Serial.println("\n--- Connection Status ---");
    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.print("Firebase: ");
    Serial.println(Firebase.ready() ? "Connected" : "Disconnected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("System Status: ");
    Serial.println(systemDisabled ? "DISABLED" : "ENABLED");
    Serial.print("Door State: ");
    Serial.println(lastDoorState);
    Serial.println("-------------------------");
    lastPrintTime = millis();
  }
}

void setup() {
  Serial.begin(9600);
  
  // Initialize hardware pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);
  pinMode(FIREBASE_LED, OUTPUT);
  
  // Initialize servo
  garageDoorServo.attach(SERVO_PIN);
  garageDoorServo.write(SERVO_CLOSED_ANGLE);
  lastDoorState = "closed";
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  
  // Start web server
  server.on("/update_mac", HTTP_GET, handleClientMAC);
  server.begin();
  Serial.println("HTTP server started");

  // Initialize Firebase
  Serial.println("Connecting to Firebase...");
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  while (!Firebase.ready()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nFirebase connected!");

  // Seed random number generator
  randomSeed(analogRead(A0));
  
  // Initial status LEDs
  digitalWrite(WIFI_LED, HIGH);
  digitalWrite(FIREBASE_LED, HIGH);
}

void loop() {
  server.handleClient();
  checkForExpiredMAC();
  checkSystemStatus();
  checkDoorState(); // Check for direct door state changes
  
  // Only proceed with normal operations if system is not disabled
  if (!systemDisabled) {
    // Measure distance and update LEDs
    float currentDistance = measureDistance();
    distanceValidated = (currentDistance <= DISTANCE_THRESHOLD);
    updateFirebaseDistanceStatus();
    updateDistanceLEDs(currentDistance);
    
    // Check MAC validation
    if (VALID_MAC.length() > 0 && Firebase.getString(fbdo, MAC_PATH)) {
      if (fbdo.dataType() == "string") {
        macValidated = VALID_MAC.equalsIgnoreCase(fbdo.stringData());
        Firebase.setBool(fbdo, MAC_VALIDATED_PATH, macValidated);
      }
    }
    
    checkPinValidation();

    // Final validation (MAC + distance + PIN)
    bool currentValidationState = (macValidated && distanceValidated && pinValidated);
    
    // Generate new driver PIN only when transitioning from invalid to valid state
    if (currentValidationState && !wasPreviouslyValidated) {
      generateDriverPIN();
    }
    wasPreviouslyValidated = currentValidationState;
    
    // Control servo
    if (currentValidationState != lastValidationState) {
      controlServo(currentValidationState);
      lastValidationState = currentValidationState;
    }
  }
  
  // Check and display connection status
  checkConnectionStatus();
  printConnectionStatus();
  
  delay(200);
}