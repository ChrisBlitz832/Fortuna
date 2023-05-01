/*
program: Clock.cpp
usage: handling of clock-time and clock-date with RTC-module
author: Christoph Blizenetz
date: 15.03.2023
*/

#include "Clock.h"

#define HOUR_CHAR 0
#define HOUR (\
                ((__TIME__)[HOUR_CHAR + 0] - '0') * 10 + \
                ((__TIME__)[HOUR_CHAR + 1] - '0') * 1    \
             )

#define MINUTE_CHAR 3
#define MINUTE (\
                ((__TIME__)[MINUTE_CHAR + 0] - '0') * 10 + \
                ((__TIME__)[MINUTE_CHAR + 1] - '0') * 1    \
             )

#define SECOND_CHAR 6
#define SECOND (\
                ((__TIME__)[SECOND_CHAR + 0] - '0') * 10 + \
                ((__TIME__)[SECOND_CHAR + 1] - '0') * 1    \
             )

#define MONTH_CHAR 0
#define MONTH  (\
                ((__DATE__)[YEAR_CHAR + 1] - '0') * 100  + \
                ((__DATE__)[YEAR_CHAR + 2] - '0') * 10   + \
                ((__DATE__)[YEAR_CHAR + 3] - '0') * 1      \
              )

#define DAY_CHAR 4
#define DAY  (\
                ((__DATE__)[DAY_CHAR + 0] - '0') * 10 + \
                ((__DATE__)[DAY_CHAR + 1] - '0') * 1    \
              )

#define YEAR_CHAR 7
#define YEAR  (\
                ((__DATE__)[YEAR_CHAR + 0] - '0') * 1000 + \
                ((__DATE__)[YEAR_CHAR + 1] - '0') * 100  + \
                ((__DATE__)[YEAR_CHAR + 2] - '0') * 10   + \
                ((__DATE__)[YEAR_CHAR + 3] - '0') * 1      \
              )

void Clock :: begin() {
    this->timeManger.setDuration(60000);
    Wire.begin();
    myDS.setClockMode(false);
}

void Clock :: setDateTime(byte _year, byte _month, byte _date, byte _hour, byte _minute, byte _second) {
    myDS.setYear(_year);
    myDS.setMonth(_month);
    myDS.setDate(_date);
    myDS.setHour(_hour);
    myDS.setMinute(_minute);
    myDS.setSecond(_second);
}

void Clock :: getDateTime() {
    this->year = myDS.getYear();
    this->month = myDS.getMonth(this->centuryBit);
    this->day = myDS.getDate();
    this->hour = myDS.getHour(this->h12, this->hPM);
    this->minute = myDS.getMinute();
    this->second = myDS.getSecond();
}

void Clock :: update() {
    getDateTime();
}

void Clock :: getDigits(int n, int arr[]) {
   int rest = 0;
   int buff = n;
   
  for (int i=0; i<2; i++) {
      rest = buff%10;
      
      arr[i] = rest;
      
      buff = (buff-rest)/10;
   }
}

std::string Clock :: convertTimeToString() {
    getDigits(this->hour, this->hourDigit);
    getDigits(this->minute, this->minuteDigit);

    this->clockTime[0] = this->hourDigit[1]+ 48;
    this->clockTime[1] = this->hourDigit[0] + 48;
    this->clockTime[2] = 58;
    this->clockTime[3] = this->minuteDigit[1] + 48;
    this->clockTime[4] = this->minuteDigit[0] + 48;
    
    return this->clockTime;
}

std::string Clock :: convertDateToString() {
    getDigits(this->year, this->yearDigit);
    getDigits(this->month, this->monthDigit);
    getDigits(this->day, this->dayDigit);

    this->clockDate[0] = this->dayDigit[1]+ 48;
    this->clockDate[1] = this->dayDigit[0]+ 48;
    this->clockDate[2] = 46;
    this->clockDate[3] = this->monthDigit[1]+ 48;
    this->clockDate[4] = this->monthDigit[0]+ 48;
    this->clockDate[5] = 46;
    this->clockDate[6] = 2 + 48;
    this->clockDate[7] = 0 + 48;
    this->clockDate[8] = this->yearDigit[0]+ 48;
    this->clockDate[9] = this->yearDigit[1]+ 48;

    return this->clockDate;
}

void Clock :: printTime() {
    DateTime now = myRTC.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
}

void Clock :: printTestTime() {

    Serial.printf("Time: ");

    getDateTime();
    getDigits(this->hour, this->hourDigit);

    for (int i = 0; i<2; i++) {
        Serial.printf(", ");
        Serial.printf("%d", this->hourDigit[i]);
    }

    Serial.printf("\n");
}

void Clock :: setRealDateTime() {
    char currentDate[] = __DATE__; // "Apr  6 2023"
    char currentTime[] = __TIME__; // "16:05:22"
    
    std::string sYear = dateTimeConverter(__DATE__, 4, 8);
    std::string sMonth = dateTimeConverter(__DATE__, 3, 0);
    std::string sDay = dateTimeConverter(__DATE__, 2, 4);

    Serial.printf("%s", __TIME__);

    std::string sHour = dateTimeConverter(__TIME__, 2, 0);
    std::string sMinute = dateTimeConverter(__TIME__, 2, 3);
    std::string sSecond = dateTimeConverter(__TIME__, 2, 6);

    if (sMonth == "Jan") {
        this->month = 1;
    }
    else if (sMonth == "Feb") {
        this->month = 2;
    }
    else if (sMonth == "Mar") {
        this->month = 3;
    }
    else if (sMonth == "Apr") {
        this->month = 4;
    }
    else if (sMonth == "May") {
        this->month = 5;
    }
    else if (sMonth == "Jun") {
        this->month = 6;
    }
    else if (sMonth == "Jul") {
        this->month = 7;
    }
    else if (sMonth == "Aug") {
        this->month = 8;
    }
    else if (sMonth == "Sep") {
        this->month = 9;
    }
    else if (sMonth == "Oct") {
        this->month = 10;
    }
    else if (sMonth == "Nov") {
        this->month = 11;
    }
    else {
        this->month = 12;
    }

    setDateTime(stoi(sYear), this->month, stoi(sDay), stoi(sHour), stoi(sMinute), stoi(sSecond));
}

std::string Clock :: dateTimeConverter(std::string type, int digits, int offset) {
    std::string buffer = "";
    
    for (int i=0; i<digits; i++) {
        buffer = buffer + type[i+offset];
    }

    return buffer;
}

int Clock :: getHour() {
    update();
    return this->hour;
}