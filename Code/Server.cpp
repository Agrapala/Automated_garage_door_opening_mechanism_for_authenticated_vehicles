#include <ESP8266WiFi.h>

const int triggerPin = D6;
const int echoPin = D5;

const char* ssid = "test";
const char* password = "test1234";
const char* serverIP = "192.168.1.1";
const int serverPort = 8888;

const uint8_t serverMAC[] = {0x9A, 0xF4, 0xAB, 0xF5, 0x3C, 0x30};

int distance = 0;

// Timer constants
const unsigned long connectionTimeout = 4000;
const unsigned long keepAliveInterval = 3000;
const unsigned long triggerDuration = 12;
const unsigned long distanceTimeout = 38000;
const unsigned long doorDuration = 10000;
const unsigned long doorOpenDuration = 20000;

// Timer variables
unsigned long connectionTimer = 0;
unsigned long keepAliveTimer = 0;
unsigned long triggerTimer = 0;
unsigned long distanceTimer = 0;
unsigned long doorTimer = 0;

// State variables and flags
bool isConnected = false;
bool isCloseEnough = false;
bool override = false;
bool updateDistance = false;
int WiFiConnectionState = 0;
int sonarState = 0;
int doorState = 0;

const String doorPass = "Keep alive";
const String doorPassOpenOverride = "Keep alive and Open";

WiFiClient client;

// Pin configuration
const int sonarVcc = D7;
const int thresholdLED[] = {D0, D1, D2, D3};
const int doorOpen = D4;
const int doorClose = D8;

void setUpdateDistance() {
  updateDistance = true;
}

void setup() {
  Serial.begin(9600);
  delay(100);

  Serial.print("ESP8266 MAC Address: ");
  Serial.println(WiFi.macAddress());

  WiFi.begin(ssid, password);
  WiFiConnectionState = 0;

  pinMode(sonarVcc, OUTPUT);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(thresholdLED[0], OUTPUT);
  pinMode(thresholdLED[1], OUTPUT);
  pinMode(thresholdLED[2], OUTPUT);
  pinMode(thresholdLED[3], OUTPUT);
  pinMode(doorOpen, OUTPUT);
  pinMode(doorClose, OUTPUT);

  digitalWrite(sonarVcc, HIGH);
  digitalWrite(triggerPin, LOW);
  digitalWrite(thresholdLED[0], LOW);
  digitalWrite(thresholdLED[1], LOW);
  digitalWrite(thresholdLED[2], LOW);
  digitalWrite(thresholdLED[3], LOW);
  digitalWrite(doorOpen, LOW);
  digitalWrite(doorClose, LOW);

  timer1_attachInterrupt(setUpdateDistance);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
  timer1_write(156250);
}

void loop() {
  // Manage WiFi connection
  switch (WiFiConnectionState) {
    case 0:
      isConnected = false;
      if (WiFi.status() == WL_CONNECTED) {
        const uint8_t *mac = WiFi.BSSID();
        Serial.print("Connected to WiFi, Server MAC Address: ");
        Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n", 
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
        if (memcmp(mac, serverMAC, 6) == 0) WiFiConnectionState = 1;
        else ESP.restart();
      }
      break;

    case 1:
      if (client.connect(serverIP, serverPort)) {
        WiFiConnectionState = 2;
        connectionTimer = millis();
        keepAliveTimer = millis();
      } else {
        WiFiConnectionState = 0;
      }
      break;

    case 2:
      if (client.connected()) {
        if (keepAliveInterval < millis() - keepAliveTimer)  {
          keepAliveTimer = millis();
          client.print("A");
        }

        if (client.available()) {
          String response = client.readStringUntil('\n');
          response.trim();
          
          if(response.compareTo(doorPass) == 0) {
            isConnected = true;
            Serial.println("Correct password received");
          }
          else if(response.compareTo(doorPassOpenOverride) == 0) {
            isConnected = true;
            override = true;
            Serial.println("Correct password received with override");
          }
          else {
            Serial.print("Password incorrect");
            ESP.restart();
          }

          connectionTimer = millis();
        }

        if (millis() - connectionTimer > connectionTimeout) {
          isConnected = false;
          WiFiConnectionState = 0;
        }

      } else {
        WiFiConnectionState = 0;
      }
      break;

    default:
      WiFiConnectionState = 0;
      break;
  }

  // Measure distance
  if(isConnected) {
    switch (sonarState) {
      case 0:
        isCloseEnough = false;
        if (digitalRead(echoPin) == LOW && updateDistance == true) sonarState = 1;
        break;

      case 1:
        digitalWrite(triggerPin, HIGH);
        triggerTimer = micros();
        sonarState = 2;
        break;

      case 2:
        if (micros() - triggerTimer > triggerDuration) {
          digitalWrite(triggerPin, LOW);
          sonarState = 3;
        }
        break;

      case 3:
        if (digitalRead(echoPin) == HIGH) {
          distanceTimer = micros();
          sonarState = 4;
        } else if (micros() - triggerTimer > distanceTimeout) {
          Serial.println("Echo signal too narrow");
          updateDistance = false;
          sonarState = 0;
        }
        break;

      case 4:
        if (digitalRead(echoPin) == LOW) {
          unsigned long duration = micros() - distanceTimer;
          distance = duration / 58.2;
          
          Serial.print("Distance: ");
          Serial.print(distance);
          Serial.println(" cm");

          // Adjusted LED lighting for the range of 10 to 50 cm
          if (distance <= 50 && distance > 40) digitalWrite(thresholdLED[0], HIGH); // 40-50 cm
          else digitalWrite(thresholdLED[0], LOW);

          if (distance <= 40 && distance > 30) digitalWrite(thresholdLED[1], HIGH); // 30-40 cm
          else digitalWrite(thresholdLED[1], LOW);

          if (distance <= 30 && distance > 20) digitalWrite(thresholdLED[2], HIGH); // 20-30 cm
          else digitalWrite(thresholdLED[2], LOW);

          if (distance <= 20 && distance >= 10) digitalWrite(thresholdLED[3], HIGH); // 10-20 cm
          else digitalWrite(thresholdLED[3], LOW);

          isCloseEnough = (distance <= 10);
          sonarState = 0;
        } else if (micros() - distanceTimer > distanceTimeout) {
          Serial.println("Echo signal too wide");
          digitalWrite(thresholdLED[0], LOW);
          digitalWrite(thresholdLED[1], LOW);
          digitalWrite(thresholdLED[2], LOW);
          digitalWrite(thresholdLED[3], LOW);
          sonarState = 0;
        }
        updateDistance = false;
        break;
    }
  }
  else {
    digitalWrite(triggerPin, LOW);
    sonarState = 0;
    digitalWrite(thresholdLED[0], LOW);
    digitalWrite(thresholdLED[1], LOW);
    digitalWrite(thresholdLED[2], LOW);
    digitalWrite(thresholdLED[3], LOW);
  }

  // Control garage door
  switch(doorState){
    case 0:
      digitalWrite(doorOpen, LOW);
      digitalWrite(doorClose, LOW);

      if (isConnected && (isCloseEnough || override)) {
        override = false;
        Serial.println("Opening door...");
        doorState = 1;
        doorTimer = millis();
      }
      break;

    case 1:
      digitalWrite(doorClose, LOW);
      digitalWrite(doorOpen, HIGH);
      override = false;

      if (!isConnected) {
        Serial.println("Disconnected, closing door...");
        doorState = 4;
        doorTimer = millis() + millis() - doorTimer;
      }
      else if (millis() - doorTimer > doorDuration) {
        doorState = 2;
        doorTimer = millis();
      }
      break;

    case 2:
      Serial.println("Door Opened...");
      digitalWrite(doorOpen, LOW);
      digitalWrite(doorClose, LOW);
      doorTimer = millis();
      doorState = 3;
      override = false;
      break;

    case 3:
      override = false;
      
      if (!isConnected || millis() - doorTimer > doorOpenDuration) {
        Serial.println("Closing door...");
        doorTimer = millis() + doorDuration;
        doorState = 4;
      }
      break;

    case 4:
      digitalWrite(doorOpen, LOW);
      digitalWrite(doorClose, HIGH);

      if (doorTimer < millis()) {
        Serial.println("Door Closed...");
        doorState = 0;
        doorTimer = millis();
      }
      if (isConnected && (isCloseEnough || override)) {
        override = false;
        Serial.println("Opening door...");
        doorState = 1;
        doorTimer = millis() + millis() - doorTimer;
      }
      break;
  }
}
