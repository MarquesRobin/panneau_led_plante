#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_ADS1X15.h>

#include "config.h"
#include "display.h"
#include "storage.h"
#include "encoder.h"

// --- Objets matériels ---
Adafruit_ADS1115 ads;
SPIClass spi(VSPI);

// --- Tampon circulaire ---
int16_t tampon[TAILLE_TOTALE][2];
int indexCourant = 0;
int echantillonsCapturesPost = 0;

// --- Timestamps ---
unsigned long tempsPrecedentLecture   = 0;
unsigned long tempsPrecedentVeilleSD  = 0;
unsigned long tempsPrecedentAffichage = 0;
const long INTERVALLE_AFFICHAGE = 1000;

// --- Dernières lectures ADC ---
int16_t derniere_lecture_0 = 0;
int16_t derniere_lecture_1 = 0;

// --- Machine à états acquisition ---
enum Etat { VEILLE, CAPTURE_POST, TRAITEMENT_SD };
Etat etatActuel = VEILLE;

// --- Mode principal ---
enum ModePrincipal { MENU, MODE_AFFICHAGE, MODE_MANUEL, MODE_ACQUISITION };
ModePrincipal modePrincipal = MENU;

int selectionMenu       = 0;
int pourcentageEnCours  = 0;
int pourcentageApplique = 0;

// ------------------- SETUP -------------------
void setup() {
    Serial.begin(115200);
    delay(500);

    Wire.begin(21, 22);
    initialiserOLED();
    initialiserEncoder();
    dacWrite(PIN_DAC_SORTIE, 0);

    ads.setGain(GAIN_TWOTHIRDS);
    if (!ads.begin()) {
        afficher4LignesOLED("ERREUR", "ADS1115 HS", "", "");
        while (true);
    }

    spi.begin();
    if (!initialiserSD()) {
        afficher4LignesOLED("ERREUR", "SD HS", "", "");
        while (true);
    }

    afficherMenu(selectionMenu);
    Serial.println("SYSTEME PRET.");
}

// ------------------- LOOP -------------------
void loop() {
    unsigned long tempsActuel = millis();

    // --- Lecture encodeur ---
    mettreAJourBouton();
    int  delta       = lireEncoderDelta();
    bool courtPresse = boutonCourtPresse();
    bool longPresse  = boutonLongPresse();

    // --- Retour au menu (appui long) ---
    if (longPresse && modePrincipal != MENU) {
        modePrincipal = MENU;
        etatActuel    = VEILLE;
        afficherMenu(selectionMenu);
        return;
    }

    // --- Bloc ADC (Affichage + Acquisition) ---
    if ((modePrincipal == MODE_AFFICHAGE || modePrincipal == MODE_ACQUISITION) &&
        (tempsActuel - tempsPrecedentLecture >= INTERVALLE_LECTURE)) {

        tempsPrecedentLecture = tempsActuel;
        derniere_lecture_0 = ads.readADC_SingleEnded(0);
        derniere_lecture_1 = ads.readADC_SingleEnded(1);

        // Machine à états uniquement en MODE_ACQUISITION
        if (modePrincipal == MODE_ACQUISITION) {
            switch (etatActuel) {
                case VEILLE:
                    tampon[indexCourant][0] = derniere_lecture_0;
                    tampon[indexCourant][1] = derniere_lecture_1;
                    indexCourant = (indexCourant + 1) % TAILLE_TOTALE;

                    if (derniere_lecture_0 >= (SEUIL_TENSION / ADS1115_VOLT_PAR_BIT)) {
                        etatActuel = CAPTURE_POST;
                        echantillonsCapturesPost = 0;
                        tempsPrecedentVeilleSD = tempsActuel;
                        Serial.println(">>> TRIGGER ! Passage en Capture Post <<<");
                    } else if (tempsActuel - tempsPrecedentVeilleSD >= INTERVALLE_VEILLE_SD) {
                        tempsPrecedentVeilleSD = tempsActuel;
                        ecritureSimple(tempsActuel, derniere_lecture_0, derniere_lecture_1);
                    }
                    break;

                case CAPTURE_POST:
                    tampon[indexCourant][0] = derniere_lecture_0;
                    tampon[indexCourant][1] = derniere_lecture_1;
                    indexCourant = (indexCourant + 1) % TAILLE_TOTALE;
                    echantillonsCapturesPost++;
                    if (echantillonsCapturesPost >= NB_ECH_POST) etatActuel = TRAITEMENT_SD;
                    break;

                case TRAITEMENT_SD:
                    break;
            }
        }
    }

    // --- Logique par mode ---
    switch (modePrincipal) {

        case MENU:
            if (delta != 0) {
                selectionMenu = (selectionMenu + delta + 3) % 3;
                afficherMenu(selectionMenu);
            }
            if (courtPresse) {
                if (selectionMenu == 0) {
                    modePrincipal = MODE_AFFICHAGE;
                    tempsPrecedentAffichage = 0;
                } else if (selectionMenu == 1) {
                    modePrincipal = MODE_MANUEL;
                    afficherModeManuel(pourcentageEnCours, pourcentageApplique);
                } else {
                    modePrincipal = MODE_ACQUISITION;
                    etatActuel = VEILLE;
                    indexCourant = 0;
                    echantillonsCapturesPost = 0;
                    tempsPrecedentVeilleSD  = tempsActuel;
                    tempsPrecedentLecture   = tempsActuel;
                    tempsPrecedentAffichage = 0;
                }
            }
            break;

        case MODE_AFFICHAGE:
            break;

        case MODE_MANUEL:
            if (delta != 0) {
                pourcentageEnCours = constrain(pourcentageEnCours + delta, 0, 100);
                afficherModeManuel(pourcentageEnCours, pourcentageApplique);
            }
            if (courtPresse) {
                pourcentageApplique = pourcentageEnCours;
                dacWrite(PIN_DAC_SORTIE, map(pourcentageApplique, 0, 100, 0, 255));
                afficherModeManuel(pourcentageEnCours, pourcentageApplique);
            }
            break;

        case MODE_ACQUISITION:
            if (etatActuel == TRAITEMENT_SD) {
                actualiserMetriqueRafale(tampon);
                sauvegarderTamponSD(tampon, indexCourant);
                etatActuel = VEILLE;
                Serial.println("-> Retour en VEILLE.");
            }
            break;
    }

    // --- Affichage périodique (Affichage + Acquisition) ---
    if ((modePrincipal == MODE_AFFICHAGE || modePrincipal == MODE_ACQUISITION) &&
        (tempsActuel - tempsPrecedentAffichage >= INTERVALLE_AFFICHAGE)) {

        tempsPrecedentAffichage = tempsActuel;
        float v0 = derniere_lecture_0 * ADS1115_VOLT_PAR_BIT;
        float v1 = derniere_lecture_1 * ADS1115_VOLT_PAR_BIT;
        char msg_1[16], msg_2[16];
        snprintf(msg_1, sizeof(msg_1), "V0: %.2f V", v0);
        snprintf(msg_2, sizeof(msg_2), "V1: %.2f V", v1);

        if (modePrincipal == MODE_AFFICHAGE) {
            afficher4LignesOLED("AFFICHAGE", msg_1, msg_2, "");
        } else {
            const char* etatStr = (etatActuel == VEILLE)       ? "VEILLE"   :
                                  (etatActuel == CAPTURE_POST) ? "RAFALE"   : "ECRITURE";
            afficher4LignesOLED(etatStr, msg_1, msg_2, "");
        }
    }
}
