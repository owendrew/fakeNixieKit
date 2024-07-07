//====================================================================================
//                1.14" IPS LCD 4 DIGIT FAKE NIXIE CLOCK HHMM
//                
//====================================================================================

//BOARD: ESP32 WROOM 32D (38 Pin version)
//PARTITION SIZE: Minimal SPIFFS (Max App size = 1.9MB - This will allow all digits to 
//be stored internally

//====================================================================================
//                                  Definitions
//====================================================================================

#define digit1 21  // 1xxx Digit Enable Active Low
#define digit2 22  // x2xx Digit Enable Active Low
#define digit3 16  // xx3x Digit Enable Active Low
#define digit4 17  // xxx4 Digit Enable Active Low

#define PIN_HIGH true
#define PIN_LOW false

//====================================================================================
//                                  Digit data includes
//====================================================================================

#include "nixieFont.h"  // All the digit image data is stored here

//====================================================================================
//                                  Libraries
//====================================================================================

#include <SPI.h>              //required for TFT_eSPI (Displays)
#include "TFT_eSPI.h"         //Hardware-specific library for ESP32 - Don't forget to amend the config.txt file to suit your Microcontroller device and displays
#include "nixieClockConfig.h" //The file with your WIFI credentials
#include <time.h>             //time library functions
#include <WiFi.h>             //WiFi library

//====================================================================================
//                                    Variables and Constants
//====================================================================================

const char* ssid = SECRET_SSID;         //Home Wifi credentials from nixieClockConfig.h
const char* password = SECRET_PASS;     //Home Wifi credentials from nixieClockConfig.h
const char* dstRule = DST_RULE;         //daylight saving code from nixieClockConfig.h

int minutesUnits, minutesTens, hoursUnits, hoursTens, nmins, nhour;

//When these variables are set to 99 the corresponding digit is forced to refresh
int oldMinuteUnits = 99, oldMinutesTens = 99, oldHoursUnits = 99, oldHoursTens = 99;

time_t currentTime;

unsigned long currentMillis;
unsigned long lastUpdateTime;
unsigned long updateInterval = 30000;  // this equates to 30 secs

// Digit are defined in "nixieFont.h"
static const unsigned short* nixieNumeral[] = { Nx0, Nx1, Nx2, Nx3, Nx4, Nx5, Nx6, Nx7, Nx8, Nx9 };

//====================================================================================
//                                    Initialise Functions
//====================================================================================
WiFiServer server(80);
TFT_eSPI tft = TFT_eSPI();

//====================================================================================
//====================================================================================
//                                    Setup
//====================================================================================
//====================================================================================

void setup() {

  Serial.begin(115200);  // Used for debug messages
  delay(100);

  Serial.println(F("Fake Nixie Clock"));

  pinMode(digit1, OUTPUT);
  pinMode(digit2, OUTPUT);
  pinMode(digit3, OUTPUT);
  pinMode(digit4, OUTPUT);

  // Now initialise the TFT
  enableDisplays();
  tft.begin();
  tft.setRotation(0);         // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLACK);  // Clear the screens
  tft.setSwapBytes(true);     // Requred for the 1.14" IPS SPI (8pin) LCD displays from Aliexpress - If your colors are messed up try changing this to false
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(1);
  tft.println(F("Connecting to "));
  tft.setTextFont(2);
  tft.println(ssid);

  WiFi.setHostname("NIXIE_CLOCK");  // We start by connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(F("."));
  }
  tft.println("");
  tft.println(F("WiFi connected."));
  tft.println(F("IP address: "));
  tft.println(WiFi.localIP());

  disableDisplays();

  initTime(dstRule);  // Set timezone and daylight saving for London

  lastUpdateTime = millis();
}

//====================================================================================
//====================================================================================
//                                    Loop
//====================================================================================
//====================================================================================

void loop() {

  currentMillis = millis();

  // Check to see if and update from the time server is required
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;
    // Fetch and update the time
    initTime(dstRule);  // Adjust the timezone as needed
  }

  timeVariablesSet();  //  This function sets the nsecs, nmins, nhour variables from system time

  minutesUnits = nmins % 10;          // extract the unit digit of the number of minutes from nmins
  minutesTens = (nmins / 10) % 10;    // extract the tens digit of the number of minutes from nmins
  hoursUnits = nhour % 10;            // extract the unit digit of the number of hours from nhour
  hoursTens = (nhour / 10) % 10;      // extract the tens digit of the number of hours from nhour

  if (oldMinuteUnits != minutesUnits) {
    loadDigit(digit4, minutesUnits);  //Load Digit to display
    oldMinuteUnits = minutesUnits;
  }

  if (oldMinutesTens != minutesTens) {
    loadDigit(digit3, minutesTens);  //Load Digit to display
    oldMinutesTens = minutesTens;
  }

  if (oldHoursUnits != hoursUnits) {
    loadDigit(digit2, hoursUnits);  //Load Digit to display
    oldHoursUnits = hoursUnits;
  }

  if (oldHoursTens != hoursTens) {
    loadDigit(digit1, hoursTens);  //Load Digit to display
    oldHoursTens = hoursTens;
  }
  
}

//====================================================================================
//                                 initTime
//====================================================================================

void initTime(String timezone) {
 
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");  // First connect to NTP server, with 0 TZ offset
  if (!getLocalTime(&timeinfo)) {
    Serial.println("initTime: Failed to obtain time");
    return;
  }
  setTimezone(timezone);

}

//====================================================================================
//                                 timeVariableSet
//====================================================================================

void timeVariablesSet() {

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("timeVariableSet: Failed to obtain time");
    return;
  }

  Serial.println(&timeinfo, "timeVariableSet: %A, %B %d %Y %H:%M:%S zone %Z %z ");

  nmins = (timeinfo.tm_min);
  nhour = (timeinfo.tm_hour);

}

//====================================================================================
//                                 Set Time Zone
//====================================================================================

void setTimezone(String timezone) {

  Serial.printf(" setTimeZone: Setting Timezone to %s\n", timezone.c_str());
  setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();

}

//====================================================================================
//                                 Display digit on LCD x
//====================================================================================

void loadDigit(int displayPin, int numeral) {

  digitalWrite(displayPin, PIN_LOW);                      // Enable the display to be updated
  tft.pushImage(0, 0, 135, 240, nixieNumeral[numeral]);   // display correct image
  digitalWrite(displayPin, PIN_HIGH);                     // Disable the display after update

}

//====================================================================================
//                                 Enable all displays
//====================================================================================

void enableDisplays() {

  digitalWrite(digit1, PIN_LOW);
  digitalWrite(digit2, PIN_LOW);
  digitalWrite(digit3, PIN_LOW);
  digitalWrite(digit4, PIN_LOW);

}

//====================================================================================
//                                 Disable all displays
//====================================================================================

void disableDisplays() {

  digitalWrite(digit1, PIN_HIGH);  //hoursTens
  digitalWrite(digit2, PIN_HIGH);  //hoursUnits
  digitalWrite(digit3, PIN_HIGH);  //hoursTens
  digitalWrite(digit4, PIN_HIGH);  //hoursUnits

}
