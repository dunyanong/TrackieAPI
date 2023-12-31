/*
  Rui Santos
  Complete project details at our blog: https://RandomNerdTutorials.com/esp32-data-logging-firebase-realtime-database/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "time.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Insert Firebase project API Key
#define API_KEY "AIzaSyCmV_ZIx8mohqcbraP2Y4o_O54b-wMt_uc"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://trackie-cfff3-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String moistPath = "/soil_moist";
String sunPath = "/sun_inten";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

// Sensors
const int oneWireBus = 4;  // GPIO where the DS18B20 is connected to
#define AOUT_PIN 36 // ESP32 pin GPIO36 (ADC0) that connects to AOUT pin of moisture sensor
int soil_moisture_lvl;
float temp;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 180000;
bool signupOK = false;

void read_sensor_data(void * parameter) {
   for (;;) {
    sensors.requestTemperatures(); 
    temp = sensors.getTempCByIndex(0);
    delay(500);
    soil_moisture_lvl = analogRead(AOUT_PIN); // read the analog value from soil moisture sensor
    delay(500);
    Serial.println("Read sensor data");
 
    vTaskDelay(600 / portTICK_PERIOD_MS);
   }
}

void setup_task() {    
  xTaskCreate(     
  read_sensor_data,      
  "Read sensor data",      
  1000,      
  NULL,      
  1,     
  NULL     
  );     
}

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());
  setup_task();    
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup(){
  Serial.begin(115200);
  initWiFi();
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// Function to format timestamp to Singapore time
String formatSingaporeTime(time_t timestamp) {
    char buffer[80];

    // Singapore time (UTC+8 hours)
    timestamp += 8 * 3600; // Adding 8 hours in seconds

    struct tm timeinfo;
    gmtime_r(&timestamp, &timeinfo);

    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
            timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    return String(buffer);
}

void loop(){

  // Send new readings to database
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //Get current timestamp
    time_t timestamp = getTime();
    Serial.print("Singapore time: ");
    Serial.println(formatSingaporeTime(timestamp));
    
    parentPath= databasePath + "/" + String(formatSingaporeTime(timestamp));
    
    // Create a string including temperature value and its unit
    String temperatureString = String(temp) + " °C"; // Adding the Celsius unit

    json.set(tempPath.c_str(), temperatureString);
    json.set(moistPath.c_str(), String(soil_moisture_lvl));
    json.set(timePath, String(formatSingaporeTime(timestamp)));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}
