/*
Programm: Button.h
Zweck: Handling der Buttons inkl. longClick
Autor: Christoph Blizenetz
Datum: 06.05.2022 
*/

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <Arduino.h>

enum ButtonState {PRESSED, RELEASED};
enum ClickState {UNCLICKED, HAS_PRESSED, HAS_LONG_PRESSED, HAS_CLICKED};

class Button {
  unsigned char pinNr;
  bool isActiveLow;
  unsigned long startTime;
  unsigned long waitTime = 500;
  unsigned long runTime;

  ButtonState btnState;
  ClickState clickState;

public:
  Button(unsigned char pinNr, bool isActiveLow);

  void begin();
  void update();
  bool hasPressed();
  bool hasLongPressed();
  bool isPressed();
};

#endif
