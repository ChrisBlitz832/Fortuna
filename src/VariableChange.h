#include <Arduino.h>

#ifndef VARIABLECHANGE_H
#define VARIABLECHANGE_H

template<typename T>
class VariableChange {
    T currentValue;
    T previousValue;

public:
    VariableChange(T value) { currentValue = value; }

    void begin() { previousValue = currentValue; }

    bool hasChanged(T value) {
        currentValue = value;

        if (currentValue != previousValue) {
            previousValue = currentValue;
            return true;
        }
        else {
            return false;
        }
    }

    T getCurrentValue() { return currentValue; }
    T getPreviousValue() { return previousValue; }

};

#endif
