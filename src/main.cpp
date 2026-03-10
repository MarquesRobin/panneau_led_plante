#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_ADS1X15.h>

#include "config.h"
#include "display.h"
#include "storage.h"

// --- Objets Matériels Locaux ---
Adafruit_ADS1115 ads;
SPIClass spi(VSPI);

// --- Variables d'État Globale ---
int16_t tampon[TAILLE_TOTALE];
int indexCourant = 0;
int echantillonsCapturesPost = 0;

unsigned long tempsPrecedentLecture = 0;
unsigned long tempsPrecedentVeilleSD = 0;
unsigned long tempsPrecedentAffichage = 0;
const long INTERVALLE_AFFICHAGE = 1000; 

enum Etat { VEILLE, CAPTURE_POST, TRAITEMENT_SD };
Etat etatActuel = VEILLE;

// ------------------- SETUP -------------------
void setup() {
    Serial.begin(115200);
    delay(500); 

    Wire.begin(21, 22); 
    initialiserOLED();
    
    ads.setGain(GAIN_ONE); 
    if (!ads.begin()) { 
        afficher4LignesOLED("ERREUR", "ADS1115 HS", "", "");
        while(true);
    }

    spi.begin(); 
    if (!initialiserSD()) {
        afficher4LignesOLED("ERREUR", "SD HS", "", "");
        while(true); 
    }

    afficher4LignesOLED("PRET", "Systeme arme", "", "");
    Serial.println("SYSTEME PRET.");
}

// ------------------- LOOP -------------------
void loop() {
    unsigned long tempsActuel = millis();

    // --- BLOC 1 : ACQUISITION ---
    if (tempsActuel - tempsPrecedentLecture >= INTERVALLE_LECTURE) {
        tempsPrecedentLecture = tempsActuel;
        int16_t lecture_ads_0 = ads.readADC_SingleEnded(0);
        int16_t lecture_ads_1 = ads.readADC_SingleEnded(1);

        switch (etatActuel) {
            case VEILLE:
                tampon[indexCourant] = lecture_ads_0;
                indexCourant = (indexCourant + 1) % TAILLE_TOTALE;

                if (lecture_ads_0 >= (SEUIL_TENSION / ADS1115_VOLT_PAR_BIT)) {
                    etatActuel = CAPTURE_POST;
                    echantillonsCapturesPost = 0;
                    Serial.println(">>> TRIGGER ! Passage en Capture Post <<<");
                    tempsPrecedentVeilleSD = tempsActuel;
                }

                else if (tempsActuel - tempsPrecedentVeilleSD >= INTERVALLE_VEILLE_SD) {
                    tempsPrecedentVeilleSD = tempsActuel;
                    ecritureSimple(tempsActuel, lecture_ads_0);
                }

                break;

            case CAPTURE_POST:
                tampon[indexCourant] = lecture_ads_0;
                indexCourant = (indexCourant + 1) % TAILLE_TOTALE;
                echantillonsCapturesPost++;

                if (echantillonsCapturesPost >= NB_ECH_POST) {
                    etatActuel = TRAITEMENT_SD;
                }
                break;
            
            case TRAITEMENT_SD:
                break;
        }
    }

    // --- BLOC 2 : SAUVEGARDE ---
    if (etatActuel == TRAITEMENT_SD) {
        sauvegarderTamponSD(tampon, indexCourant); 
        etatActuel = VEILLE; 
        Serial.println("-> Retour immediat en mode VEILLE.");
    }

    // --- BLOC 3 : INTERFACE ---
    if (tempsActuel - tempsPrecedentAffichage >= INTERVALLE_AFFICHAGE) {
        tempsPrecedentAffichage = tempsActuel;
        
        int16_t val = ads.readADC_SingleEnded(0); 
        float tension_v = val * ADS1115_VOLT_PAR_BIT;
        char msg[16];
        
        if (etatActuel == VEILLE) {
            snprintf(msg, sizeof(msg), "V: %.2f V", tension_v); 
            afficher4LignesOLED("VEILLE", msg, "Attente Trigger...", "");
        } else if (etatActuel == CAPTURE_POST) {
            afficher4LignesOLED("ALERTE", "Enregistrement...", "", "");
        } else if (etatActuel == TRAITEMENT_SD) {
            afficher4LignesOLED("ECRITURE", "Sauvegarde...", "", "");
        }
    }
}