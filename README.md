#TOPIC : AUTOMATED GARAGE DOOR OPENING SYSTEM - EC6020_EMBEDDED_SYSTEM

#MEMBERS: 2021E009, 2021E053, 2021E121, 2021E138

#Project Description:  
      This project focuses on designing and implementing an automated garage door opening system using embedded systems and wireless communication technologies. It 
      utilizes microcontrollers with Wi-Fi or Bluetooth modules to authenticate the owner's vehicle and automatically trigger the door mechanism upon approach, 
      offering a secure, convenient, and efficient solution for homeowners.

#Features:
      Automatic garage door opening using vehicle authentication.
      MAC address-based security for enhanced protection.
      Real-time distance measurement with sonar sensors.
      Non-blocking programming for efficient multitasking.
      
#Problem:
      Conventional garage doors require manual operation or basic remote controls, which lack advanced security and convenience, especially during bad weather or 
      when users are busy. Unauthorized access remains a critical issue.
#Solution:
      This system automates the garage door process using Wi-Fi or Bluetooth-based MAC address authentication, ensuring security and convenience. It integrates 
      sensors to detect vehicle proximity and triggers door opening automatically.

#Installation Instructions - 
      #Hardware Setup:
      Connect the NodeMCU ESP8266 microcontroller to the sonar sensor (HC-SR04), motor driver module (L9110H H-Bridge), and LEDs.
      Assemble the vehicle unit with the second NodeMCU for authentication.

#System Workflow:
      Vehicle sends a connection request to the garage system.
      The garage verifies the vehicle's MAC address.
      If authenticated, the sonar sensor measures the vehicle's distance.
      The door opens when the vehicle is within range.

#Circuit Design:
      The garage unit includes the microcontroller, sonar sensor, and motor driver module.
      The vehicle unit features a microcontroller for secure communication
