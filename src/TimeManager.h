/*
program: TimeManager.cpp
usage: handling of time with millis()
author: Christoph Blizenetz
date: 26.03.2022
*/

#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>

class TimeManager {
    unsigned long currentTime;
    unsigned long elapsedTime;
    unsigned long previousTime;
    unsigned long duration;
    bool cont;
  
public:
    TimeManager();
    void setDuration(unsigned long duration);
    void setPrevious();
    void setPreviousExtra();
    bool elapsed();

    unsigned long getCurrentTime();
    unsigned long getElaspedTime();
    unsigned long getPreviousTime();
};

#endif
