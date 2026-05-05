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

void afficherMenu(int selection) {
    if (!ecranOLEDInitialise) return;
    const char* modes[] = { "Affichage", "Manuel", "Acquisition" };
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("--- MENU ---");
    for (int i = 0; i < 3; i++) {
        display.setCursor(0, 16 + i * 16);
        display.print(i == selection ? "> " : "  ");
        display.println(modes[i]);
    }
    display.display();
}

void afficherModeManuel(int enCours, int applique) {
    char l2[16], l3[16];
    snprintf(l2, sizeof(l2), "Select: %3d%%", enCours);
    snprintf(l3, sizeof(l3), "Sortie: %3d%%", applique);
    afficher4LignesOLED("MODE MANUEL", l2, l3, "BP: appliquer");
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