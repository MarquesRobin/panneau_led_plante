#pragma once
#include <Arduino.h>

bool initialiserSD();
void ecritureSimple(unsigned long temps, int adc);
void sauvegarderTamponSD(const int16_t* tampon, int indexCourant);