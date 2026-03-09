// ==============================================
// APPLICATION : Enregistreur Hybride (Rafale + Fond)
// MATERIEL : ESP32, Module SD Card, ADS1115, OLED
// VERSION : Sans délai de re-déclenchement
// ===============================================

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>

// --- Configuration Matérielle ---
const int PIN_CS_SD = 5;
const char* NOM_FICHIER_SD = "/mes_donnees.csv"; 

// Ecran OLED
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool ecranOLEDInitialise = false; 

// ADS1115
Adafruit_ADS1115 ads;
const float ADS1115_VOLT_PAR_BIT = 0.000125; 

// --- Paramètres de l'application ---
const float SEUIL_TENSION = 1; // Seuil de déclenchement (Volts)
const int FREQUENCE_HZ = 20;     // Fréquence d'échantillonnage
const long INTERVALLE_LECTURE = 1000 / FREQUENCE_HZ;

const int SECONDES_PRE_TRIGGER = 1;  // Temps enregistré AVANT le seuil
const int SECONDES_POST_TRIGGER = 3; // Temps enregistré APRES le seuil

// Paramètres Heartbeat (Mesure de fond)
const long INTERVALLE_VEILLE_SD = 5000; // Enregistrement calme toutes les 5s

// --- Calculs automatiques des tampons ---
const int NB_ECH_PRE = SECONDES_PRE_TRIGGER * FREQUENCE_HZ;
const int NB_ECH_POST = SECONDES_POST_TRIGGER * FREQUENCE_HZ;
const int TAILLE_TOTALE = NB_ECH_PRE + NB_ECH_POST;

// --- Variables Globales ---
int16_t tampon[TAILLE_TOTALE]; // Le tampon circulaire unique
int indexCourant = 0;
int echantillonsCapturesPost = 0;

// Variables de temps
unsigned long tempsPrecedentLecture = 0;
unsigned long tempsPrecedentVeilleSD = 0;
unsigned long tempsPrecedentAffichage = 0;
const long INTERVALLE_AFFICHAGE = 1000; 

// Machine à états
enum Etat { VEILLE, CAPTURE_POST, TRAITEMENT_SD };
Etat etatActuel = VEILLE;

// Initialisation SPI
SPIClass spi = SPIClass(VSPI);

// ------------------- FONCTIONS UTILITAIRES -------------------

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

void sauvegarderTamponSD() {
  File dataFile = SD.open(NOM_FICHIER_SD, FILE_APPEND);
  if (dataFile) {
    unsigned long tempsFin = millis();
    Serial.println("-> Ecriture RAFALE (Pre + Post)...");

    for (int i = 0; i < TAILLE_TOTALE; i++) {
      // 1. On récupère l'index circulaire (du plus vieux au plus récent)
      int idx = (indexCourant + i) % TAILLE_TOTALE;
      
      // 2. On reconstruit le temps exact rétroactivement
      unsigned long tempsEchantillon = tempsFin - ((TAILLE_TOTALE - 1 - i) * INTERVALLE_LECTURE);
      
      // 3. On écrit
      dataFile.printf("%lu,%d\n", tempsEchantillon, tampon[idx]);
    }
    dataFile.close();
    Serial.println("-> Sauvegarde terminée.");
    afficherMessageOLED("SUCCES", "Rafale Sauvegardee");
  } else {
    Serial.println("ERREUR SD (Rafale)");
  }
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);
  delay(500); 

  // Init I2C & OLED
  Wire.begin(21, 22); 
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    ecranOLEDInitialise = true;
    display.clearDisplay();
    display.display();
  }
  
  // Init ADS1115
  ads.setGain(GAIN_ONE); 
  if (!ads.begin()) { 
    afficherMessageOLED("ERREUR", "ADS1115 HS");
    while(1);
  }

  // Init SD
  spi.begin(); 
  if (!SD.begin(PIN_CS_SD, spi)) {
    afficherMessageOLED("ERREUR", "SD HS"); 
    while (true); 
  }
  
  // En-tête CSV si nouveau fichier
  if (!SD.exists(NOM_FICHIER_SD)) {
    File dataFile = SD.open(NOM_FICHIER_SD, FILE_WRITE); 
    if (dataFile) {
      dataFile.println("Temps_ms,ADC_Brute");
      dataFile.close();
    }
  }

  afficherMessageOLED("PRET", "Systeme arme");
  Serial.println("SYSTEME PRET.");
}

// ------------------- LOOP -------------------
void loop() {
  unsigned long tempsActuel = millis();

  // --- BLOC 1 : ACQUISITION (20 Hz) ---
  if (tempsActuel - tempsPrecedentLecture >= INTERVALLE_LECTURE) {
    tempsPrecedentLecture = tempsActuel;
    int16_t lecture_ads = ads.readADC_SingleEnded(0);

    switch (etatActuel) {
      
      case VEILLE:
        // Gestion du tampon circulaire (Pre-Trigger)
        tampon[indexCourant] = lecture_ads;
        indexCourant = (indexCourant + 1) % TAILLE_TOTALE;

        // Heartbeat (Mesure de fond toutes les 3s)
        if (tempsActuel - tempsPrecedentVeilleSD >= INTERVALLE_VEILLE_SD) {
          tempsPrecedentVeilleSD = tempsActuel;
          ecritureSimple(tempsActuel, lecture_ads);
        }

        // Trigger (Seuil)
        if (lecture_ads >= (SEUIL_TENSION / ADS1115_VOLT_PAR_BIT)) {
          etatActuel = CAPTURE_POST;
          echantillonsCapturesPost = 0;
          Serial.println(">>> TRIGGER ! Passage en Capture Post <<<");
        }
        break;

      case CAPTURE_POST:
        // On continue de remplir le tampon (Post-Trigger)
        tampon[indexCourant] = lecture_ads;
        indexCourant = (indexCourant + 1) % TAILLE_TOTALE;
        echantillonsCapturesPost++;

        if (echantillonsCapturesPost >= NB_ECH_POST) {
          etatActuel = TRAITEMENT_SD;
        }
        break;
      
      case TRAITEMENT_SD:
        // Pas de lecture ADC ici, on attend que le bloc de sauvegarde prenne la main
        break;
    }
  }

  // --- BLOC 2 : SAUVEGARDE (Unique) ---
  if (etatActuel == TRAITEMENT_SD) {
    sauvegarderTamponSD(); 
    // Retour immédiat en veille pour nouvelle détection
    etatActuel = VEILLE; 
    Serial.println("-> Retour immediat en mode VEILLE.");
  }

  // --- BLOC 3 : INTERFACE (1 Hz) ---
  if (tempsActuel - tempsPrecedentAffichage >= INTERVALLE_AFFICHAGE) {
    tempsPrecedentAffichage = tempsActuel;
    
    // Lecture pour affichage (indépendante du tampon)
    int16_t val = ads.readADC_SingleEnded(0); 
    float tension_v = val * ADS1115_VOLT_PAR_BIT;
    char msg[16];
    
    if (etatActuel == VEILLE) {
        snprintf(msg, sizeof(msg), "V: %.2f V", tension_v); // Affichage avec 2 décimales
        afficherMessageOLED("VEILLE", msg);
    } else if (etatActuel == CAPTURE_POST) {
        afficherMessageOLED("ALERTE", "Enregistrement...");
    } else if (etatActuel == TRAITEMENT_SD) {
        afficherMessageOLED("ECRITURE", "Sauvegarde...");
  }
  }
}