#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"
#include <WiFi.h>
#include "ThingSpeak.h"
#include "average.h"
#include "serproc.h"
#include "stdio.h"
#include <iostream>

#include "Filters.h"
#include <AH/Hardware/FilteredAnalog.hpp>

#include <AH/Timing/MillisMicrosTimer.hpp>
#include <Filters/Butterworth.hpp>
#include "secrets.h"

// Function declarations
void wifiConnect(void);
float calcDcCurrent(float zeroVoltage, float voltage, float mPositive, float bPositive, float mNegative, float bNegative);

RTC_DS3231 rtc;
#define sda 21
#define scl 22
#define an1 34
#define rly1 18
#define rly2 4
#define an2 35



WiFiClient client;

unsigned long myChannelNumber = 1;


// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 60000;

// Battery measurements
float batVolt;
uint32_t lastAnalogRead = 0;
bool rly1State = false;
bool batLowReached = false;
uint32_t tstStartChg = 0;
uint32_t tstEndChg = 0;
uint32_t dischargeTime = 0;
Average avgBatVol;
#define MINBATVOLT 20.4

float batCur, batCurAn, batCur1;
Average avgBatCur;
bool start = false;
bool on = false;
bool batVoltReading=false;

// Process serial messages
ProcessSerial serProcess(Serial);

bool debug = false;
uint32_t prevLedMil = 0;
int ledState = LOW;

FilteredAnalog<12,       // Output precision in bits
               6,        // The amount of filtering
               uint32_t, // The integer type for the filter calculations
               analog_t  // The integer type for the upscaled analog values
               >
    analog = an1;

FilteredAnalog<12,       // Output precision in bits
               6,        // The amount of filtering
               uint32_t, // The integer type for the filter calculations
               analog_t  // The integer type for the upscaled analog values
               >
    analog2 = an2;

void setup()
{
  Serial.begin(115200);
  delay(20);

  Wire.begin(sda, scl);

  WiFi.mode(WIFI_STA);

  wifiConnect();

  ThingSpeak.begin(client); // Initialize ThingSpeak

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(rly1, OUTPUT);
  pinMode(rly2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(rly1, LOW);
  digitalWrite(rly2, LOW);

  analogReadResolution(12);

  DateTime now = rtc.now();
  Serial.printf("DATETIME: %03i/%02i/%02i %02i:%02i:%02i \n",
                now.year(), now.day(), now.month(), now.hour(), now.minute(), now.second());
  rly1State = false;

  // filtering
  FilteredAnalog<>::setupADC();
}

void loop()
{
  DateTime now = rtc.now();
  // put your main code here, to run repeatedly:
  // Serial.printf("DATETIME: %03i/%02i/%02i %02i:%02i:%02i \n",
  //               now.year(), now.day(), now.month(), now.hour(), now.minute(), now.second());

  static Timer<millis> timer = 10000; // ms
  static Timer<millis> timer2 = 1;    // ms

  if (start && (millis() - prevLedMil > 500))
  {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);

    prevLedMil = millis();
  }

  char *msg = serProcess.readSer();
  if (serProcess.newInput)
  {
    if (serProcess.isCommand(msg, "start"))
    {
      start = !start;
    }

    if (serProcess.isCommand(msg, "debug"))
    {
      debug = !debug;
    }

    if (serProcess.isCommand(msg, "rtc"))
    {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.printf("DATETIME: %03i/%02i/%02i %02i:%02i:%02i \n",
                    now.year(), now.day(), now.month(), now.hour(), now.minute(), now.second());
    }

    if (serProcess.isCommand(msg, "on"))
    {
      on = !on;
    }

    if (serProcess.isCommand(msg, "help"))
    {
      std::cout << "\nCommands:\n\n"
                   "\tdebugAll \t\tenable all debug messages\n"
                   "\tdebugOff  \t\tdisable all debug messages\n"
                   "\tdebugBattery \t\tenable battery related messages\n"
                   "\tdebugCharger \t\tenable charger related debug messages\n"
                   "\tdebugTimer \t\tenable timer related debug messages\n"
                   "\tdebugRtc \t\tenable RTC related debug messges\n"
                   "\tdebugSystem \t\tenable eeprom related messages\n"
                   "\tclearEeprom \t\twipe eeprom memory\n"
                   "\trestart \t\trestart system\n"
                   "\tdstEnable \t\tenable DST adjustment of system clock\n\n"
                   "\nSet values: set -[command] value \n\n"
                   "\tccc\t\tCharging current\n"
                   "\tccv\t\tConstant voltage \n"
                   "\tcfv\t\tFloating voltage\n"
                   "\tctc\t\tTaper current\n"
                   "\tccct\t\tConstant current timeout\n"
                   "\tccvt\t\tConstant voltage timeout\n"
                   "\tcfvt\t\tFloating charge timeout\n"
                   "\twpwd\t\tWifi password\n"
                   "\twssid\t\tWifi SSID\n";
    }
    serProcess.newInput = false;
  }
  // END Serial commands

  // float batVoltAn = (analogRead(an1) * 30.00) / 4096;
  // float batVoltAvg = avgBatVol.average(batVoltAn, 100);
  if (timer && analog.update())
  {
    float batVoltAn = (analog.getValue() * 30.00) / 4096;
    batVolt = batVoltAn;
  }

  // if (avgBatVol.readIndex == 0)
  // {
  //   batVolt = batVoltAvg;
  // }

  // batCurAn = analogRead(an2) * 4.5 / 4096;
  // //  batCur1 = calcDcCurrent(2.5, batCurAn, 0.077717034, 1.222817672, 0, 0);
  // batCur1 = calcDcCurrent(2.5, batCurAn, 12.641, 15.406, 0, 0);
  // float batCurAvg = avgBatCur.average(batCur1, 100);

  // if (avgBatCur.readIndex == 0)
  // {
  //   batCur = batCurAvg;
  // }
  if (timer2 && analog2.update())
  {
    batCurAn = analog2.getValue() * 4.5 / 4096;
    // batCur = calcDcCurrent(2.5, batCurAn, 12.641, 15.406, 0, 0);
    batCur = calcDcCurrent(2.5, batCurAn, 0.077717034, 1.222817672, 0, 0);
  }

  if (debug && (millis() - lastAnalogRead > 1000))
  {
    float directVolt = (analogRead(an1) * 30.00) / 4096;
    Serial.printf("batVolt=%.2f directVolt=%.2f batCurAn=%.2f batCur=%.2f batCur1=%.2f \n\n",
                  batVolt, directVolt, batCurAn, batCur, batCur1);

    Serial.printf("enableRelay=%i RelayState=%i batLowReached=%i \n\n",
                  start, rly1State, batLowReached);

    Serial.printf("WifiStatus=%i\n", WiFi.status() == WL_CONNECTED);

    Serial.println("---------------------------------------------------------------------------------");

    lastAnalogRead = millis();
  }

  if (start && !rly1State && (batVolt > (MINBATVOLT)))
  {
    rly1State = true;
    tstStartChg = now.unixtime();
  }

  if (!start || (rly1State && (batVolt < (MINBATVOLT))))

  {
    batLowReached = HIGH;
    tstEndChg = now.unixtime();
  }

  digitalWrite(rly1, rly1State);
  digitalWrite(rly2, rly1State);

  if ((millis() - lastTime) > timerDelay)
  {

    // Connect or reconnect to WiFi
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Wifi disconneted");
      wifiConnect();
    }

    if (rly1State)
    {
      dischargeTime = now.unixtime() - tstStartChg;
    }
    else
    {
      dischargeTime = 0;
    }

    if (start)
    {
      ThingSpeak.setField(1, String(batVolt));
      ThingSpeak.setField(2, String(dischargeTime));
      ThingSpeak.setField(3, String(batCur));
      ThingSpeak.setField(4, String(rly1State));
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

      if (x == 200)
      {
        Serial.println("Channel update successful.");
      }
      else
      {
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
    }
    lastTime = millis();
  }
  delayMicroseconds(10);
}

void wifiConnect(void)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Attempting to connect");
    while (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, password);
      delay(100);
    }
    Serial.println("\nConnected.");
  }
}

float calcDcCurrent(float zeroVoltage, float voltage, float mPositive, float bPositive, float mNegative, float bNegative)
{
  // if (voltage < zeroVoltage)
  // {
  //   return (voltage - bNegative) / mNegative; // x=(y-b)/m
  // }
  return (voltage - bPositive) / mPositive; // x=(y-b)/m
  // return mPositive * voltage - bPositive;
}
