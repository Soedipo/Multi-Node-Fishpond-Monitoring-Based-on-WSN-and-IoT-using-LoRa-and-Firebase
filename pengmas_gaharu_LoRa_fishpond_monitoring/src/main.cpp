/***
    Lora WSN Node
  
  Created by Soediponegoro
  June 2023
***/

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/* LORA START */
// define the pins used by the transceiver module
#define ss 15
#define rst 0
#define dio0 27
#define en 32
/* LORA START */

/* TEMPERATURE DS18B20 START */
// SENSOR PIN
// Data wire is plugged into port 2 on the Arduino
#define TEMPERATURE 4
#define PH 36

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TEMPERATURE);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensor(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;
/* TEMPERATURE DS18B20 END */

int counter = 0;
const int id = 2;

float PH_READING(float x);

void setup()
{
  pinMode(PH, INPUT);

  // Initiate the LoRa Enable pin
  pinMode(en, OUTPUT);
  // LoRa chip is Active High
  digitalWrite(en, HIGH);

  // initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("LoRa Sender");

  // setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  while (!LoRa.begin(915E6))
  {
    Serial.println(".");
    delay(500);
  }

  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF);
  Serial.println("LoRa Initializing OK!");

  /* TEMPERATURE START */
  // locate devices on the bus
  Serial.print("Locating temperature sensor...");
  sensor.begin();

  if (!sensor.getAddress(insideThermometer, 0))
    Serial.println("Unable to find address for Device 0");
  delay(1000);
  sensor.setResolution(insideThermometer, 9);
  delay(1000);
  /* TEMPERATURE END */
}

void loop()
{
  // pH
  float ph_read = analogRead(PH);
  float ph = PH_READING(ph_read);

  // Degrees
  sensor.requestTemperatures();
  float temperature = sensor.getTempC(insideThermometer);

  // LoRa
  String packet_to_send = String(String(id) + '#' + String(temperature) + '#' + String(ph, 1));
  Serial.print(counter);
  Serial.print(". Sending packet: ");
  Serial.println(packet_to_send);

  // Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print(packet_to_send);
  LoRa.endPacket();

  counter++;
  delay(1000);
}

float PH_READING(float x)
{
  float volt_ph = (3.3 / 4095.0) * x + 0.18;
  // float ph = -14.37 * volt_ph + 36.0982; // 4,7 volt here
  float ph = -8.73 * volt_ph + 25.77; // 4,1 volt
  // float ph = -9.71 * volt_ph + 29.825; // 4,7 volt
  float roundNum = ph-floor(ph);
  if(roundNum <= 0.25)
    ph = floor(ph);
  else if(roundNum > 0.25 && roundNum <= 0.75)
    ph = floor(ph) + 0.5;
  else if (roundNum > 0.75)
    ph = ceil(ph);
  
  if (ph < 0)
    ph = 0;
  else if (ph > 14)
    ph = 14;

  return ph;
}
