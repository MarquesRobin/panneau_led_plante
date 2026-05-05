#pragma once

void initialiserOLED();
void afficher4LignesOLED(const char* ligne1, const char* ligne2, const char* ligne3, const char* ligne4);
void afficherMenu(int selection);
void afficherModeManuel(int pourcentageEnCours, int pourcentageApplique);