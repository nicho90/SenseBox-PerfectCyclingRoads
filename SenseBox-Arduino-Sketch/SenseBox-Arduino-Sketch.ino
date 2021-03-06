#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "Barometer.h"
#include "I2Cdev.h"
#include "MPU9150.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

const int LED = 7;
const int chipSelect = 4;
const int soundSensor = A0;

//Light sensor variables
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
uint16_t ir, full;

//Barometer variables
float pressure;
float atm;
Barometer myBarometer;

//Accelerometer variables
MPU9150 accelgyro;
I2Cdev   I2C_M;

TinyGPS gps;

File dataFile;
String dataRow;
String seperator = ",";
String dattim = "noval";
int rowCounter = 1;

SoftwareSerial ss(2, 3);

//Button variables
const int buttonPin = 8;
int buttonState = 0;
int lastButtonState = 0;
int isOn;

/*
  GPS to 3, 2, 5V and GND.
  Sound to A0.
  Gyro, Light and Barometer to I2C.
  Button to 8.
  LED to 7.
*/

void setup() {
  Serial.begin(115200);
  Wire.begin();
  //Initialize Accelerometer
  accelgyro.initialize();
  TWBR = 12; // set 400kHz mode @ 16MHz CPU or 200kHz mode @ 8MHz CPU

  /**
  *  Most serial messages are commented out to save memory.
  *  To debug the program, enable them again. Use F("string")
  *  to store strings in flash memory. Around 390 bytes of
  *  dynamic memory have to be available after compilation,
  *  otherwise the SD card won't work.
  **/

  //Serial.println(accelgyro.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

  delay(1000);

  // Initialize Light Sensor
  tsl.begin();
  tsl.setGain(TSL2591_GAIN_HIGH);     // 428x gain (will only work at low luminosity)
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time

  //Init barometer
  myBarometer.init();

  // Initalize SD card:
  //Serial.print("Initialize SD card...");

  if (!SD.begin(chipSelect)) {
    //Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  //Serial.println(F("card initialized."));

  //Pins:
  pinMode(LED, OUTPUT);
  pinMode(soundSensor, INPUT);
  pinMode(buttonPin, INPUT);

  dataFile = SD.open("test.csv", FILE_WRITE);

  // If the file opened okay, write to it:
  if (dataFile) {
    //Serial.print("Writing header test.csv...");
    dataFile.println("row,lat,lon,speed,timestamp,sound,luminosity,lux,ir,vibration,altitude");
    // Close the file:
    dataFile.close();
    //Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println(F("Error opening test.csv"));
  }
}

void loop() {
  buttonState = digitalRead(buttonPin);

  //Serial.println(buttonState);
  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      //buttonPushCounter++;
      //Serial.println(F("ON"));
      isOn = 1 - isOn;
      //Serial.print(buttonPushCounter);

    } else {
    }
    delay(50);
  }
  lastButtonState = buttonState;

  // LED is OFF if data is being recorded.
  // This is to avoid light from the LED influencing the light sensor.
  if (isOn == 1) {
    digitalWrite(LED, LOW);
    bool newData = false;

    ss.begin(9600);

    //Serial.println("starting gps reading");

    // Parse GPS data for one second and report some key values
    //Serial.println("Searching GPS...");
    for (unsigned long start = millis(); millis() - start < 500;)
    {
      while (ss.available())
      {
        if (gps.encode(ss.read())) {
          newData = true;
          //Serial.println(F("Data found! Proceeding..."));
        }
      }
    }

    ss.end();

    if (newData)
    {
      // IF NEW GPS-DATA
      //Serial.println(F("GPS data found!"));

      float flat, flon;
      unsigned long age, date, time;
      gps.f_get_position(&flat, &flon, &age);
      //Serial.println("");
      //Serial.print("LAT = ");
      flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6;
      //Serial.print("LON = ");
      flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6;
      //Speed
      gps.f_speed_kmph() == TinyGPS::GPS_INVALID_F_SPEED ? 0 : gps.f_speed_kmph();

      // Serial.println("DATE & TIME:");

      // Print date
      int year;
      byte month, day, hour, minute, second, hundredths;
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
      if (age == TinyGPS::GPS_INVALID_AGE)
        Serial.print(F("********** ******** "));
      else
      {
        char sz[24];
        sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d",
                month, day, year, hour, minute, second);
        //Serial.print(sz);
        dattim = String(sz);
        //Serial.print(dattim);
      }

      // GPS reading done
      newData = false;

      //Light sensor
      uint32_t lum = tsl.getFullLuminosity();
      ir = lum >> 16;
      full = lum & 0xFFFF;

      //Save collected data to sd card
      dataFile = SD.open("test.csv", FILE_WRITE);

      if (dataFile) {
        // Serial.print("Writing to test.txt...");
        dataFile.print(rowCounter);
        dataFile.print(seperator);
        dataFile.print(flat, 6);
        dataFile.print(seperator);
        dataFile.print(flon, 6);
        dataFile.print(seperator);
        dataFile.print(gps.f_speed_kmph(), 2);
        dataFile.print(seperator);
        dataFile.print(dattim);
        dataFile.print(seperator);
        dataFile.print(analogRead(soundSensor));
        dataFile.print(seperator);
        dataFile.print(tsl.getLuminosity(TSL2591_VISIBLE));
        dataFile.print(seperator);
        dataFile.print(tsl.calculateLux(full, ir));
        dataFile.print(seperator);
        dataFile.print(ir);
        dataFile.print(seperator);
        dataFile.print(((double) accelgyro.getAccelerationZ() / 16384), 4);
        dataFile.print(seperator);
        dataFile.println(myBarometer.calcAltitude(myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP())));

        //Serial.println(accelgyro.getAccelerationZ());
        // close the file:
        dataFile.close();

        rowCounter = rowCounter + 1;
        Serial.println(F("done writing."));
      } else {
        // if the file didn't open, print an error:
        //Serial.println(F("error opening file"));
      }

    }
  } else {
    digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
    //Serial.println(F("OFF"));
  }

}

