#pragma once

// ==============================================
// FICHIER DE CONFIGURATION UTILISATEUR
// Modifiez uniquement les valeurs numériques ci-dessous
// ==============================================

// --- Paramètres de détection ---
constexpr float SEUIL_TENSION = 3.0;         // Seuil de déclenchement (Volts)
constexpr int FREQUENCE_HZ = 20;             // Vitesse d'acquisition (Mesures par seconde)

// --- Plages d'enregistrement ---
constexpr int SECONDES_PRE_TRIGGER = 1;      // Durée mémorisée AVANT l'événement (secondes)
constexpr int SECONDES_POST_TRIGGER = 3;     // Durée enregistrée APRES l'événement (secondes)

// --- Enregistrement de fond (Heartbeat) ---
constexpr long INTERVALLE_VEILLE_SD = 5000;  // Enregistrement continu toutes les X millisecondes

// ==============================================
// CALCULS AUTOMATIQUES (Ne pas modifier)
// ==============================================
constexpr float ADS1115_VOLT_PAR_BIT = 0.000125;
constexpr long INTERVALLE_LECTURE = 1000 / FREQUENCE_HZ;
constexpr int NB_ECH_PRE = SECONDES_PRE_TRIGGER * FREQUENCE_HZ;
constexpr int NB_ECH_POST = SECONDES_POST_TRIGGER * FREQUENCE_HZ;
constexpr int TAILLE_TOTALE = NB_ECH_PRE + NB_ECH_POST;