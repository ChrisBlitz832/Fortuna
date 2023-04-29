/*
Programm: TimeManager.h
Zweck: Handling von Zeit
Autor: Christoph Blizenetz
Datum: 26.03.2022 
*/

#ifndef B39B9BAA_910E_47E1_9C16_AC540C822FDB
#define B39B9BAA_910E_47E1_9C16_AC540C822FDB

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


#endif /* B39B9BAA_910E_47E1_9C16_AC540C822FDB */
