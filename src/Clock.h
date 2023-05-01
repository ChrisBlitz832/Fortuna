/*
program: Clock.cpp
usage: handling of clock-time and clock-date with RTC-module
author: Christoph Blizenetz
date: 15.03.2023
*/

#ifndef CLOCK_H
#define CLOCK_H

#include <Arduino.h>
#include <TimeManager.h>
#include <DS3231.h>
#include <Wire.h>
#include <time.h>
#include <string>
#include <iostream>

class Clock {
    int hourDigit[2];
    int minuteDigit[2];

    int yearDigit[2];
    int monthDigit[2];
    int dayDigit[2];

    int year;
    int month;
    bool centuryBit;
    int day;
    int hour;
    bool h12;
    bool hPM;
    int minute;
    int second;

    std::string clockTime = "10:05";
    std::string clockDate = "01.02.2020";

    TimeManager timeManger;
    
    RTClib myRTC;
    DS3231 myDS;

public:
    void begin();
    void update();
    std::string convertTimeToString();
    std::string convertDateToString();
    void loadTime();
    void reset();
    void printTime();
    void printTestTime();
    int getHour();

    void getDigits(int n, int arr[]);

    void setDateTime(byte year, byte month, byte date, byte hour, byte minute, byte second);
    void setRealDateTime();
    void getDateTime();

    std::string dateTimeConverter(std::string type, int digits, int offset);
};

#endif
