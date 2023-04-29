// file: environmental data.h
// processed: 09.01.2023 - 
// autor: Philipp Hipfinger

// libraries:
#include "Arduino.h" // arduino functions
#include "DFRobot_CCS811.h" // air quality sensor
#include "DFRobot_DHT20.h" // temperature and humidity sensor
#include "LiquidCrystal_I2C.h" // LCD - Display
#include "used_pins.h" // PINs

// DHT20 - Sensor:
DFRobot_DHT20 dht20;

// CCS811 - Sensor:
DFRobot_CCS811 CCS811;

// I2C - Display:
LiquidCrystal_I2C lcd(0x27, 16, 2);

// define:
#define duration_time 1000

// counter for the avarrge of the environmental datas
int i_tem = 0;
int i_hum = 0;
int i_co2 = 0;
int i_tvo = 0;

float result_tem = 0.0;
float result_hum = 0.0;
float result_co2 = 0.0;
float result_tvo = 0.0;

// millis variables:
unsigned long startTime;
unsigned long currentTime = millis();
unsigned long elapsedTime = currentTime - startTime;

// generell setup:
void setup_general() {
  // Serieller Monitor:
  Serial.begin(9600);

  // saving the value of millis into a veriable 
  startTime = millis();
}

// Temperatur vom DHT20 - Sensor
float get_temperature_dht20() {
  return dht20.getTemperature();
}

// Luftfeuchtigkeit vom DHT20 - Sensor
float get_humanidity_dht20() {
  return dht20.getHumidity()*100;
}

// CO2 Wert von CCS811 - Sensor:
int get_CO2PPM_CCS811() {
  return CCS811.getCO2PPM();
}

// TVOC Wert von CCS811 - Sensor:
float get_TVOCPPB_CCS811() {
  return CCS811.getTVOCPPB();
}

void setting_zero() {
  // counter auf 0 setzten
  i_tem = 0;
  i_hum = 0;
  i_co2 = 0;
  i_tvo = 0;
  
  // messergebnisse auf 0 setzen
  result_tem = 0.0;
  result_hum = 0.0;
  result_co2 = 0.0;
  result_tvo = 0.0;
}

// calculate the avarge of temperature
float get_avarge_tem(int sec) { // sec... Sekunden
  elapsedTime = currentTime - startTime;
    if (elapsedTime > duration_time) {
      startTime = currentTime;
      if (i_tem >= 0 && i_tem < sec) {
        result_tem = result_tem + get_temperature_dht20();
        i_tem++;
      } 
      return 0;
    }

    if (i_tem == sec) {
      //Serial.println(result_tem/sec);
      //result_tem = result_tem/sec;
      return result_tem;
      i_tem = 0;
      result_tem = 0.0;
    }
}

// calculate the avarge of humanidity
float get_avarage_hum(int sec) {
  elapsedTime = currentTime - startTime;
  if (elapsedTime > duration_time) {
    startTime = currentTime;
    if (i_hum >= 0 && i_hum < sec) {
      result_hum = result_hum + get_humanidity_dht20();
      i_hum++;
    } 
    return 0;
  }

  if (i_hum == sec) {
    //Serial.println(result_hum/sec);
    //result_hum = result_hum/sec;
    return result_hum;
    i_hum = 0;;
    result_hum = 0.0;
  }
}

float get_avarage_co2(int sec) {
  elapsedTime = currentTime - startTime;
  if (elapsedTime > duration_time) {
    startTime = currentTime;
    if (i_co2 >= 0 && i_co2 < sec) {
      result_co2 = result_co2 + get_CO2PPM_CCS811();
      //Serial.println(result);
      i_co2++;
    } 
    return 0;
  }

  if (i_co2 == sec) {
    //Serial.println(result_co2/sec);
    //result_co2 = result_co2/sec;
    return result_co2;
    i_co2 = 0;;
    result_co2 = 0.0;
  }
}

float get_avarage_tvo(int sec) {
  elapsedTime = currentTime - startTime;
  if (elapsedTime > duration_time) {
    startTime = currentTime;
    if (i_tvo >= 0 && i_tvo < sec) {
      result_tvo = result_tvo + get_TVOCPPB_CCS811();
      //Serial.println(result);
      i_tvo++;
    }
    return 0;
  }

  if (i_tvo == sec) {
    //Serial.println(result_tvo/sec);
    //result_tvo = result_tvo/sec;
    return result_tvo;
    i_tvo = 0;;
    result_tvo = 0.0;
  }
}

/*

float get_avarge_of_sensors(int sec, int i) {
  if (elapsedTime > duration_time) {
    startTime = currentTime;
    if (i_tem >= 0 && i_tem < sec) {
      result_tem = result_tem + get_temperature_dht20();
      i_tem++;
    } 
  }

  if (elapsedTime > duration_time) {
    startTime = currentTime;
    if (i_hum >= 0 && i_hum < sec) {
      result_hum = result_hum + get_humanidity_dht20();
      //Serial.println(result);
      i_hum++;
    } 
  }

  if (elapsedTime > duration_time) {
    startTime = currentTime;
    if (i_co2 >= 0 && i_co2 < sec) {
      result_co2 = result_co2 + get_CO2PPM_CCS811();
      //Serial.println(result);
      i_co2++;
    } 
  }

  if (elapsedTime > duration_time) {
    startTime = currentTime;
    if (i_tvo >= 0 && i_tvo < sec) {
      result_tvo = result_tvo + get_TVOCPPB_CCS811();
      //Serial.println(result);
      i_tvo++;
    } 
  }

  if (i_co2 == sec && i_hum == sec && i_tem == sec && i_tvo == sec) {
    Serial.println(result_tem/sec);
    Serial.println(result_hum/sec);
    Serial.println(result_co2/sec);
    Serial.println(result_tvo/sec);

    result_tem = result_tem/sec;
    result_hum = result_hum/sec;
    result_co2 = result_co2/sec;
    result_tvo = result_tvo/sec;

    setting_zero();
  }
}

*/

int LDR1_value() {
  int LDR1_value = 0;
  LDR1_value = analogRead(LDR1);
  //Serial.println(LDR1_value);
  return LDR1_value; 
}

int LDR2_value() {
  int LDR2_value = 0;
  LDR2_value = analogRead(LDR2);
  //Serial.println(LDR2_value);
  return LDR2_value; 
}

// I2C - Display Ausgabe
// void display(String line1, String line2) {
//   lcd.setCursor(0, 0);
//   int length1 = 0;
//   length1 = line1.length();

//   lcd.print(line1);

//   for (int i = length1; i < 16; i++) {
//     lcd.setCursor(i, 0);
//     lcd.print(" ");
//   }
  
//   lcd.setCursor(0, 1);
//   int length2 = 0;
//   length2 = line2.length();

//   lcd.print(line2);

//   for (int i = length2; i < 16; i++) {
//     lcd.setCursor(i, 1);
//     lcd.print(" ");
//   }
// }

void setup_sensor() {
  // LEDs:
  pinMode(LED_STRING_COLD, OUTPUT);
  pinMode(LED_STRING_WARM, OUTPUT);

  // Knöpfe:
  pinMode(button_brightness, INPUT);
  pinMode(button_intensity, INPUT);
  pinMode(button_modus, INPUT);
  pinMode(button_power, INPUT);

  // LDR - Sensoren:
  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);
}

void setup_lcddisplay(bool backlight_lcd) {
  // I2C - Display:
  lcd.init(); 
  
  if (backlight_lcd == HIGH) {
    lcd.backlight();
  }
}

void check_sensors() {
  
  while(dht20.begin()) {
    Serial.println("Initialize sensor failed");
    delay(1000);
  }

  while(CCS811.begin() != 0){
    Serial.println("failed to init chip, please check if the chip connection is fine");
    delay(1000);
  }
}