/**************************************************************************
 * Watch.h
 * 
 * A simple library for the DSTIKE OLED Wrist-Watch
 * https://www.tindie.com/products/lspoplove/dstike-deauther-watch-v1/
 * 
 * 
 * Hague Nusseck @ electricidea
 * v1.1 24.April.2020
 * https://github.com/electricidea/DSTIKE-NTP-Wristwatch
 * 
 * 
 * Distributed as-is; no warranty is given.
**************************************************************************/

#ifndef Watch_h
#define Watch_h

#include <Arduino.h>
#include <Wire.h>
// Arduino Button Library
#include "Button.h"

/****** Neopixel ******/
// library to control the WS2812B Neopixel LED
#include <Adafruit_NeoPixel.h>
// install:
// pio lib install "Adafruit NeoPixel"
// or in platformio.ini:
// lib_deps = 28

/****** OLED display ******/
// Display type: SH1106 1.3" OLED display
// Resolution: 128 x 64 Pixel
#include "SH1106Wire.h"
// see: https://platformio.org/lib/show/2978/ESP8266%20and%20ESP32%20OLED%20driver%20for%20SSD1306%20displays
// install:
// pio lib install "ESP8266 and ESP32 OLED driver for SSD1306 displays"
// or
// pio lib install 2978@4.1.0
// or in platformio.ini:
// lib_deps = 2978@4.1.0
// NOTE:
// With version 4.2.0 the screen is not working
//
// include Custom fonts Created by http://oleddisplay.squix.ch/
#include "font.h"

// Navigation Button on the side
#define NAV_BUTTON_UP_PIN 12
#define NAV_BUTTON_DOWN_PIN 13
#define NAV_BUTTON_PUSH_PIN 14

/****** OLED display ******/
// Display type: SH1106 1.3" OLED display
// Resolution: 128 x 64 Pixel
// Pin definitions for I2C connected OLED display
#define OLED_SDA_PIN    D1  // pin 5
#define OLED_SCL_PIN    D2  // pin 4
#define OLED_ADDR       60  //0x3C
#define OLED_WIDTH      128
#define OLED_CENTER_W   64
#define OLED_HEIGHT     64
#define OLED_CENTER_H   32

// usefull values to display strings at the right positions
#define OLED_nLines 5
#define OLED_Line_1 0
#define OLED_Line_2 12
#define OLED_Line_3 24
#define OLED_Line_4 36
#define OLED_Line_5 48
const uint8_t OLED_lines[OLED_nLines] = {OLED_Line_1, OLED_Line_2, OLED_Line_3, OLED_Line_4, OLED_Line_5};


/****** White LED ******/
// pin number of the white LED on the side
#define WHITE_LED_PIN 16
// functions to control the LED
class White_LED{
    public:
        White_LED(uint8_t Pin);
        void on();
        void off();
        void cycle(int delay_ms);
 private:
    uint8_t LEDPin;
};


/****** Neopixel ******/
// Digital IO DATA pin connected to the NeoPixels.
#define PIXEL_PIN   15  
// functions to control the LED
class RGB_LED{
    public:
        RGB_LED(uint8_t Pin);
        void off();
        void Red(uint8_t brightness);
        void Green(uint8_t brightness);
        void Blue(uint8_t brightness);
        void White(uint8_t brightness);
        void cycle(uint8_t brightness, int delay_ms);
        Adafruit_NeoPixel pixel;
    private:
        uint8_t LEDPin;
};


/****** DSTIKE_Watch ******/
class DSTIKE_Watch{
    public:
        DSTIKE_Watch();
        void begin();
        void updateButtons();
        // Buttons
        #define DEBOUNCE_MS 10
        Button NavBtn_UP = Button(NAV_BUTTON_UP_PIN, true, DEBOUNCE_MS);
        Button NavBtn_DOWN = Button(NAV_BUTTON_DOWN_PIN, true, DEBOUNCE_MS);
        Button NavBtn_PUSH = Button(NAV_BUTTON_PUSH_PIN, true, DEBOUNCE_MS);
        // LEDS
        White_LED WhiteLED = White_LED(WHITE_LED_PIN); 
        RGB_LED RGBLED = RGB_LED(PIXEL_PIN); 
        // OLED display

        SH1106Wire OLED = SH1106Wire(OLED_ADDR, OLED_SDA_PIN, OLED_SCL_PIN);

        void drawString(int16_t x, int16_t y, String text);
        void println(String text);
        void updateDisplay();
        void setFont(const uint8_t *fontData);
        // possible values for Text Alignment:
        // TEXT_ALIGN_LEFT
        // TEXT_ALIGN_RIGHT
        // TEXT_ALIGN_CENTER
        // TEXT_ALIGN_CENTER_BOTH
        void setTextAlignment(OLEDDISPLAY_TEXT_ALIGNMENT textAlignment);
        void screenOn();
        void screenOff();
        void clearScreen();
        void screenBrightness(uint8_t brightness);
        bool screenState = true;
    private:
        bool isInitialized;
        uint8_t print_line = 0;
};

extern DSTIKE_Watch Watch;

#endif
