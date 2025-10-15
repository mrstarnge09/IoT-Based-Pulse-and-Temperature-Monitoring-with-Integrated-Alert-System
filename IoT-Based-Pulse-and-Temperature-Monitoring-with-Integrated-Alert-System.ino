// -----------------------------------------------------------------------
//  CONFIGURATION
//  **Update these with your actual Wi-Fi credentials**
// -----------------------------------------------------------------------
char ssid[] = "YOUR_WIFI_SSID";           // Your WiFi Network Name
char password[] = "YOUR_WIFI_PASSWORD";   // Your WiFi Password

// Your ThingSpeak Channel ID (provided)
unsigned long myChannelNumber = shanti; 

// Your provided Write API Key (for Field 1: BPM, Field 2: Temp)
const char * myWriteAPIKey = "om"; 

// -----------------------------------------------------------------------
//  PIN DEFINITIONS & LIBRARIES
// -----------------------------------------------------------------------
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include "DS18B20.h"        // Assumes you are using this specific DS18B20 library
#include "pulse-sensor-arduino.h"

// DS18B20 on NodeMCU D1 (GPIO 5) - Connect Data Pin here
#define DS18B20WP_PIN_DQ    5
// Pulse Sensor on NodeMCU A0 (ADC) - Connect Signal Pin here
#define HEARTPULSE_PIN_SIG  A0 

// -----------------------------------------------------------------------
//  OBJECTS & VARIABLES
// -----------------------------------------------------------------------
DS18B20 ds18b20wp(DS18B20WP_PIN_DQ);
PulseSensor heartpulse;

WiFiClient  client;

// Time variables for non-blocking delay
unsigned long lastUpdate = 0;
const long updateInterval = 20000; // 20 seconds (20000 milliseconds)

// -----------------------------------------------------------------------
//  SETUP
// -----------------------------------------------------------------------
void setup() 
{
    Serial.begin(115200);
    delay(10);
    Serial.println("\n--- IoT Vital Monitor Start ---");
    
    // Initialize Pulse Sensor
    heartpulse.begin(HEARTPULSE_PIN_SIG);

    // Connect to Wi-Fi
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    ThingSpeak.begin(client);
}

// -----------------------------------------------------------------------
//  MAIN LOOP
// -----------------------------------------------------------------------
void loop() 
{
    // Check if it's time to send data (non-blocking)
    if (millis() - lastUpdate > updateInterval) 
    {
        // 1. Read Temperature
        float tempC = ds18b20wp.readTempC();

        // 2. Read Heart Rate (Pulse Sensor updates its BPM value continuously in the background)
        if (heartpulse.QS == true) {
            Serial.println("Pulse Detected!");
            heartpulse.QS = false; // Reset the flag
        }
        int bpm = heartpulse.getBPM(); // Get the latest BPM value

        // 3. Print values to Serial Monitor
        Serial.print("Heart Rate (BPM): ");
        Serial.println(bpm);
        Serial.print("Temperature (C): ");
        Serial.println(tempC);

        // 4. Send data to ThingSpeak
        // -127 is a common error code for DS18B20 when it fails to read.
        if (tempC > -127.0 && bpm > 0) { 
            
            // ThingSpeak Fields: 1st parameter is Field 1 (BPM), 2nd is Field 2 (Temp)
            int statusCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey, bpm, tempC);

            if (statusCode == 200) {
                Serial.println("ThingSpeak Update Successful. ✅");
            } else {
                Serial.print("ThingSpeak Update Failed (Error Code: ");
                Serial.print(statusCode);
                Serial.println(") ❌");
            }
        } else {
            Serial.println("Invalid sensor data. Skipping ThingSpeak update.");
        }

        // 5. Update the timer
        lastUpdate = millis();
    }
    
    // Minimal delay required for the Pulse Sensor library to work correctly
    delay(10); 
}
