/*
 * APPLICATION : Enregistreur de luminosité sur seuil
 * MATERIEL : ESP32, LDR, Module SD Card, ADS1115, OLED
 *
 * MODIFICATION (2025-10-31) :
 * 1. Migration LDR vers ADC interne (GPIO 34).
 * 2. Réactivation de l'affichage OLED (SSD1306).
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// Bibliothèques I2C
#include <Wire.h>
// --- ECRAN REACTIVE ---
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h> // ADS1115 est requis

// --- I. Configuration Matérielle (LDR) ---
const long R_REFERENCE = 10000; 
const float ALIMENTATION_V = 3.3; 
// Pin pour la LDR sur l'ADC interne de l'ESP32 (ADC1_CH6)
const int PIN_LDR_ESP = 34; 

// --- II. Configuration Matérielle (SD) ---
const int PIN_CS_SD = 5;      
const char* NOM_FICHIER = "/luminosite_data.csv"; 

// --- III. Configuration Matérielle (I2C Devices) ---
// --- ECRAN REACTIVE ---
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool ecranOLEDInitialise = false; // Sera mis à true dans setup() si OK

// ADS1115 (ACTIF pour l'init)
Adafruit_ADS1115 ads;
const float ADS1115_VOLT_PAR_BIT = 0.000125; 

// --- IV. Variables d'État et de Chronométrage ---
const float SEUIL_TENSION = 3.0; 
const long DUREE_ENREGISTREMENT_MS = 20000; 

bool estEnregistrementActif = false;
unsigned long tempsDebutEnregistrement = 0;

unsigned long tempsPrecedentLecture = 0;
const long INTERVALLE_LECTURE = 50; 

unsigned long tempsPrecedentAffichage = 0;
// Réduit à 1 seconde pour un affichage plus réactif
const long INTERVALLE_AFFICHAGE = 1000; 

// --- V. Initialisation explicite du bus SPI ---
SPIClass spi = SPIClass(VSPI);

// ------------------- FONCTION D'AFFICHAGE OLED (REACTIVE) -------------------
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


// ------------------- FONCTION D'ÉCRITURE SUR SD -------------------
// (Inchangée)
void ecrireDonnees(int adc, float tension, float resistance) {
  File dataFile = SD.open(NOM_FICHIER, FILE_APPEND);
  
  if (dataFile) {
    char dataString[100];
    snprintf(dataString, sizeof(dataString), 
             "%lu,%d,%.4f,%.0f", 
             millis(), adc, tension, resistance);
    
    dataFile.println(dataString);
    dataFile.close();
    
    Serial.print("Enregistrement -> ");
    Serial.println(dataString);
  } else {
    Serial.println("ERREUR SD: Impossible d'ouvrir le fichier pour ecriture.");
  }
}

// ------------------- FONCTION SETUP -------------------
void setup() {
  Serial.begin(115200);
  delay(500); 
  Serial.println("\n[SYSTEME TEST] Initialisation (OLED ACTIVE)...");

  // --- Initialisation ADC interne ESP32 ---
  Serial.print("-> Configuration ADC interne (Pin ");
  Serial.print(PIN_LDR_ESP);
  Serial.println(")...");
  pinMode(PIN_LDR_ESP, INPUT);
  analogReadResolution(12); // Correction (était analogSetResolution)
  analogSetAttenuation(ADC_11db); 


  // --- Initialisation I2C (Bus partagé) ---
  Serial.println("-> Initialisation I2C (SDA=21, SCL=22)...");
  Wire.begin(21, 22); 

  // --- Initialisation OLED (REACTIVE) ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("ERREUR: Allocation SSD1306 echouee.");
  } else {
    Serial.println("-> Ecran OLED initialise (0x3C).");
    ecranOLEDInitialise = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED OK. Init ADS/SD...");
    display.display();
    delay(500); // Laisse le temps de lire
  }
  

  // --- Initialisation ADS1115 (ACTIF) ---
  Serial.println("-> Initialisation ADS1115 (0x48)...");
  ads.setGain(GAIN_ONE); 
  
  if (!ads.begin()) { 
    Serial.println("ERREUR FATALE: ADS1115 non detecte (0x48).");
    afficherMessageOLED("ERREUR", "ADS1115 non detecte");
    while(1);
  }
  Serial.println("-> ADS1115 initialise (0x48).");


  // --- Initialisation SPI ---
  spi.begin(); 
  Serial.println("-> Bus SPI (VSPI) initialise sur SCK=18, MISO=19, MOSI=23");

  // --- Initialisation SD Card ---
  if (!SD.begin(PIN_CS_SD, spi)) {
    Serial.println("ERREUR FATALE: Initialisation de la carte SD echouee.");
    afficherMessageOLED("ERREUR", "Carte SD non detectee"); // REACTIVE
    while (true); 
  }

  Serial.println("-> Carte SD initialisee.");
  
  File dataFile = SD.open(NOM_FICHIER, FILE_WRITE); 
  if (dataFile) {
    dataFile.println("Temps_ms,ADC_Brute_12bit,Tension_V,Resistance_Ohm");
    dataFile.close();
    Serial.println("-> Fichier CSV initialise avec en-tete.");
  } else {
    Serial.println("ERREUR SD: Impossible d'ecrire l'en-tete du fichier.");
    afficherMessageOLED("ERREUR", "Ecriture SD echouee"); // REACTIVE
  }

  Serial.println("\n[SYSTEME PRET] En attente du depassement du seuil de luminosite...");
  afficherMessageOLED("PRET", "En attente de seuil"); // REACTIVE
}

// ------------------- FONCTION LOOP -------------------
void loop() {
  unsigned long tempsActuel = millis();
  
  // --- Bloc principal cadencé à 20 Hz (toutes les 50ms) ---
  if (tempsActuel - tempsPrecedentLecture >= INTERVALLE_LECTURE) {
    tempsPrecedentLecture = tempsActuel; 

    // --- (BLOC ADS1115 COMMENTE - Inchangé) ---
    /*
    ... (Logique ADS1115 obsolète) ...
    */
    
    // --- Acquisition LDR sur ESP32 ADC (Inchangé) ---
    int valeur_adc_brute = analogRead(PIN_LDR_ESP);
    float tension_v = (float)valeur_adc_brute / 4095.0 * ALIMENTATION_V;
    float resistance_ldr_ohm;
    if (tension_v < (ALIMENTATION_V - 0.01)) { 
      resistance_ldr_ohm = R_REFERENCE * (tension_v / (ALIMENTATION_V - tension_v));
    } else {
      resistance_ldr_ohm = -1.0; 
    }
    
    // 2. Logique de Détection et d'Enregistrement
    if (tension_v > SEUIL_TENSION && !estEnregistrementActif) {
      estEnregistrementActif = true;
      tempsDebutEnregistrement = tempsActuel;
      Serial.println("\n--- SEUIL DEPASSE : DEMARRAGE DE L'ENREGISTREMENT (20 secondes) ---");
      afficherMessageOLED("ENREG.", "Collecte en cours..."); // REACTIVE
    }

    // 3. Phase d'enregistrement
    if (estEnregistrementActif) {
      ecrireDonnees(valeur_adc_brute, tension_v, resistance_ldr_ohm);

      // 4. Vérification de la fin
      if (tempsActuel - tempsDebutEnregistrement >= DUREE_ENREGISTREMENT_MS) {
        estEnregistrementActif = false;
        Serial.println("--- FIN DE L'ENREGISTREMENT (20 secondes ecoulees) ---");
        Serial.println("\n[SYSTEME PRET] En attente du depassement du seuil de luminosite...");
        afficherMessageOLED("PRET", "En attente de seuil"); // REACTIVE
      }
    }
  }

  // --- Bloc d'affichage d'état (Moniteur Série ET OLED) ---
  if (tempsActuel - tempsPrecedentAffichage >= INTERVALLE_AFFICHAGE) {
    tempsPrecedentAffichage = tempsActuel;

    if (!estEnregistrementActif) {
      
      int adc = analogRead(PIN_LDR_ESP);
      float tension = (float)adc / 4095.0 * ALIMENTATION_V;

      // Affichage Série (Inchangé)
      Serial.print("[SONDE] Lecture periodique | Tension: ");
      Serial.print(tension, 4); 
      Serial.print(" V (Seuil: ");
      Serial.print(SEUIL_TENSION, 1);
      Serial.println(" V)");

      // --- ECRAN REACTIVE ---
      if (ecranOLEDInitialise) {
        char tensionStr[20];
        // Formate la tension pour l'affichage (ex: "T: 1.234 V")
        snprintf(tensionStr, sizeof(tensionStr), "T: %.3f V", tension);
        afficherMessageOLED("PRET", tensionStr);
      }
    }
  }
}