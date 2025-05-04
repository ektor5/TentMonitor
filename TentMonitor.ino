// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * TentMonitor
 *
 * Copyright 2024 Ettore Chimenti <ek5.chimenti@gmail.com>
 *
 */

#include <AdafruitIO.h>
#include <AdafruitIO_Dashboard.h>
#include <AdafruitIO_Data.h>
#include <AdafruitIO_Definitions.h>
#include <AdafruitIO_Feed.h>
#include <AdafruitIO_Group.h>
#include <AdafruitIO_MQTT.h>
#include <AdafruitIO_Time.h>
#include <AdafruitIO_WiFi.h>
#include <Wire.h>

#include "SparkFun_SCD4x_Arduino_Library.h" // library: http://librarymanager/All#SparkFun_SCD4x

#include "TentMonitor.h"

#define SCL_PIN 21
#define SDA_PIN 18
#define BLUE_LED 32
#define ORANGE_LED 33

// track time of last published messages and limit feed->save events to once
// every IO_LOOP_DELAY milliseconds
#define IO_LOOP_DELAY 10000
#define MAX_COUNT 30

TwoWire WireKey = TwoWire(2);

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

AdafruitIO_Feed *feed_temp = io.feed("tent-temperature");
AdafruitIO_Feed *feed_humidity = io.feed("tent-humidity");
AdafruitIO_Feed *feed_co2 = io.feed("tent-co2");

SCD4x mySensor;

void setup()
{
  Serial.begin(115200);
  Serial.println(F("SCD4x Example"));
  pinMode(BLUE_LED, OUTPUT);
  pinMode(ORANGE_LED, OUTPUT);
  digitalWrite(BLUE_LED, LOW);

  WireKey.begin(SDA_PIN, SCL_PIN); // In this example, let's use Wire1 instead of Wire

  //mySensor.enableDebugging(); // Uncomment this line to get helpful debug messages on Serial

  //mySensor.enableDebugging(Serial1); // Uncomment this line instead to get helpful debug messages on Serial1

  Serial.print("Connecting to Adafruit IO");

  // ADAFRUIT.IO
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    digitalWrite(BLUE_LED, HIGH);
    delay(500);
    digitalWrite(BLUE_LED, LOW);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());


  if (mySensor.begin(WireKey) == false) // .begin the sensor on Wire1 instead of Wire
  {
    Serial.println(F("Sensor not detected. Please check wiring. Freezing..."));
    digitalWrite(ORANGE_LED, HIGH);
    while (1)
      delay(1000);
  } else {
    digitalWrite(ORANGE_LED, LOW);
  }

  //We need to stop periodic measurements before we can change the sensor signal compensation settings
  if (mySensor.stopPeriodicMeasurement() == true)
  {
    Serial.println(F("Periodic measurement is disabled!"));
  }

  Serial.print(F("Temperature offset is currently: "));
  Serial.println(mySensor.getTemperatureOffset(), 2); // Print the temperature offset with two decimal places
  mySensor.setTemperatureOffset(4); // Set the temperature offset to 5C
  Serial.print(F("Temperature offset is now: "));
  Serial.println(mySensor.getTemperatureOffset(), 2); // Print the temperature offset with two decimal places

  Serial.print(F("Sensor altitude is currently: "));
  Serial.println(mySensor.getSensorAltitude()); // Print the sensor altitude
  mySensor.setSensorAltitude(110); // Set the sensor altitude to 110m
  Serial.print(F("Sensor altitude is now: "));
  Serial.println(mySensor.getSensorAltitude()); // Print the sensor altitude

  /* There is no getAmbientPressure command
  bool success = mySensor.setAmbientPressure(98700); // Set the ambient pressure to 98700 Pascals
  if (success)
  {
    Serial.println(F("setAmbientPressure was successful"));
  }
  */

  /* Finally, we need to restart periodic measurements */
  if (mySensor.startPeriodicMeasurement() == true)
  {
    Serial.println(F("Periodic measurements restarted!"));
  }

  lastUpdate = millis();
}

void loop()
{
  io.run();

  /* readMeasurement will return true when fresh data is available */
  if (mySensor.readMeasurement() &&
      millis() > (lastUpdate + IO_LOOP_DELAY))
  {
    uint16_t co2 = mySensor.getCO2();
    float temp = mySensor.getTemperature();
    float humi = mySensor.getHumidity();

    Serial.println();

    Serial.print(F("CO2(ppm):"));
    Serial.print(co2);

    Serial.print(F("\tTemperature(C):"));
    Serial.print(temp, 1);

    Serial.print(F("\tHumidity(%RH):"));
    Serial.print(humi, 1);
    Serial.println();

    Serial.println("Sending data");
    feed_co2->save(co2);
    feed_temp->save(temp);
    feed_humidity->save(humi);
    lastUpdate = millis();
    count = 0;

    digitalWrite(BLUE_LED, HIGH);
    delay(300);
    digitalWrite(BLUE_LED, LOW);

  } else {
    count++;
  }

  if (!(count % 5))
  {
    /* heartbeat */
    digitalWrite(BLUE_LED, HIGH);
    delay(10);
    digitalWrite(BLUE_LED, LOW);
  }

  Serial.print(F("."));
  delay(1000);

  if (count > MAX_COUNT) {
    digitalWrite(ORANGE_LED, HIGH);
    exit(0);
  }
}
