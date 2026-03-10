#pragma once
#include <Arduino.h>

bool initialiserSD();
void ecritureSimple(unsigned long temps, int adc_0, int adc_1);
void sauvegarderTamponSD(int16_t tampon[][2], int indexCourant);

// Déclaration de la fonction mathématique et de la procédure d'actualisation
float fonction_calcul(float val_1, float val_2);
void actualiserMetriqueRafale(int16_t tampon[][2]);