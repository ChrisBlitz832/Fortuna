// file: used_pins.h
// processed: 09.01.2023 - 
// autor: Philipp Hipfinger

/*
useful links:

ESP32-PINOUT: https://cdn.shopify.com/s/files/1/0609/6011/2892/files/doc-esp32-pinout-reference-wroom-devkit.png?width=1038

*/

// PINs für Arduino
// PINs für ESP32 anpassen [siehe PIN-Belegung]

// Empfängermodul der Uhrzeit:
#define PIN_DCF_Signal 2
#define PIN_CLOCK_Power 0 // nicht vorhanden 

// CCS811 Modul:
#define PIN_CCS811_Power 0 // nicht vorhanden

// Knöpfe:
#define button_brightness 9
#define button_intensity 8
#define button_modus 7
#define button_power 0 // nicht vorhanden bei Probeaufbau

// LDR - Sensor
#define LDR1 36
#define LDR2 39

// LEDs - Steuerung 
#define LED_STRING_WARM 5
#define LED_STRING_COLD 6