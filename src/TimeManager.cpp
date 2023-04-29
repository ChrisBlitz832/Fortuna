/*
Programm: TimeManager.cpp
Zweck: Handling von Zeit
Autor: Christoph Blizenetz
Datum: 26.03.2022 
*/

#include <Arduino.h>
#include "TimeManager.h"

TimeManager :: TimeManager() {
  cont = false;
}

void TimeManager :: setDuration(unsigned long duration) {
  currentTime = millis();
  this->duration = duration;
  cont = false;
}

void TimeManager :: setPrevious() {
  previousTime = millis();
  this->previousTime = previousTime;
  cont = false;
}

unsigned long TimeManager :: getCurrentTime() {
  return millis();
}

unsigned long TimeManager :: getElaspedTime() {
  return this->elapsedTime;
}

unsigned long TimeManager :: getPreviousTime() {
  return this->previousTime;
}

bool TimeManager :: elapsed() {
  if (!cont) {
    if (millis() - previousTime >= duration) {
      cont = true;
    }
  }
  return cont;
}
