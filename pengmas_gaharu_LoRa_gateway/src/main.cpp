#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <time.h>
#include <WiFi.h>
#define DEVICE_TOTAL 10
/* FIREBASE START */
// FIREBASE HEADER
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" // Provide the token generation process info.
#include "addons/RTDBHelper.h"  // Provide the RTDB payload printing info and other helper functions.

// FIREBASE SETUP
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

struct DEVICE
{
  // int id_int;
  String id, name, name_root, ph_root, temperature_root;
  float ph, temperature;
  bool isEmpty = true;

} device[DEVICE_TOTAL];

/* 1. Define the WiFi credentials */
#define WIFI_SSID "mywifi"
#define WIFI_PASSWORD "mypass"
/* 2. Define the API Key */
#define API_KEY "myapi"
/* 3. Define the RTDB URL */
#define DATABASE_URL "myurl"
/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "myemail.com"
#define USER_PASSWORD "mypass"

// Function List
void firebaseSetFloatAsync(String, float);
void receive_parse_to_device(const String &data_received, int &id, float &temperature, float &ph);
void firebaseSetStringAsync(String databaseDirectory, String value);

// Root directory
String device_root = "/devices/";
String ph_root_to_firebase = "";
String temperature_root_to_firebase = "";
String name_root_to_firebase = "";
/* FIREBASE END */

/* LORA START */
// define the pins used by the transceiver module
#define ss 15
#define rst 0
#define dio0 27
#define en 32
/* LORA END */

unsigned long last_time = 0;
int i = 0;

void setup()
{
  Serial.begin(115200);

  /* WIFI START */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* WIFI END */

  /* FIREBASE START */
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY; // Assign RTDB API Key

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;                 // Assign rtdb url
  config.token_status_callback = tokenStatusCallback; // Set callback

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  /* FIREBASE END */

  /* LORA START */
  // Initiate the LoRa Enable pin
  pinMode(en, OUTPUT);
  // LoRa chip is Active High
  digitalWrite(en, HIGH);

  // initialize Serial Monitor
  Serial.println("LoRa Receiver");

  // setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  while (!LoRa.begin(915E6))
  {
    Serial.println(".");
    delay(500);
  }

  // ranges from 0-0xFF (We Use 0xF)
  LoRa.setSyncWord(0xF);
  Serial.println("LoRa Initializing OK!");
  /* LORA END */
}

void loop()
{
  unsigned long current_time = millis();
  int ID = 0;
  float temperature_value = 0, ph_value = 0;

  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    String LoRaData = "";
    String name_id = "";
    // received a packet
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available())
    {
      LoRaData = LoRa.readString();
      Serial.println(LoRaData);
      receive_parse_to_device(LoRaData, ID, temperature_value, ph_value);

      device[ID].name = String("KOLAM " + String(ID));
      device[ID].id = String("id" + String(ID));
      device[ID].ph = ph_value;
      device[ID].temperature = temperature_value;
      if (device[ID].isEmpty == true)
        device[ID].isEmpty = false;

      device[ID].name_root = String(device_root + device[ID].id + '/' + "name");
      device[ID].temperature_root = String(device_root + device[ID].id + '/' + "temperature");
      device[ID].ph_root = String(device_root + device[ID].id + '/' + "ph");
      // temperature_root_to_firebase = String(device_root + device[ID].id + '/' + "temperature");
      // ph_root_to_firebase = String(device_root + device[ID].id + '/' + "ph");
      // name_root_to_firebase = String(device_root + device[ID].id + '/' + "name");

      Serial.println(device[ID].name_root);
      Serial.println(device[ID].temperature_root);
      Serial.println(device[ID].ph_root);

      Serial.println("---------");
    }

    // print RSSI of packet
    Serial.print(" with RSSI ");
    Serial.println(LoRa.packetRssi());
    Serial.println("=======================");
  }
  if (current_time - last_time > 300000)
  {
    last_time = current_time;
    for (size_t i = 0; i < DEVICE_TOTAL; i++)
    {
      if (device[i].id != "id0" && !device[i].isEmpty)
      {
        firebaseSetStringAsync(device[i].name_root, device[i].name);
        firebaseSetFloatAsync(device[i].ph_root, device[i].ph);
        firebaseSetFloatAsync(device[i].temperature_root, device[i].temperature);
        Serial.println(">>>>>>>>>>>>>>>>>>>>>>>");
      }
    }
  }
}

void firebaseSetFloatAsync(String databaseDirectory, float value)
{
  // Write an Int number on the database path test/int
  if (Firebase.RTDB.setFloatAsync(&fbdo, databaseDirectory, value))
  {
    Serial.print("PASSED: ");
    Serial.println(value);
  }
  else
  {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void firebaseSetStringAsync(String databaseDirectory, String value)
{
  // Write a string on the database path
  if (Firebase.RTDB.setStringAsync(&fbdo, databaseDirectory, value))
  {
    Serial.print("PASSED: ");
    Serial.println(value);
  }
  else
  {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void receive_parse_to_device(const String &data_received, int &id, float &temperature, float &ph)
{
  int first_hashtag = data_received.indexOf('#');
  int second_hashtag = data_received.lastIndexOf('#');

  String id_string = data_received.substring(0, first_hashtag);
  // id = data_received.substring(0, first_hashtag);
  String temperature_string = data_received.substring(first_hashtag + 1, second_hashtag);
  String ph_string = data_received.substring(second_hashtag + 1);

  id = id_string.toInt();
  temperature = temperature_string.toFloat();
  ph = ph_string.toFloat();
}
