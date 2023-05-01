/*
program: Button.cpp
usage: handling of buttons
author: Christoph Blizenetz
date: 06.05.2022
*/

#include "Button.h"

Button :: Button(unsigned char pinNr, bool isActiveLow) {
  this->pinNr = pinNr;
  this->isActiveLow = isActiveLow;

  btnState = RELEASED;
  clickState = UNCLICKED;
}

void Button :: begin() {
  pinMode(pinNr, INPUT_PULLDOWN);
}

void Button :: update() {
  bool isActiveHigh = isActiveLow ^ digitalRead(pinNr);
  
  this->runTime = millis();

  switch(btnState) {
    case RELEASED:
      if(isActiveHigh) {
        btnState = PRESSED;
        startTime = runTime;
      }
    break;

    case PRESSED:
      if(! isActiveHigh) {
        btnState = RELEASED;
        clickState = HAS_CLICKED;
        startTime = runTime;
      }
      else if(runTime - startTime >= waitTime) {
        btnState = RELEASED;
        clickState = HAS_LONG_PRESSED;
      }
    break;
  }
}

bool Button :: hasPressed() {
  bool value = clickState != UNCLICKED;
  clickState = UNCLICKED;
  return value;
}

bool Button :: hasLongPressed() {
  if(clickState == HAS_LONG_PRESSED) {
    clickState = UNCLICKED;
    return true;
  }
  return false;
}

bool Button :: isPressed() {
  return btnState == PRESSED;
}
