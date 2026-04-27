# Panneau LED Horticole — Système de contrôle par fluorescence chlorophyllienne

Projet de fin d'études (GEI1052) réalisé à l'Université du Québec à Trois-Rivières (UQTR) en collaboration avec le laboratoire de physiologie végétale du Pr Guy Samson.

## Objectif

Mesurer en temps réel l'efficacité photosynthétique d'une plante via la fluorescence chlorophyllienne, et utiliser cette mesure pour piloter dynamiquement l'intensité d'un panneau LED horticole (Black Dog PhytoMAX 4).

L'indicateur clé est le **rendement quantique effectif du photosystème II** :

```
ΔF/Fm' = (Fm' − Fs) / Fm'
```

- **Fs** : fluorescence basale (moyenne des valeurs les plus basses lors d'un flash)
- **Fm'** : fluorescence maximale (moyenne des valeurs les plus hautes lors d'un flash)

## Matériel

| Composant | Rôle |
|-----------|------|
| ESP32-WROOM-32E (Freenove) | Microcontrôleur central |
| ADS1115 | ADC 16 bits — lecture fluorimètre (ADC0) et photomètre (ADC1) |
| SSD1306 | Écran OLED 128×64 — affichage temps réel |
| Lecteur micro SD | Enregistrement CSV |
| TL082CP + NJU7662 | Amplification 0–3,3V → 0–10V pour piloter le panneau LED |
| Black Dog PhytoMAX 4 | Panneau LED horticole (entrée 0–10V) |

## Architecture logicielle

Le firmware tourne sur ESP32 avec PlatformIO (framework Arduino). Il est organisé en **trois blocs non-bloquants** dans la boucle principale :

```
loop()
 ├── Bloc acquisition   → toutes les 50 ms (20 Hz), via ADS1115
 ├── Bloc sauvegarde SD → déclenché par les transitions d'état
 └── Bloc affichage     → toutes les secondes, via SSD1306
```

### Machine à états

```
VEILLE ──(ADC0 ≥ 3V)──► CAPTURE_POST ──(60 échantillons)──► TRAITEMENT_SD ──► VEILLE
```

- **VEILLE** : enregistrement périodique de fond (toutes les 5 s) + tampon circulaire actif
- **CAPTURE_POST** : enregistrement intensif post-flash (3 s à 20 Hz)
- **TRAITEMENT_SD** : calcul de ΔF/Fm' et écriture du tampon en CSV

### Tampon circulaire

Le système maintient en permanence un tampon de 80 échantillons (1 s avant + 3 s après le déclenchement), permettant de capturer le contexte **avant** chaque flash saturant sans prédiction.

## Structure du projet

```
panneau_led_plante/
├── src/
│   ├── main.cpp          # Boucle principale et machine à états
│   ├── display.cpp       # Gestion écran OLED SSD1306
│   └── storage.cpp       # Gestion carte SD et calcul ΔF/Fm'
├── include/
│   ├── config.h          # Paramètres configurables (fréquence, seuil, durées...)
│   ├── display.h
│   └── storage.h
├── documentation/
│   └── datasheets/
│       └── sources.md    # Liens vers les fiches techniques des composants
└── platformio.ini
```

## Configuration

Tous les paramètres ajustables sont centralisés dans `include/config.h` :

```cpp
constexpr float SEUIL_TENSION       = 3.0;   // Seuil de déclenchement (V)
constexpr int   FREQUENCE_HZ        = 20;     // Fréquence d'acquisition (Hz)
constexpr int   SECONDES_PRE_TRIGGER = 1;     // Durée pré-flash mémorisée (s)
constexpr int   SECONDES_POST_TRIGGER = 3;    // Durée post-flash enregistrée (s)
constexpr long  INTERVALLE_VEILLE_SD = 5000;  // Enregistrement de fond (ms)
```

## Installation et compilation

**Prérequis** : [PlatformIO](https://platformio.org/) (extension VS Code ou CLI)

```bash
# Compiler
pio run

# Compiler et flasher
pio run --target upload

# Moniteur série
pio device monitor
```

## Format des données CSV

Les mesures sont enregistrées dans `/mes_donnees.csv` sur la carte SD :

```
Temps_ms,ADC0,ADC1,Valeur_Calculee
12500,1234,890,0.42
```

> **Note** : `Temps_ms` est la valeur de `millis()` depuis le démarrage de l'ESP32, pas un horodatage absolu (pas de module RTC dans la version actuelle).

## État actuel du projet

### Fonctionnel ✓
- Acquisition ADS1115 à 20 Hz
- Calcul ΔF/Fm' en temps réel
- Sauvegarde CSV sur carte SD avec gestion d'erreur
- Circuit de contrôle 0–10V (validé sur breadboard)
- Affichage OLED

### À implémenter ✗
- **Boucle de rétroaction** : ajuster automatiquement le DAC selon ΔF/Fm'
- Interface utilisateur : encodeur rotatif + switchs
- Module RTC pour horodatage absolu
- Caractérisation du panneau (courbe tension → puissance lumineuse)
- Câblage de connexion au panneau Black Dog
- Transfert sur PCB

## Crédits

- **Auteur** : Robin Marques
- **Encadrant** : Pr Guy Samson, département des sciences de l'environnement, UQTR
- **Cours** : GEI1052 — Projet de fin d'études, UQTR, 2025–2026
- **Assistance au développement** : Claude Sonnet 4.6 (Anthropic) — https://claude.ai
