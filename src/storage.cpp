#include "storage.h"
#include <SPI.h>
#include <SD.h>
#include "config.h"
#include "display.h" // Pour afficher les erreurs SD sur l'écran

const int PIN_CS_SD = 5;
const char* NOM_FICHIER_SD = "/mes_donnees.csv"; 
extern SPIClass spi; // Référence à l'objet SPI instancié dans le main

bool initialiserSD() {
    if (!SD.begin(PIN_CS_SD, spi)) {
        return false;
    }
    
    if (!SD.exists(NOM_FICHIER_SD)) {
        File dataFile = SD.open(NOM_FICHIER_SD, FILE_WRITE); 
        if (dataFile) {
            dataFile.println("Temps_ms,ADC_Brute");
            dataFile.close();
        }
    }
    return true;
}

void ecritureSimple(unsigned long temps, int adc) {
    File dataFile = SD.open(NOM_FICHIER_SD, FILE_APPEND);
    if (dataFile) {
        dataFile.printf("%lu,%d\n", temps, adc);
        dataFile.close();
        Serial.printf("Fond -> %lu : %d\n", temps, adc);
    } else {
        Serial.println("ERREUR SD (Ecriture Simple)");
    }
}

void sauvegarderTamponSD(const int16_t* tampon, int indexCourant) {
    File dataFile = SD.open(NOM_FICHIER_SD, FILE_APPEND);
    if (dataFile) {
        unsigned long tempsFin = millis();
        Serial.println("-> Ecriture RAFALE (Pre + Post)...");

        for (int i = 0; i < TAILLE_TOTALE; i++) {
            int idx = (indexCourant + i) % TAILLE_TOTALE;
            unsigned long tempsEchantillon = tempsFin - ((TAILLE_TOTALE - 1 - i) * INTERVALLE_LECTURE);
            dataFile.printf("%lu,%d\n", tempsEchantillon, tampon[idx]);
        }
        dataFile.close();
        Serial.println("-> Sauvegarde terminée.");
        afficherMessageOLED("SUCCES", "Rafale Sauvegardee");
    } else {
        Serial.println("ERREUR SD (Rafale)");
    }
}