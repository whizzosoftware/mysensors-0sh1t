/**
 * DESCRIPTION
 *
 * The 0SH1T sensor is a multi-sensor that monitors multiple
 * sensors to determine if a "tripped" condition has occurred.
 * This includes water leak and natural gas detection. It will 
 * additionally report temperature and humidity.
 *
 * Author: Dan Noguerol
 *
 * By default, connect a DHT (e.g. DHT22) sensor to digital 
 * pin 7, a water leak sensor to analog pin 0 and a gas sensor 
 * (e.g. MQ5) to analog pin 1.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

// uncomment this to monitor a water leak sensor
#define WATER_SENSOR
// uncomment this to report temperature and humidity
#define DHT_SENSOR
// uncomment this to monitor a gas sensor
#define GAS_SENSOR
// uncomment this to activate a buzzer when the sensor is triggered
#define BUZZER

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <MySensors.h>
#include "DHT.h"

#define SKETCH_NAME "0SH1T Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

#define PRIMARY_SENSOR_ID 1
// the number of milliseconds to wait in between sensor readings
#define POLLING_INTERVAL_MS 1000
// the number of polling intervals to force sending value updates even if sensor data hasn't changed (this doubles as a heartbeat)
#define IDLE_THRESHOLD 60
// the maximum allowable water leak sensor value before 0SH1T is considered "tripped"
#define WATER_THRESHOLD 50
// the minimum allowable gas sensor value before 0SH1T is considered "tripped"
#define GAS_THRESHOLD   15

#define WATER_LEAK_PIN A0
#define GAS_SENSOR_PIN A1
#define DHT_PIN 7
#define BUZZER_PIN 5
#define DHT_TYPE DHT22

MyMessage msgTripped(PRIMARY_SENSOR_ID, V_TRIPPED);
bool metric = true;

#ifdef DHT_SENSOR
  DHT dht(DHT_PIN, DHT_TYPE);
  MyMessage msgTemp(PRIMARY_SENSOR_ID, V_TEMP);
  MyMessage msgHum(PRIMARY_SENSOR_ID, V_HUM);
#endif

void setup()
{
  #ifdef WATER_SENSOR
    pinMode(WATER_LEAK_PIN, INPUT);
  #endif
  #ifdef GAS_SENSOR
    pinMode(GAS_SENSOR_PIN, INPUT);
  #endif
  #ifdef DHT_SENSOR
    dht.begin();
  #endif
  #ifdef BUZZER
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
  #endif
}

void presentation()
{
  sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER "." SKETCH_MINOR_VER);
  present(PRIMARY_SENSOR_ID, S_CUSTOM, "0SH1T Sensor", false);
  metric = getConfig().isMetric; 
}

// Loop will iterate on changes on the BUTTON_PINs
void loop()
{
  uint8_t trippedValue = 0;
  static uint8_t sentTrippedValue = -1;

  uint8_t tempValue;
  static uint8_t sentTempValue = -1;

  uint8_t humValue;
  static uint8_t sentHumValue = -1;
  
  static uint8_t idleThreshold;

  #ifdef WATER_SENSOR
    uint8_t leakValue;
    leakValue = analogRead(WATER_LEAK_PIN);
    trippedValue = max(trippedValue, leakValue > WATER_THRESHOLD);
    #ifdef MY_DEBUG
      Serial.print("Leak: ");
      Serial.print(leakValue);
    #endif
  #endif

  #ifdef DHT_SENSOR
    // send temp
    tempValue = (int)dht.readTemperature();
    if (!metric) {
      tempValue = (tempValue * 18 + 325) / 10; // convert to fahrenheit
    }
    if ((tempValue != sentTempValue) || idleThreshold >= 60) {
      send(msgTemp.set(tempValue, 0));
      sentTempValue = tempValue;
    }
    #ifdef MY_DEBUG
      Serial.print(", Temp: ");
      Serial.print(tempValue);
    #endif

    // send humidity
    humValue = (int)dht.readHumidity();
    if ((humValue != sentHumValue) || idleThreshold >= 60) {
      send(msgHum.set(humValue, 0));
      sentHumValue = humValue;
    }
    #ifdef MY_DEBUG
      Serial.print(", Hum: ");
      Serial.print(humValue);
    #endif
  #endif

  #ifdef GAS_SENSOR
    float sensor_volt, RS_air, sensorValue;
    int R0;
    for (int x=0; x < 500; x++) {
      sensorValue = sensorValue + analogRead(GAS_SENSOR_PIN);
    }
    sensor_volt = (sensorValue / 500.0) * (5.0 / 1023.0);
    RS_air = ((5.0 * 10.0) / sensor_volt) - 10.0;
    R0 = (int)(RS_air / 4.4);
    trippedValue = max(trippedValue, R0 < GAS_THRESHOLD);
    #ifdef MY_DEBUG
      Serial.print(", R0: ");
      Serial.print(R0);
    #endif
  #endif

  // send tripped value
  if ((trippedValue != sentTrippedValue) || idleThreshold >= 60) {
    send(msgTripped.set(trippedValue));
    sentTrippedValue = trippedValue;
  }
  #ifdef MY_DEBUG
    Serial.print(", Tripped: ");
    Serial.print(trippedValue);
  #endif

  #ifdef BUZZER
    if (sentTrippedValue > 0) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }
  #endif

  idleThreshold++;

  if (idleThreshold > 60) {
    idleThreshold = 0;
  }

  #ifdef MY_DEBUG
    Serial.println();
  #endif

  sleep(POLLING_INTERVAL_MS);
}
