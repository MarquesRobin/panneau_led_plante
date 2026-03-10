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

void afficher4LignesOLED(const char* ligne1, const char* ligne2, const char* ligne3, const char* ligne4) {
    if (!ecranOLEDInitialise) return; 
    
    display.clearDisplay();
    
    // Taille 1 obligatoire pour s'inscrire dans l'espacement de 16px
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Distribution linéaire sur l'axe Y des ordonnées
    display.setCursor(0, 0);
    display.println(ligne1);

    display.setCursor(0, 16);
    display.println(ligne2);

    display.setCursor(0, 32);
    display.println(ligne3);

    display.setCursor(0, 48);
    display.println(ligne4);
    
    // Transfert du tampon RAM vers le contrôleur matériel
    display.display();
}