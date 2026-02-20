---
title: rapport_pfe_ind.md
author: Robin Marques
date: 2026-01-17
updated: 2026-01-17
type: rapport
status: active
tags:
  - rapport
  - suivis
  - projet
---

Où j'en suis :
- Faire le montage pour contrôler le panneau de LED. Cela permet de faire un contrôle 0 - 10 V avec un signal de 0 - 3.3 V.
- Faire un GitHub et un dossier propre pour mon projet.
- Faire fonctionner platformio et mettre un premier code de test sur mon esp32.
- Faire fonctionner l'écran, l'ADS et le lecteur de carte SD

Ce que je dois faire :
- Faire fonctionner mes communications I2C.
- Faire un plan pour la partie électronique.
- Tester avec le fluorimétre.
- Faire de la documentation pour le projet.

## NOTES DU 20 janvier

- La fonction millis() compte en millisecondes. De ce que gemini dit le décalage peut être de 1.7 à 4.3 seconde dans la journée. Ça serait une information à vérifier. 
- Un unsigned long est codée sur 32 bits soit 4 294 967 295 millisecondes. Ce qui fait presque 50 jours. On devrait être tranquille.

La structure "Blink Without Delay" :

```
if (tempsActuel_ms - tempsPrecedentLecture >=INTERVALLE_LECTURE) {
    tempsPrecedentLecture = tempsActuel_ms;

    [...]
  }
```
