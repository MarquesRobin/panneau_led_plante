#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool ecranOLEDInitialise = false; 

void initialiserOLED() {
    if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        ecranOLEDInitialise = true;
        display.clearDisplay();
        display.display();
    }
}

void afficherMessageOLED(const char* titre, const char* message) {
    if (!ecranOLEDInitialise) return; 
    
    display.clearDisplay();
    
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(titre, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 5);
    display.println(titre);

    display.setTextSize(1);
    display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 35);
    display.println(message);
    
    display.display();
}