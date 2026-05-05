#pragma once

// ==============================================
// FICHIER DE CONFIGURATION UTILISATEUR
// Modifiez uniquement les valeurs numériques ci-dessous
// ==============================================

// --- Configuration Matérielle ---
constexpr const char* NOM_FICHIER_SD = "/mes_donnees.csv";

// --- Encodeur rotatif KY-040 ---
constexpr int PIN_ENCODER_CLK = 32;
constexpr int PIN_ENCODER_DT  = 33;
constexpr int PIN_ENCODER_SW  = 27;
constexpr int PIN_DAC_SORTIE  = 25;

// --- Bouton : seuils temporels ---
constexpr unsigned long SEUIL_APPUI_LONG    = 1000;  // ms
constexpr unsigned long SEUIL_DEBOUNCE      = 50;    // ms

// --- Encodeur : anti-rebond ISR ---
constexpr unsigned long DEBOUNCE_ENCODER_US = 100000; // µs — 1 compte par clic mécanique

// --- Paramètres de détection ---
constexpr float SEUIL_TENSION = 3.0;         // Seuil de déclenchement (Volts)
constexpr int FREQUENCE_HZ = 20;             // Vitesse d'acquisition (Mesures par seconde)

// --- Plages d'enregistrement ---
constexpr int SECONDES_PRE_TRIGGER = 1;      // Durée mémorisée AVANT l'événement (secondes)
constexpr int SECONDES_POST_TRIGGER = 3;     // Durée enregistrée APRES l'événement (secondes)

// --- Enregistrement de fond (Heartbeat) ---
constexpr long INTERVALLE_VEILLE_SD = 5000;  // Enregistrement continu toutes les X millisecondes

// --- Paramètres d'analyse statistique ---
constexpr int POURCENTAGE_VALEURS_HAUTES = 5;
constexpr int POURCENTAGE_VALEURS_BASSES = 5;

// ==============================================
// CALCULS AUTOMATIQUES (Ne pas modifier)
// ==============================================
constexpr float ADS1115_VOLT_PAR_BIT = 0.0001875; // GAIN_TWOTHIRDS (±6.144V FSR, entrée max ~5V avec VDD=5V)
constexpr long INTERVALLE_LECTURE = 1000 / FREQUENCE_HZ;
constexpr int NB_ECH_PRE = SECONDES_PRE_TRIGGER * FREQUENCE_HZ;
constexpr int NB_ECH_POST = SECONDES_POST_TRIGGER * FREQUENCE_HZ;
constexpr int TAILLE_TOTALE = NB_ECH_PRE + NB_ECH_POST;

// Calcul automatique du nombre d'échantillons avec sécurité anti-zéro
constexpr int NB_VAL_HAUTES = (TAILLE_TOTALE * POURCENTAGE_VALEURS_HAUTES) / 100;
constexpr int NB_VAL_BASSES = (TAILLE_TOTALE * POURCENTAGE_VALEURS_BASSES) / 100;

// Sécurité : Forçage à 1 échantillon minimum pour prévenir les divisions par zéro
constexpr int ECHANTILLONS_HAUTS_EFF = (NB_VAL_HAUTES > 0) ? NB_VAL_HAUTES : 1;
constexpr int ECHANTILLONS_BAS_EFF = (NB_VAL_BASSES > 0) ? NB_VAL_BASSES : 1;