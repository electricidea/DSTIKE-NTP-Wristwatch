/**************************************************************************
 * Watch.cpp
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

#include "Watch.h"

#include <Arduino.h>


/****** White LED ******/
White_LED::White_LED(uint8_t Pin) {
    // init the on-board LED (white)
    White_LED::LEDPin = Pin;
    pinMode(White_LED::LEDPin, OUTPUT);
    // turn off the LED
    White_LED::off();
}

void White_LED::on(){
    digitalWrite(White_LED::LEDPin, LOW);
}

void White_LED::off(){
    digitalWrite(White_LED::LEDPin, HIGH);
}

void White_LED::cycle(int delay_ms){
    White_LED::on();
    delay(delay_ms);
    White_LED::off();
}


/****** Neopixel ******/
RGB_LED::RGB_LED(uint8_t Pin) {
    // init the Neopixel LED (RGB)
    RGB_LED::LEDPin = Pin;
    // Declare the NeoPixel pixel object:
    RGB_LED::pixel = Adafruit_NeoPixel(1, LEDPin, NEO_GRB + NEO_KHZ800);
    // Argument 1 = Number of pixels in NeoPixel strip
    // Argument 2 = Arduino pin number (most are valid)
    // Argument 3 = Pixel type flags, add together as needed:
    //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
    //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
    //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
    //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
    //   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

    // Initialize NeoPixel strip object (REQUIRED)
    RGB_LED::pixel.begin(); 
    RGB_LED::pixel.show(); 
}

void RGB_LED::Red(uint8_t brightness){
    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    RGB_LED::pixel.setPixelColor(0, RGB_LED::pixel.Color(brightness, 0, 0));
    RGB_LED::pixel.show();  
}

void RGB_LED::Green(uint8_t brightness){
    RGB_LED::pixel.setPixelColor(0, RGB_LED::pixel.Color(0, brightness, 0));
    RGB_LED::pixel.show();  
}

void RGB_LED::Blue(uint8_t brightness){
    RGB_LED::pixel.setPixelColor(0, RGB_LED::pixel.Color(0, 0, brightness));
    RGB_LED::pixel.show();  
}

void RGB_LED::White(uint8_t brightness){
    RGB_LED::pixel.setPixelColor(0, RGB_LED::pixel.Color(brightness, brightness, brightness));
    RGB_LED::pixel.show();  
}

void RGB_LED::off(){
    RGB_LED::pixel.setPixelColor(0, RGB_LED::pixel.Color(0, 0, 0));
    RGB_LED::pixel.show();  
}
void RGB_LED::cycle(uint8_t brightness, int delay_ms){
    RGB_LED::Red(brightness);
    delay(delay_ms);
    RGB_LED::Green(brightness);
    delay(delay_ms);
    RGB_LED::Blue(brightness);
    delay(delay_ms);
    RGB_LED::White(brightness);
    delay(delay_ms);
    RGB_LED::off();
}



/****** DSTIKE_Watch ******/
DSTIKE_Watch::DSTIKE_Watch():isInitialized(0) {

}

void DSTIKE_Watch::begin(){
	
	// Allow init only once
	if (isInitialized) return;
	else isInitialized = true;
    

	// Init UART
    Serial.begin(115200);
    Serial.flush();
    delay(50);
    Serial.print("DSTIKE ESP8266 Watch initializing...");

    // Init I2C
    // is called inside the OLED library
    // Wire.begin();

	Serial.println("Init OLED Display");
    // init the OLED display
    OLED.init();
    // flip to fit for the Watch
    OLED.flipScreenVertically();
    // default font
    OLED.setFont(ArialMT_Plain_10);
    // default text alignment
    OLED.setTextAlignment(TEXT_ALIGN_LEFT);
    // activate (if not already activated)
    OLED.displayOn();
    // clear the display
    OLED.clear();
    // show the content (Write the buffer to the display memory)
    OLED.display();

    
    // turm off the Neopixel
    // otherwise it will ligt up green
    RGBLED.off();

	Serial.println("[OK] Init done");
}

// call this function inside the main loop
// to update the button states
void DSTIKE_Watch::updateButtons() {
	Watch.NavBtn_UP.read();
	Watch.NavBtn_DOWN.read();
	Watch.NavBtn_PUSH.read();
}

// dont forget to call this function after every drawing function
// otherwise, you will not see any changes
void DSTIKE_Watch::updateDisplay(){
    OLED.display();
}

// simple methos to draw a string on a specific position
void DSTIKE_Watch::drawString(int16_t x, int16_t y, String text){
    OLED.drawString(x, y, text);
}

// simple method to print text line by line
// if last line is reached, the screen is cleared automatically
// Note:
// Works with this font: Watch.setFont(DejaVu_Sans_Mono_12);
// or other fonts with a line height of 12px
void DSTIKE_Watch::println(String text){
    if(print_line >= OLED_nLines){
        OLED.clear();
        print_line = 0; 
    }
    OLED.drawString(0, OLED_lines[print_line], text);
    OLED.display();
    print_line++;
}

// to change fonts
// see font.h for available fonts
void DSTIKE_Watch::setFont(const uint8_t *fontData){
    OLED.setFont(fontData);
}

// possible values for Text Alignment:
// TEXT_ALIGN_LEFT
// TEXT_ALIGN_RIGHT
// TEXT_ALIGN_CENTER
// TEXT_ALIGN_CENTER_BOTH
void DSTIKE_Watch::setTextAlignment(OLEDDISPLAY_TEXT_ALIGNMENT textAlignment){
    OLED.setTextAlignment(textAlignment);
}

// to switch the screen ON or OFF
void DSTIKE_Watch::screenOn(){
    OLED.displayOn();
    DSTIKE_Watch::screenState = true;
}
void DSTIKE_Watch::screenOff(){
    OLED.displayOff();
    DSTIKE_Watch::screenState = false;
}

// clear teh screen and reset the line pointer for println()
void DSTIKE_Watch::clearScreen(){
    OLED.clear();
    OLED.display();
    print_line = 0;
}

// Values goes from 0 to 255
void DSTIKE_Watch::screenBrightness(uint8_t brightness){
    OLED.setBrightness(brightness);
}

// create the Watch object
DSTIKE_Watch Watch;