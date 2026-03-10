#include "storage.h"
#include <SPI.h>
#include <SD.h>
#include <algorithm> // Requis pour std::sort
#include "config.h"

extern SPIClass spi;

// Instanciation de la variable globale persistante
float derniere_valeur_calculee = 0.0;

// Fonction de calcul personnalisée
float fonction_calcul(float val_1, float val_2) {
    return val_1 + val_2; 
}

// Algorithme d'extraction et d'actualisation
void actualiserMetriqueRafale(int16_t tampon[][2]) {
    int16_t copie_A0[TAILLE_TOTALE];

    // Extraction isolée pour préserver l'intégrité chronologique du tampon source
    for (int i = 0; i < TAILLE_TOTALE; i++) {
        copie_A0[i] = tampon[i][0]; // Analyse appliquée au canal A0
    }

    // Tri de la copie de travail
    std::sort(copie_A0, copie_A0 + TAILLE_TOTALE);

    // Calcul de l'espérance mathématique des queues de distribution
    long somme_basses = 0;
    for (int i = 0; i < ECHANTILLONS_BAS_EFF; i++) { 
        somme_basses += copie_A0[i]; 
    }
    float moy_basse = (float)somme_basses / ECHANTILLONS_BAS_EFF;

    long somme_hautes = 0;
    for (int i = TAILLE_TOTALE - ECHANTILLONS_HAUTS_EFF; i < TAILLE_TOTALE; i++) { 
        somme_hautes += copie_A0[i]; 
    }
    float moy_haute = (float)somme_hautes / ECHANTILLONS_HAUTS_EFF;

    // Mise à jour de l'état persistant
    derniere_valeur_calculee = fonction_calcul(moy_haute, moy_basse);
}

// --- Fonctions d'écriture modifiées pour inclure la valeur persistante ---

bool initialiserSD() {
    if (!SD.begin(5, spi)) return false;
    
    if (!SD.exists(NOM_FICHIER_SD)) {
        File dataFile = SD.open(NOM_FICHIER_SD, FILE_WRITE); 
        if (dataFile) {
            // Ajout de la colonne Valeur_Calculee
            dataFile.println("Temps_ms,ADC0,ADC1,Valeur_Calculee");
            dataFile.close();
        }
    }
    return true;
}

void ecritureSimple(unsigned long temps, int adc0, int adc1) {
    File dataFile = SD.open(NOM_FICHIER_SD, FILE_APPEND);
    if (dataFile) {
        dataFile.printf("%lu,%d,%d,%.2f\n", temps, adc0, adc1, derniere_valeur_calculee);
        dataFile.close();
    }
}

void sauvegarderTamponSD(int16_t tampon[][2], int indexCourant) {
    File dataFile = SD.open(NOM_FICHIER_SD, FILE_APPEND);
    if (dataFile) {
        unsigned long tempsFin = millis();
        for (int i = 0; i < TAILLE_TOTALE; i++) {
            int idx = (indexCourant + i) % TAILLE_TOTALE;
            unsigned long tempsEchantillon = tempsFin - ((TAILLE_TOTALE - 1 - i) * INTERVALLE_LECTURE);
            dataFile.printf("%lu,%d,%d,%.2f\n", tempsEchantillon, tampon[idx][0], tampon[idx][1], derniere_valeur_calculee);
        }
        dataFile.close();
    }
}