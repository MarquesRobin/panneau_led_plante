#pragma once
#include <Arduino.h>

bool initialiserSD();
void ecritureSimple(unsigned long temps, int adc_0, int adc_1);
void sauvegarderTamponSD(int16_t tampon[][2], int indexCourant);