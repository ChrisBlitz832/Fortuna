/*
  program: main.cpp
  usage: main program for the project Fortuna
  author: Christoph Blizenetz
  date: 01.05.2023 
*/

#include <iostream>
#include <string>

// include custom build libraries for this program
#include "Button.h"
#include "TimeManager.h"
#include "Clock.h"
#include "VariableChange.h"

// include libraries for the SPI e-paper panels (from Jean-Marc Zingg)
#include <GxEPD2_BW.h> 
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"

// include required fonts for the display
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold42pt7b.h>
#include <Fonts/customSymbols.h>

#include "DFRobot_CCS811.h" // CCS811-Sensor: air quality sensor
#include "DFRobot_DHT20.h" // DHT20-Sensor: temperature and humidity sensor

// pins for the I2C
#define SDA_PIN 21
#define SCL_PIN 22

// pins for the capacitive buttons
#define BUTTON_POWER 12
#define BUTTON_BRIGHTNESS 27
#define BUTTON_LUMINOUS 25
#define BUTTON_HIGHER 26
#define BUTTON_LOWER 14

#define ldr 2 // light dependent resistor
#define pir 35 // motion sensor

#define pwm0 15 // MOSFET for the cold white SMDs
#define pwm1 0 // MOSFET for the warm white SMDs

DFRobot_DHT20 DHT20;
DFRobot_CCS811 CCS811;

enum OperationMode {BRIGHTNESS, LUMINOUS, SETTING};
OperationMode operationMode = BRIGHTNESS;

enum DisplayModes {SETUP, OPERATION, LOWPOWER, AUTOMATIC, OFF_PREP, OFF};
DisplayModes displayMode = SETUP;

enum SettingMode {START_TIME, HOUR_0, HOUR_1, MINUTE_0, MINUTE_1, DATE_SETUP, DAY, MONTH, YEAR, END};
SettingMode settingMode = END;

TaskHandle_t Task1;
TaskHandle_t Task2;

TimeManager ldrPirTimeManager;
TimeManager buttonTimeManager;
TimeManager messageLoopTimeManager;
TimeManager measureDuration;
TimeManager validationDuration;
TimeManager displayRefresh;

Clock clockManager; // handling clock-time and date-time with backed RTC-Module

bool systemIsOn = true;
bool automaticIsOn = false;

int16_t tbx, tby; uint16_t tbw, tbh; // helpers for centering clock-time and date-time on display

Button btnPower(BUTTON_POWER, false);
Button btnBrightness(BUTTON_BRIGHTNESS, false);
Button btnLuminous(BUTTON_LUMINOUS, false);
Button btnHigher(BUTTON_HIGHER, false);
Button btnLower(BUTTON_LOWER, false);

float sumTemperature = 0;
float sumHumandity = 0;
float sumPPM = 0;
u_int16_t sumLDR = 0;

u_int16_t meanTemperature = 0;
u_int16_t meanHumandity = 0;
u_int16_t meanPPM = 0;
u_int16_t meanLDR = 0;

uint16_t brightnessSlider = 20;    
uint16_t luminousSlider = 30;

float automaticBrightness = 0;
float luminousValueCold = 0;
float luminousValueWarm = 0;
float brightnessValue = 0;

VariableChange<bool> systemStatus(systemIsOn);
VariableChange<uint16_t> brightness(brightnessSlider);
VariableChange<float> autoBrightness(automaticBrightness);
VariableChange<uint16_t> luminous(luminousSlider);
VariableChange<OperationMode> mode(operationMode);
VariableChange<char> clockMinute(clockManager.convertTimeToString()[4]);
VariableChange<char> clockHour(clockManager.convertTimeToString()[1]);

int hour0Counter;
int hour1Counter;
int minute0Counter;
int minute1Counter;

int dayCounter;
int monthCounter;
int yearCounter;

u_int16_t environmentalDataMeanCounter = 0;

uint16_t pirValue = 1;
uint16_t ldrCounter = 0;

std :: string averageAirQualityM = "Luftqualität in PPM: " + std::to_string(meanPPM);
std :: string badAirQualityM = "Lüften wird empfohlen: " + std::to_string(meanPPM);
int messageHandlerPosition = 0;

// https://rop.nl/truetype2gfx/

// handling pwm control for SMDs
void pwmAgent() {
  luminousValueCold = static_cast<float>(1-(luminousSlider/100.0));
  luminousValueWarm = static_cast<float>(luminousSlider/100.0);

  automaticBrightness = map(meanLDR, 0, 250, 100, 0);

  automaticBrightness < 6 ? automaticBrightness = 5 : automaticBrightness;

  if (automaticIsOn && pirValue == HIGH) {
    brightnessValue = automaticBrightness;
  }
  else if (automaticIsOn && pirValue == LOW) {
    brightnessValue = automaticBrightness / 2;
  }
  else if (!automaticIsOn && pirValue == HIGH) {
    brightnessValue = brightnessSlider;
  }
  else {
    brightnessValue = brightnessSlider / 2;
  }

  analogWrite(pwm0, (255.0/100.0)*brightnessValue*luminousValueCold); 
  analogWrite(pwm1, (255.0/100.0)*brightnessValue*luminousValueWarm); 
}

// setting basic parameters for the display on first run
void frameSetup() 
{
  display.init();
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);

  do
  {
    display.fillScreen(GxEPD_WHITE);
  } 
  while (display.nextPage());
}

// drawing progress bar for brightness and luminous
void drawProgressBar(int x, int y, float w, int h, int progress) {
  int progressWidth = progress * (w / 100);

  display.fillRoundRect(x, y, w, h, 20, GxEPD_BLACK);
  display.fillRoundRect(x+3, y+3, w-6, h-6, 20, GxEPD_WHITE);

  display.fillRoundRect(x+5, y+5, progressWidth-10, h-10, 20, GxEPD_BLACK);
}

// helper function to round automaticBrightness to certain base
int roundUp(int numToRound, int multiple) {
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

// handling buttons and their actions
void buttonAgent() {
  // turn display on and off (always avalible)
  if (btnPower.hasPressed() && settingMode == END) {
    systemIsOn = !systemIsOn;
  }
  // activate Clock-Setting mode
  else if (btnPower.isPressed() && btnLuminous.isPressed() && settingMode == END) {
    settingMode = START_TIME;
  }
  // activate/deactivate automatic-mode
  else if (btnBrightness.isPressed() && btnLuminous.isPressed() && settingMode == END) {
    if (automaticIsOn) {
      automaticIsOn = false;
      brightnessSlider = roundUp(automaticBrightness, 10);
      operationMode = BRIGHTNESS;
      displayMode = OPERATION;
    }
    else {
      automaticIsOn = true;
      operationMode = LUMINOUS;
      displayMode = AUTOMATIC;
    }

    pwmAgent();
  }
  // change operation-mode to brightness
  else if(btnBrightness.hasPressed() && settingMode == END && !automaticIsOn) {
    operationMode = BRIGHTNESS;
  } 
  // change operation-mode to luminous
  else if(btnLuminous.hasPressed() && settingMode == END) {
    operationMode = LUMINOUS;
  } 
  // set variable parameters (depending on operationMode) higher
  else if(btnHigher.hasPressed() && settingMode == END) {
    switch (operationMode)
    {
    case BRIGHTNESS:
      brightnessSlider < 100 ? brightnessSlider += 10 : brightnessSlider=brightnessSlider;
      break;
    
    case LUMINOUS:
      luminousSlider < 100 ? luminousSlider += 10 : luminousSlider=luminousSlider;
      break;

    default:
      break;
    }
  }
   // set variable parameters (depending on operationMode) lower
  else if(btnLower.hasPressed() && settingMode == END) {
    switch (operationMode)
    {
    case BRIGHTNESS:
      brightnessSlider > 0 ? brightnessSlider -= 10 : brightnessSlider=brightnessSlider;
      break;
    
    case LUMINOUS:
      luminousSlider > 0 ? luminousSlider -= 10 : luminousSlider=luminousSlider;
      break;
    
    default:
      break;
    }
  }
}

// drawing step bar for air-quality (ppm)
void drawStepBar(int x, int y, int w, int h, int progress) 
{
  uint16_t sectionWidth = w / 5 ;
  
  for (int i=0; i<5; i++) {
    display.fillRoundRect(x+((sectionWidth)*i), y, sectionWidth, h, 20, GxEPD_BLACK);
    display.fillRoundRect(x+3+((sectionWidth)*i), y+3, sectionWidth-6, h-6, 20, GxEPD_WHITE);

    if (i<progress)
    {
      display.fillRoundRect(x+5+(sectionWidth*i), y+5, sectionWidth-10, h-10, 20, GxEPD_BLACK);
    }
  }
}

// controlling progress of step bar for air-quality (range of ppm: 400-2500)
int calcPPWBar(int ppm, int min=400, int max=2500) {
  int oneElement = (max-min)/5;

  if (ppm < oneElement) {
    return 5;
  }
  else if (ppm >= oneElement && ppm < oneElement*2) {
    return 4;
  }
  else if (ppm >= oneElement*2 && ppm < oneElement*3) {
    return 3;
  }
  else if (ppm >= oneElement*3 && ppm < oneElement*4) {
    return 2;
  }
  else {
    return 1;
  }
}

// handling messages about air-quality
void messageHandler() {
  if (messageHandlerPosition == 0) {
    averageAirQualityM = "Luftqualität in PPM: " + std::to_string(meanPPM);
    display.print(averageAirQualityM.c_str());
  }
  else if (messageHandlerPosition == 1) {
    badAirQualityM = "Lüften wird empfohlen: " + std::to_string(meanPPM);
    display.print(badAirQualityM.c_str());
  }
}

// visualize static elements (none changing) on display
void displayStaticElements() {
  display.setPartialWindow(0, 0, 300, 400);
  display.fillRect(0, 120, 300, 5, GxEPD_BLACK);  // center 1
  display.fillRect(0, 0, 300, 5, GxEPD_BLACK);    // top
  display.fillRect(0, 0, 5, 400, GxEPD_BLACK);  // left
  display.fillRect(295, 0, 5, 400, GxEPD_BLACK);  // right
  display.fillRect(0, 395, 300, 5, GxEPD_BLACK);  // bottom       
  display.fillRect(0, 245, 300, 5, GxEPD_BLACK);  // center 2
  
  display.setFont(&customSymbols);
  display.setCursor(5, 180);
  display.print("0");

  display.setCursor(5, 240);
  display.print("0");
  display.setCursor(5, 240);
  display.print("1");

  display.setCursor(5, 305);
  display.print("2");

  display.setCursor(160, 305);
  display.print("3");

  display.setCursor(5, 365);
  display.print("4");
}

// partial refresh of clock time
void displayPartialRefreshClockTime() {
  display.fillRect(10, 75, 280, -65, GxEPD_WHITE);

  display.setFont(&FreeSansBold42pt7b);
  display.getTextBounds(clockManager.convertTimeToString().c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(((display.width() - tbw) / 2) - tbx, 70);
  display.print(clockManager.convertTimeToString().c_str());
}

// partial refresh of clock date
void displayPartialRefreshClockDate() {
  display.fillRect(10, 110, 280, -30, GxEPD_WHITE);

  display.setFont(&FreeSans18pt7b);
  display.getTextBounds(clockManager.convertDateToString().c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(((display.width() - tbw) / 2) - tbx, 105);
  display.print(clockManager.convertDateToString().c_str());
}

// partial refresh of brightness process bar
void displayPartialRefreshBrightnessProcessBar() {
  display.fillRect(70, 140, 215, 30, GxEPD_WHITE);
  drawProgressBar(70, 140, 215, operationMode == BRIGHTNESS ? 30 : 20, brightnessSlider);
}

// partial refresh of bighntess automatic process bar
void displayPartialRefreshBrightnessAutomaticProcessBar() {
  display.fillRect(70, 140, 215, 30, GxEPD_WHITE);
  drawProgressBar(70, 140, 215-40, 30, automaticBrightness);

  display.setFont(&FreeSans18pt7b);
  display.setCursor(256, 168);
  display.print('A');
}

// partial refresh of luminous process bar
void displayPartialRefreshLuminousProcessBar() {
  display.fillRect(70, 200, 215, 30, GxEPD_WHITE);
  drawProgressBar(70, 200, 215, operationMode == LUMINOUS ? 30 : 20, luminousSlider);
}

// partial refresh of temperature
void displayPartialRefreshTemperature() {
  display.fillRect(50, 295, 100, -35, GxEPD_WHITE);

  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(50, 290);
  display.print(meanTemperature);
  display.print("#");
}

// partial refresh of humandity
void displayPartialRefreshHumandity() {
  display.fillRect(220, 290, 70, -30, GxEPD_WHITE);

  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(220, 290);
  display.print(meanHumandity);
  display.print("%");
}

// partial refresh of ppm step bar
void displayPartialRefreshPPMdrawStepBar() {
  display.setFont(&customSymbols);
  display.setCursor(5, 365);
  drawStepBar(60, 322, 225, 30, calcPPWBar(meanPPM));
}

// partial refrsh of message handler
void displayPartialRefreshMessageHandler() {
  display.fillRect(10, 385, 280, -25, GxEPD_WHITE);

  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(10, 380);

  messageHandler();
}

// handler for all partrial refresh functions
void displayRefreshHandler(void (*func)()) {
  int count = 0;

  do 
  {
    func();
  }
  while(display.nextPage());
}

// collecting and validation of environmental data
void environmentalData() {
  // every 5sek (5000ms)
  if (measureDuration.elapsed()) {
    sumTemperature += DHT20.getTemperature();
    sumHumandity += DHT20.getHumidity()*100.0;
    sumPPM += CCS811.getCO2PPM();
    
    environmentalDataMeanCounter++;

    measureDuration.setPrevious();
  }

  // every 1min (60000ms)
  if (validationDuration.elapsed()) {
    meanTemperature = round(sumTemperature / environmentalDataMeanCounter);
    meanHumandity = round(sumHumandity / environmentalDataMeanCounter);
    meanPPM = round(sumPPM / environmentalDataMeanCounter);

    if (settingMode == END && displayMode == OPERATION) {
      displayRefreshHandler(&displayPartialRefreshTemperature);
      displayRefreshHandler(&displayPartialRefreshHumandity);
      displayRefreshHandler(&displayPartialRefreshPPMdrawStepBar);
    }

    if (meanPPM > 1500) {
      messageHandlerPosition = 1;
    }
    else {
      messageHandlerPosition = 0;
    }

    sumTemperature = 0;
    sumHumandity = 0;
    sumPPM = 0;
    environmentalDataMeanCounter = 0;
    validationDuration.setPrevious();
  }
}

template<typename T>
void settingModePlusMinus(T value, int xOffset, int yOffset, int rectWidth, int rectHeight, int yCursor, GFXfont font) {
  do 
  { 
    display.fillRect((((display.width() - tbw) / 2) - tbx) + xOffset, yOffset, rectWidth, rectHeight, GxEPD_WHITE);

    display.setFont(&font);
    display.setCursor((((display.width() - tbw) / 2) - tbx) + xOffset, yCursor);
    display.print(value);
  } 
  while (display.nextPage());
}

template<typename T>
void settingModeNextDigit(T value, int whiteRectOffset, int blackRectOffset, int yRect, int rectWidth, int rectHeight, int yCursor, GFXfont font) {
  do 
  {
    display.fillRect((((display.width() - tbw) / 2) - tbx) + whiteRectOffset, yRect, rectWidth + 5, rectHeight, GxEPD_WHITE); // rectHeight = -4
    display.fillRect((((display.width() - tbw) / 2) - tbx) + blackRectOffset, yRect, rectWidth, rectHeight, GxEPD_BLACK); // yRect = 80 rectWidth = 40

    display.setFont(&font);

    display.setCursor((((display.width() - tbw) / 2) - tbx) + blackRectOffset, yCursor); // yCursor = 70
    display.print(value);
  }
  while(display.nextPage());
}

std :: string monthSwitch(int monthNumber) {
  switch(monthNumber-1) {
    case 0:
      return "Jan";

    case 1:
      return "Feb";
    
    case 2:
      return "Mar";

    case 3:
      return "Apr";

    case 4:
      return "Mai";

    case 5:
      return "Jun";

    case 6:
      return "Jul";

    case 7:
      return "Aug";

    case 8:
      return "Sep";

    case 9: 
      return "Oct";

    case 10:
      return "Nov";

    case 11:
      return "Dec";

    default:
      return "Jan";
      break;
  }
}

void settingModeHandler() {
  const char* currentClockTime;
  char currentClockTimeArr[5];

  const char* currentDateTime;
  char currentDateTimeArr[10];

  switch (settingMode)
  {
  case START_TIME:

    display.clearScreen();

    currentClockTime = clockManager.convertTimeToString().c_str();
    strcpy(currentClockTimeArr, currentClockTime);

    hour0Counter = currentClockTimeArr[0] - 48;
    hour1Counter = currentClockTimeArr[1] - 48;
    minute0Counter = currentClockTimeArr[3] - 48;
    minute1Counter = currentClockTimeArr[4] - 48;

    display.firstPage();

    // 0 | 0 | : | 0 | 0
    // -5  40  88  120 165

    do 
    {
      display.fillRect(48, 80, 35, -4, GxEPD_BLACK);
      display.setFont(&FreeSansBold42pt7b);
      display.getTextBounds(currentClockTime, 0, 0, &tbx, &tby, &tbw, &tbh);
      display.setCursor((((display.width() - tbw) / 2) - tbx) - 5, 70);
      display.print(hour0Counter);

      display.setCursor((((display.width() - tbw) / 2) - tbx) + 40, 70);
      display.print(hour1Counter);

      display.setCursor((((display.width() - tbw) / 2) - tbx) + 80 + 8, 70);
      display.print(":");

      display.setCursor((((display.width() - tbw) / 2) - tbx) + 120, 70);
      display.print(minute0Counter);

      display.setCursor((((display.width() - tbw) / 2) - tbx) + 165, 70);
      display.print(minute1Counter);
    }
    while(display.nextPage());
 
    settingMode = HOUR_0;

    break;

  case HOUR_0:
    if (btnHigher.hasPressed()) {
      hour0Counter < 2 ? hour0Counter++ : hour0Counter = 0;
      settingModePlusMinus(hour0Counter, -5, 75, 45, -65, 70, FreeSansBold42pt7b);
    }
    else if (btnLower.hasPressed()) {
      hour0Counter > 0 ? hour0Counter-- : hour0Counter = 2;
      settingModePlusMinus(hour0Counter, -5, 75, 45, -65, 70, FreeSansBold42pt7b);
    }

    if (btnBrightness.hasPressed()) {
      settingModeNextDigit(hour1Counter, -5, 40, 80, 40, -4, 70, FreeSansBold42pt7b); // ÄNDERT
      settingMode = HOUR_1;
    }

    break;

  case HOUR_1:
    if (btnHigher.hasPressed()) {
      if (hour0Counter < 2) {
        hour1Counter < 9 ? hour1Counter++ : hour1Counter = 0;
      }
      else {
        hour1Counter < 4 ? hour1Counter++ : hour1Counter = 0;
      }
      
      settingModePlusMinus(hour1Counter, 40, 75, 45, -65, 70, FreeSansBold42pt7b);
    }
    else if (btnLower.hasPressed()) {

      if (hour0Counter < 2) {
        hour1Counter > 0 ? hour1Counter-- : hour1Counter = 9;
      }
      else {
        hour1Counter > 0 ? hour1Counter-- : hour1Counter = 4;
      }

      settingModePlusMinus(hour1Counter, 40, 75, 45, -65, 70, FreeSansBold42pt7b);
    }

    if (btnBrightness.hasPressed()) {
      settingModeNextDigit(minute0Counter, 40, 120, 80, 40, -4, 70, FreeSansBold42pt7b);
      settingMode = MINUTE_0;
    }

    break;

  case MINUTE_0:
    Serial.printf("MINUTE_0\n");

    if (btnHigher.hasPressed()) {
      minute0Counter < 5 ? minute0Counter++ : minute0Counter = 0;
      settingModePlusMinus(minute0Counter, 120, 75, 45, -65, 70, FreeSansBold42pt7b);
    }
    else if (btnLower.hasPressed()) {
      minute0Counter > 0 ? minute0Counter-- : minute0Counter = 5;
      settingModePlusMinus(minute0Counter, 120, 75, 45, -65, 70, FreeSansBold42pt7b);
    }

    if (btnBrightness.hasPressed()) {
      settingModeNextDigit(minute1Counter, 120, 165, 80, 40, -4, 70, FreeSansBold42pt7b);
      settingMode = MINUTE_1;
    }

    break;
  
  case MINUTE_1:
    if (btnHigher.hasPressed()) {
      minute1Counter < 9 ? minute1Counter++ : minute1Counter = 0;
      settingModePlusMinus(minute1Counter, 165, 75, 45, -65, 70, FreeSansBold42pt7b);
    }
    else if (btnLower.hasPressed()) {
      minute1Counter > 0 ? minute1Counter-- : minute1Counter = 9;
      settingModePlusMinus(minute1Counter, 165, 75, 45, -65, 70, FreeSansBold42pt7b);
    }

    Serial.printf("MINUTE_1\n");

    if (btnBrightness.hasPressed()) {
    
      settingMode = DATE_SETUP;
    }

    break;

  case DATE_SETUP:
    display.clearScreen();

    const char* currentDateTime;
    char currentDateTimeArr[10];

    currentDateTime = clockManager.convertDateToString().c_str();
    strcpy(currentDateTimeArr, currentDateTime);

    dayCounter = (currentDateTimeArr[0]-48) *10 + currentDateTimeArr[1]-48;
    monthCounter = (currentDateTimeArr[3]-48)*10 + currentDateTimeArr[4]-48;
    yearCounter = (currentDateTimeArr[6]-48)*1000 + (currentDateTimeArr[7]-48)*100 + (currentDateTimeArr[8]-48)*10 + currentDateTimeArr[9]-48;

    Serial.printf("%d / %d / %d", dayCounter, monthCounter, yearCounter);

     display.firstPage();

    // 0 | 0 | : | 0 | 0
    // -5  40  88  120 165

    do 
    {
      display.setFont(&FreeSansBold18pt7b);
      display.getTextBounds(currentDateTime, 0, 0, &tbx, &tby, &tbw, &tbh);

      display.fillRect((((display.width() - tbw) / 2) - tbx) - 10, 125, 38, -4, GxEPD_BLACK);

      display.setCursor((((display.width() - tbw) / 2) - tbx) - 10, 110);
      display.print(dayCounter);

      display.setCursor((((display.width() - tbw) / 2) - tbx) + 28, 110);
      display.print('.');

      display.setCursor((((display.width() - tbw) / 2) - tbx) + 40, 110);
      display.print(monthSwitch(monthCounter).c_str());

      display.setCursor((((display.width() - tbw) / 2) - tbx) + 110, 110);
      display.print(yearCounter);
    }
    while(display.nextPage());

    settingMode = DAY;

    break;

  case DAY:
    if (btnHigher.hasPressed()) {
      dayCounter < 31 ? dayCounter++ : dayCounter = 0;
      settingModePlusMinus(dayCounter, -10, 120, 38, -65, 110, FreeSansBold18pt7b);
    }
    else if (btnLower.hasPressed()) {
      dayCounter > 0 ? dayCounter-- : dayCounter = 31;
      settingModePlusMinus(dayCounter, -10, 120, 38, -65, 110, FreeSansBold18pt7b);
    }

    if (btnBrightness.hasPressed()) {
      settingModeNextDigit(monthSwitch(monthCounter).c_str(), -10, 40, 125, 60, -4, 110, FreeSansBold18pt7b);

      settingMode = MONTH;
    }

    break;


  case MONTH:
    if (btnHigher.hasPressed()) {
      monthCounter < 12 ? monthCounter++ : monthCounter = 0;
      settingModePlusMinus(monthSwitch(monthCounter).c_str(), 40, 120, 70, -65, 110, FreeSansBold18pt7b);
    }
    else if (btnLower.hasPressed()) {
      monthCounter > 0 ? monthCounter-- : monthCounter = 31;
      settingModePlusMinus(monthSwitch(monthCounter).c_str(), 40, 120, 70, -65, 110, FreeSansBold18pt7b);
    }

    if (btnBrightness.hasPressed()) {
      settingModeNextDigit(yearCounter, 40, 60+50, 125, 80, -4, 110, FreeSansBold18pt7b);
      settingMode = YEAR;
    }

    break;

  case YEAR:
    if (btnHigher.hasPressed()) {
      yearCounter++;
      settingModePlusMinus(yearCounter, 110, 120, 120, -65, 110, FreeSansBold18pt7b);
    }
    else if (btnLower.hasPressed()) {

      yearCounter < 2000 ? 2000 : yearCounter--; 
      settingModePlusMinus(yearCounter, 110, 120, 120, -65, 110, FreeSansBold18pt7b);
    }

    if (btnBrightness.hasPressed()) {
      yearCounter = yearCounter-2000;

      int yearDigit2 = yearCounter % 10;
      int yearDigit1 = (yearCounter-(yearCounter%10)) / 10;


      clockManager.setDateTime(yearDigit2*10+yearDigit1, monthCounter, dayCounter, hour0Counter*10 + hour1Counter, minute0Counter*10 + minute1Counter, 0);

      displayMode = SETUP;
      settingMode = END;
    }

    break;

  case END:
    
    break;

  default:
    break;
  }
}

void displayAgent() {
  switch (displayMode)
  {

  case SETUP:
    display.init();
    display.setRotation(3);
    display.setTextColor(GxEPD_BLACK);

    display.fillScreen(GxEPD_WHITE);
    display.firstPage();

    pwmAgent();

    do 
    {
      displayStaticElements();
      displayPartialRefreshClockTime();
      automaticIsOn ? displayPartialRefreshBrightnessAutomaticProcessBar() : displayPartialRefreshBrightnessProcessBar();
      displayPartialRefreshLuminousProcessBar();
      displayPartialRefreshTemperature();
      displayPartialRefreshHumandity();
      displayPartialRefreshPPMdrawStepBar();
      displayPartialRefreshMessageHandler();
      displayPartialRefreshClockDate();
    }
    while(display.nextPage());

    if (automaticIsOn) {
      displayMode = AUTOMATIC;
    }
    else {
      displayMode = OPERATION;
    }

    break;

  case OPERATION:

    if (messageLoopTimeManager.elapsed()) {
      
      if (settingMode == END) {
        if (clockMinute.hasChanged(clockManager.convertTimeToString()[4])) {
          displayRefreshHandler(&displayPartialRefreshClockTime);
        }
        else if (clockHour.hasChanged(clockManager.convertTimeToString()[1])) {
          systemIsOn ? systemIsOn : !systemIsOn;
          displayMode = OFF;
        }
        else if (mode.hasChanged(operationMode) && !automaticIsOn) {
          displayRefreshHandler(&displayPartialRefreshBrightnessProcessBar);
          displayRefreshHandler(&displayPartialRefreshLuminousProcessBar);
        }
        else if (brightness.hasChanged(brightnessSlider) && !automaticIsOn) {
          displayRefreshHandler(&displayPartialRefreshBrightnessProcessBar);
          pwmAgent();
        }
        else if (autoBrightness.hasChanged(automaticBrightness) && automaticIsOn) {
          displayRefreshHandler(&displayPartialRefreshBrightnessAutomaticProcessBar);
        }
        else if (luminous.hasChanged(luminousSlider)) {
          displayRefreshHandler(&displayPartialRefreshLuminousProcessBar);
          pwmAgent();
        }
        
        messageLoopTimeManager.setPrevious();
      }
    }

     if (displayRefresh.elapsed() && settingMode == END) {
        do 
        {
          displayStaticElements();
          displayPartialRefreshClockTime();
          displayPartialRefreshClockDate();
          automaticIsOn ? displayPartialRefreshBrightnessAutomaticProcessBar() : displayPartialRefreshBrightnessProcessBar();
          displayPartialRefreshLuminousProcessBar();
          displayPartialRefreshTemperature();
          displayPartialRefreshHumandity();
          displayPartialRefreshPPMdrawStepBar();
          displayPartialRefreshMessageHandler();
        }
        while(display.nextPage());
        
        displayRefresh.setPrevious();
      }

    if (!systemIsOn) {
      analogWrite(pwm0, 0);
      analogWrite(pwm1, 0);
      displayRefresh.setPrevious();
     
      display.fillScreen(GxEPD_WHITE);
      displayMode = OFF;
    }

    break;

  case LOWPOWER:

    if (digitalRead(pir)) {
      pwmAgent();
      displayMode = OPERATION;
    }

    break;

  case AUTOMATIC:
    displayRefreshHandler(&displayPartialRefreshBrightnessAutomaticProcessBar);
    displayRefreshHandler(&displayPartialRefreshLuminousProcessBar);
    displayMode = OPERATION;
    break;

  case OFF:
    if (systemIsOn) {
      displayRefresh.setPrevious();
      displayMode = SETUP;
    }
    
    display.firstPage();

    do
    {
      display.clearScreen();
    }
    while (display.nextPage());

    if (displayRefresh.elapsed()) {
      display.powerOff();
    }

    break;
  
  default:
    break;
  }
}

void Task1code(void * parameter) {
  for(;;) {
    if (settingMode == END) {
      displayAgent(); 
    }
    else {
      settingModeHandler();
    }
  }
}

void Task2code(void * parameter) {
  for(;;) {
   if (buttonTimeManager.elapsed()) {
      buttonAgent();

      buttonTimeManager.setPrevious();
    }

    if (ldrPirTimeManager.elapsed() && systemIsOn) {
      ldrCounter++;
      sumLDR += analogRead(ldr);

      if (ldrCounter >= 20) {
        meanLDR = sumLDR / ldrCounter;
        ldrCounter = 0;
        sumLDR = 0;
        
        if (automaticIsOn) {
          pwmAgent();
        }
      }

      pirValue = digitalRead(pir);

      ldrPirTimeManager.setPrevious();
    }
    
    if (systemIsOn) {
      btnBrightness.update();
      btnLuminous.update();
      btnHigher.update();
      btnLower.update();

      clockManager.update();

      environmentalData();
    }

    btnPower.update();
  }
}


void setup()
{
  Wire.end();
  Wire.begin();
  display.init();
  Serial.begin(9600);

  // TimeManager setup
  ldrPirTimeManager.setDuration(200);
  buttonTimeManager.setDuration(200);
  messageLoopTimeManager.setDuration(200);

  displayRefresh.setDuration(30000);

  measureDuration.setDuration(5000);
  validationDuration.setDuration(60000);

  meanTemperature = DHT20.getTemperature();
  meanHumandity = DHT20.getHumidity()*100.0;
  meanPPM = CCS811.getCO2PPM();

  // Button setup
  btnPower.begin();
  btnBrightness.begin();
  btnLuminous.begin();
  btnHigher.begin();
  btnLower.begin();

  // PinMode setup
  pinMode(ldr, INPUT);
  pinMode(pwm0, OUTPUT);
  pinMode(pwm1, OUTPUT);

  pinMode(pir, INPUT);

  analogWrite(pwm0, 0);
  analogWrite(pwm1, 0);

  systemStatus.begin();
  brightness.begin();
  luminous.begin();
  mode.begin();

  while(DHT20.begin()) {
    Serial.println("Initialize sensor failed");
    delay(2000);
  }

  while(CCS811.begin() != 0){
    Serial.println("failed to init chip, please check if the chip connection is fine");
    delay(2000);
  }

  clockManager.begin();
  //clockManager.setDateTime(23, 4, 6, 15, 10, 0);
  

  xTaskCreatePinnedToCore(
      Task1code, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &Task1,  /* Task handle. */
      0); /* Core where the task should run */

  xTaskCreatePinnedToCore(
      Task2code, /* Function to implement the task */
      "Task2", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      1,  /* Priority of the task */
      &Task2,  /* Task handle. */
      1); /* Core where the task should run */

  
  CCS811.writeBaseLine(28095);
}

void loop() { }