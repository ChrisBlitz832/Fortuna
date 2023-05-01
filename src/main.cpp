// GxEPD2_HelloWorld.ino by Jean-Marc Zingg

// see GxEPD2_wiring_examples.h for wiring suggestions and examples
// if you use a different wiring, you need to adapt the constructor parameters!

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
//#include <GFX.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <iostream>
#include <string>
#include <Fonts/FreeSansBold42pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/customSymbols.h>

#include "DFRobot_CCS811.h" // air quality sensor
#include "DFRobot_DHT20.h" // temperature and humidity sensor

#define SDA_PIN 21
#define SCL_PIN 22

#define BUTTON_POWER 12
#define BUTTON_BRIGHTNESS 27
#define BUTTON_LUMINOUS 25
#define BUTTON_HIGHER 26
#define BUTTON_LOWER 14

#define ldrPin 2

#define pwm1 15
#define pwm2 0

#define pir 35

// DHT20 - Sensor:
DFRobot_DHT20 dht20;

// CCS811 - Sensor:
DFRobot_CCS811 CCS811;

// or select the display constructor line in one of the following files (old style):
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"

#include "Button.h"
#include "TimeManager.h"
#include "Clock.h"
#include "VariableChange.h"

enum OperationMode {BRIGHTNESS, LUMINOUS, SETTING};
OperationMode operationMode = BRIGHTNESS;

enum DisplayModes {SETUP, OPERATION, LOWPOWER, AUTOMATIC, OFF};
DisplayModes displayMode = SETUP;

TaskHandle_t Task1;
TaskHandle_t Task2;

TimeManager ldrTimeManager;
TimeManager timeManager2;
TimeManager buttonTimeManager;
TimeManager messageLoopTimeManager;

Clock clockManager;

bool systemIsOn = true;
bool automaticIsOn = false;

int hours = 0;
int minutes = 0;

bool displayIsActive = false;

char clockTime[] = "10:09";

char dateTime[] = "04.10.2004";
int16_t tbx, tby; uint16_t tbw, tbh;

Button btnPower(BUTTON_POWER, false);
Button btnBrightness(BUTTON_BRIGHTNESS, false);
Button btnLuminous(BUTTON_LUMINOUS, false);
Button btnHigher(BUTTON_HIGHER, false);
Button btnLower(BUTTON_LOWER, false);

// Display variables

// Temperature
bool onChange = false;

float sumTemperature = 0;
float sumHumandity = 0;
float sumPPM = 0;
int sumLDR = 0;

int ldrValue = 0;

int meanTemperature = 0;
int meanHumandity = 0;
int meanPPM = 0;
bool environmentalDataKey = true;
int counter = 0;

TimeManager measureDuration;
TimeManager validationDuration;

TimeManager displayRefresh;

uint16_t brightnessSlider = 20;    
uint16_t currentBrightnessSlider = 0;
uint16_t previousBrightnessSlider = 0;

uint16_t luminousSlider = 30;
uint16_t currentLuminousSlider = 0;
uint16_t previousLuminousSlider = 0;

float automaticBrightness = 0;

int pirValue = 1;
int pirCounter = 0;

int ldrCounter = 0;

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

// https://rop.nl/truetype2gfx/

enum SettingMode {START_TIME, HOUR_0, HOUR_1, MINUTE_0, MINUTE_1, DATE_SETUP, DAY, MONTH, YEAR, END};
SettingMode settingMode = END;
 

void pwmAgent() {

  float luminousValueCold = static_cast<float>(1-(luminousSlider/100.0));
  float luminousValueWarm = static_cast<float>(luminousSlider/100.0);
  automaticBrightness = map(ldrValue, 0, 250, 100, 0);

  automaticBrightness < 6 ? automaticBrightness = 5 : automaticBrightness;

  float brightnessValue;

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


  // Serial.printf("PWM\n");
  Serial.printf("LDR: %d\n", ldrValue);

  analogWrite(pwm1, (255.0/100.0)*brightnessValue*luminousValueCold); 
  analogWrite(pwm2, (255.0/100.0)*brightnessValue*luminousValueWarm); 
}

void frameSetup() 
{
  display.init();
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);

  do
  {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

void drawProgressBar(int x, int y, float w, int h, int progress)
{
  int progressWidth = progress * (w / 100);

  display.fillRoundRect(x, y, w, h, 20, GxEPD_BLACK);
  display.fillRoundRect(x+3, y+3, w-6, h-6, 20, GxEPD_WHITE);

  display.fillRoundRect(x+5, y+5, progressWidth-10, h-10, 20, GxEPD_BLACK);
}

int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

void buttonAgent() {
  // Turn Lamp on and off (always avalible)
  if (btnPower.hasPressed() && settingMode == END) {
    systemIsOn = !systemIsOn;
  }
  // activate Clock-Setting mode
  else if (btnPower.isPressed() && btnLuminous.isPressed() && settingMode == END) {
    settingMode = START_TIME;
  }
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
  else if(btnBrightness.hasPressed() && settingMode == END && !automaticIsOn) {
    operationMode = BRIGHTNESS;
  } 
  else if(btnLuminous.hasPressed() && settingMode == END) {
    operationMode = LUMINOUS;
  } 
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

void stepBar(int x, int y, int w, int h, int progress) 
{
  int sectionWidth = w / 5 ;
  
  // display.fillRect(x, y, w, h, GxEPD_BLACK);
  // display.fillRect(x+3, y+3, w-6, h-6, GxEPD_WHITE);

  // display.fillRect(x+sectionWidth/2, y, w-sectionWidth, 3, GxEPD_BLACK);
  // display.fillRect(x+sectionWidth/2, y+h-3, w-sectionWidth, 3, GxEPD_BLACK);

  for (int i=0; i<5; i++) {
    display.fillRoundRect(x+((sectionWidth)*i), y, sectionWidth, h, 20, GxEPD_BLACK);
    display.fillRoundRect(x+3+((sectionWidth)*i), y+3, sectionWidth-6, h-6, 20, GxEPD_WHITE);

    if (i<progress)
    {
      display.fillRoundRect(x+5+(sectionWidth*i), y+5, sectionWidth-10, h-10, 20, GxEPD_BLACK);
    }
  }
}

int calcPPWBar(int ppm, int min=400, int max=2500) {
  // Range between 400-2500

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

void displayLayout() {

  display.fillScreen(GxEPD_WHITE);
  display.fillRect(0, 120, 300, 5, GxEPD_BLACK);  // Mitte 1
  display.fillRect(0, 0, 300, 5, GxEPD_BLACK);    // Oben
  display.fillRect(0, 0, 5, 400, GxEPD_BLACK);  // Links
  display.fillRect(295, 0, 5, 400, GxEPD_BLACK);  // Rechts
  display.fillRect(0, 395, 300, 5, GxEPD_BLACK);  // Unten       

  display.fillRect(0, 245, 300, 5, GxEPD_BLACK);  // Mitte 2

  //Clocktime
  display.setFont(&FreeSansBold42pt7b);
  display.getTextBounds(clockManager.convertTimeToString().c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(((display.width() - tbw) / 2) - tbx, 70);
  display.print(clockManager.convertTimeToString().c_str());

  display.setFont(&FreeSans18pt7b);
  display.getTextBounds(clockManager.convertDateToString().c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(((display.width() - tbw) / 2) - tbx, 105);
  display.print(clockManager.convertDateToString().c_str());

  display.setFont(&customSymbols);
  display.setCursor(5, 180);
  display.print("0");

  drawProgressBar(70, 140, 215, operationMode == BRIGHTNESS ? 30 : 20, brightnessSlider);

  display.setCursor(5, 240);
  display.print("0");
  display.setCursor(5, 240);
  display.print("1");

  drawProgressBar(70, 200, 215, operationMode == LUMINOUS ? 30 : 20, luminousSlider);

  display.setCursor(5, 305);
  display.print("2");

  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(50, 290);
  display.print(meanTemperature);
  display.print("#");

  display.setFont(&customSymbols);
  display.setCursor(160, 305);
  display.print("3");

  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(220, 290);
  display.print(meanHumandity);
  display.print("%");

  display.setFont(&customSymbols);
  display.setCursor(5, 365);
  display.print("4");

  stepBar(60, 322, 225, 30, calcPPWBar(meanPPM));
}

void displayStaticElements() {
  display.setPartialWindow(0, 0, 300, 400);
  display.fillRect(0, 120, 300, 5, GxEPD_BLACK);  // Mitte 1
  display.fillRect(0, 0, 300, 5, GxEPD_BLACK);    // Oben
  display.fillRect(0, 0, 5, 400, GxEPD_BLACK);  // Links
  display.fillRect(295, 0, 5, 400, GxEPD_BLACK);  // Rechts
  display.fillRect(0, 395, 300, 5, GxEPD_BLACK);  // Unten       
  display.fillRect(0, 245, 300, 5, GxEPD_BLACK);  // Mitte 2
  
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

void displayPartialRefreshClockTime() {
  display.fillRect(10, 75, 280, -65, GxEPD_WHITE);

  display.setFont(&FreeSansBold42pt7b);
  display.getTextBounds(clockManager.convertTimeToString().c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(((display.width() - tbw) / 2) - tbx, 70);
  display.print(clockManager.convertTimeToString().c_str());
}

void displayPartialRefreshClockDate() {
  display.fillRect(10, 110, 280, -30, GxEPD_WHITE);

  display.setFont(&FreeSans18pt7b);
  display.getTextBounds(clockManager.convertDateToString().c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(((display.width() - tbw) / 2) - tbx, 105);
  display.print(clockManager.convertDateToString().c_str());
}

void displayPartialRefreshBrightnessProcessBar() {
  display.fillRect(70, 140, 215, 30, GxEPD_WHITE);
  drawProgressBar(70, 140, 215, operationMode == BRIGHTNESS ? 30 : 20, brightnessSlider);
}

void displayPartialRefreshBrightnessAutomaticProcessBar() {
  display.fillRect(70, 140, 215, 30, GxEPD_WHITE);
  drawProgressBar(70, 140, 215-40, 30, automaticBrightness);

  display.setFont(&FreeSans18pt7b);
  display.setCursor(256, 168);
  display.print('A');
}

void displayPartialRefreshLuminousProcessBar() {
  display.fillRect(70, 200, 215, 30, GxEPD_WHITE);
  drawProgressBar(70, 200, 215, operationMode == LUMINOUS ? 30 : 20, luminousSlider);
}

void displayPartialRefreshTemperature() {
  display.fillRect(50, 290, 100, -30, GxEPD_WHITE);

  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(50, 290);
  display.print(meanTemperature);
  display.print("#");
}

void displayPartialRefreshHumandity() {
  display.fillRect(220, 290, 70, -30, GxEPD_WHITE);

  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(220, 290);
  display.print(meanHumandity);
  display.print("%");
}

void displayPartialRefreshPPMStepBar() {
  display.setFont(&customSymbols);
  display.setCursor(5, 365);
  stepBar(60, 322, 225, 30, calcPPWBar(meanPPM));
}

std :: string averageAirQualityM = "Luftqualität in PPM: " + std::to_string(meanPPM);
std :: string badAirQualityM = "Lüften wird empfohlen: " + std::to_string(meanPPM);

int messageHandlerPosition = 0;

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

void displayPartialRefreshMessageHandler() {
  display.fillRect(10, 385, 280, -25, GxEPD_WHITE);

  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(10, 380);

  messageHandler();

}

void displayRefreshHandler(void (*func)()) {
  int count = 0;

  do 
  {
    func();
  }
  while(display.nextPage());
}

void environmentalData(int duration = 5000) {
  if (environmentalDataKey) {
    measureDuration.setDuration(duration);
    validationDuration.setDuration(60000);

    meanTemperature = dht20.getTemperature();
    meanHumandity = dht20.getHumidity()*100.0;
    meanPPM = CCS811.getCO2PPM();

    displayRefreshHandler(&displayPartialRefreshTemperature);
    displayRefreshHandler(&displayPartialRefreshHumandity);
    displayRefreshHandler(&displayPartialRefreshPPMStepBar);

    environmentalDataKey = false;
  }

  // every 5sek (5000ms)
  if (measureDuration.elapsed()) {
    sumTemperature += dht20.getTemperature();
    sumHumandity += dht20.getHumidity()*100.0;
    sumPPM += CCS811.getCO2PPM();
    
    counter++;

    measureDuration.setPrevious();
  }

  // every 1min (60000ms)
  if (validationDuration.elapsed()) {
    meanTemperature = round(sumTemperature / counter);
    meanHumandity = round(sumHumandity / counter);
    meanPPM = round(sumPPM / counter);

    if (settingMode == END && displayMode == OPERATION) {
      displayRefreshHandler(&displayPartialRefreshTemperature);
      displayRefreshHandler(&displayPartialRefreshHumandity);
      displayRefreshHandler(&displayPartialRefreshPPMStepBar);
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
    counter = 0;
    validationDuration.setPrevious();

  }
}

template<typename T>
void settingModePlusMinus(T value, int xOffset, int yOffset, int rectWidth, int rectHeight, int yCursor, GFXfont font) {
  do 
  { 
    display.fillRect((((display.width() - tbw) / 2) - tbx) + xOffset, yOffset, rectWidth, rectHeight, GxEPD_WHITE); // yOffset = 75 / 45 / -65

    display.setFont(&font); // FreeSansBold42pt7b
    display.setCursor((((display.width() - tbw) / 2) - tbx) + xOffset, yCursor); // yCursor = 70
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
      displayPartialRefreshPPMStepBar();
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

    Serial.printf("SETUP\n");

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
          displayPartialRefreshPPMStepBar();
          displayPartialRefreshMessageHandler();
        }
        while(display.nextPage());
        
        displayRefresh.setPrevious();
      }

    if (!systemIsOn) {
      analogWrite(pwm1, 0);
      analogWrite(pwm2, 0);
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
      Serial.printf("Power off");
    }

    break;
  
  default:
    break;
  }
}

void Task1code(void * parameter) {

  

  for(;;) {
      displayAgent(); 

      if (timeManager2.elapsed()) {
        settingModeHandler();
      }
  }
}

void Task2code(void * parameter) {
  for(;;) {
      if (timeManager2.elapsed()) {
      // Serial.printf("System status: %d\n", systemIsOn);
      // Serial.printf("Temp: %f\n", dht20.getTemperature());
      // Serial.printf("Hum: %f\n", dht20.getHumidity()*100.0);
      // Serial.printf("PPM: %d\n", CCS811.getCO2PPM());

      // Serial.printf("Mean Temp: %d\n", meanTemperature);
      // Serial.printf("Mean Hum: %d\n", meanHumandity);
      // Serial.printf("Mean PPM: %d\n", meanPPM);
      // Serial.printf("Mean LDR: %d\n", meanLDR);
      // Serial.printf("-------------\n");

      Serial.printf("PIR: %d\n", digitalRead(pir));

      // Serial.printf("BASELINE: %d", CCS811.readBaseLine());

    //   // Serial.printf("Brightness: %d\n", brightnessSlider);
    //   // Serial.printf("Luminous: %d\n", luminousSlider);
    //   // Serial.printf("-------------\n");

    //   // clockManager.getDateTime();
    // clockManager.printTime();
        timeManager2.setPrevious();
     }

   if (buttonTimeManager.elapsed()) {
      buttonAgent();

      buttonTimeManager.setPrevious();
    }

    

    if (ldrTimeManager.elapsed()) {
      ldrCounter++;
      sumLDR += analogRead(ldrPin);

      if (ldrCounter >= 20) {
        ldrValue = sumLDR / ldrCounter;
        ldrCounter = 0;
        sumLDR = 0;
        
        if (automaticIsOn) {
          pwmAgent();
        }

      }

      ldrTimeManager.setPrevious();
    }
    

   

    btnPower.update();
    environmentalData();

    if (systemIsOn) {
      btnBrightness.update();
      btnLuminous.update();
      btnHigher.update();
      btnLower.update();

      clockManager.update();
      
    }
  }
}


void setup()
{
  Wire.end();
  Wire.begin();
  display.init();
  Serial.begin(9600);

  // TimeManager setup
  ldrTimeManager.setDuration(200);
  timeManager2.setDuration(300);
  buttonTimeManager.setDuration(200);
  messageLoopTimeManager.setDuration(200);

  displayRefresh.setDuration(30000);

  // Button setup
  btnPower.begin();
  btnBrightness.begin();
  btnLuminous.begin();
  btnHigher.begin();
  btnLower.begin();

  // PinMode setup
  pinMode(ldrPin, INPUT);
  pinMode(pwm1, OUTPUT);
  pinMode(pwm2, OUTPUT);

  pinMode(pir, INPUT);

  analogWrite(pwm1, 0);
  analogWrite(pwm2, 0);

  systemStatus.begin();
  brightness.begin();
  luminous.begin();
  mode.begin();

  while(dht20.begin()) {
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