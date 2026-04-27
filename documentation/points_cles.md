---
title: Points clés du rapport — Panneau LED horticole
author: Robin Marques
date: 2026-04-23
type: reference
status: actif
tags: [PFE, ESP32, fluorescence, LED, présentation]
---

# Points clés à retenir pour la relecture et la présentation orale

## Contexte et problématique

- **Commanditaire** : Pr Guy Samson, laboratoire de physiologie végétale, département des sciences de l'environnement, UQTR
- **Problématique centrale** : mesurer en temps réel l'efficacité photosynthétique d'une plante (via la fluorescence chlorophyllienne) et utiliser cette mesure pour piloter dynamiquement un panneau LED horticole
- **Indicateur clé** : ΔF/Fm' = (Fm' − Fs) / Fm' — le rendement quantique effectif du photosystème II

---

## Matériel

| Composant | Rôle | Interface | Pins ESP32 |
|-----------|------|-----------|------------|
| ESP32 | Microcontrôleur central | — | — |
| ADS1115 | ADC 16 bits, lit fluorimètre (ADC0) et photomètre (ADC1) | I2C | SDA=21, SCL=22 |
| SSD1306 | Écran OLED 128×64 | I2C (bus partagé avec ADS1115) | SDA=21, SCL=22 |
| Lecteur SD | Stockage CSV | SPI | CS=5, SCK=18, MOSI=23, MISO=19 |
| DAC GPIO25 | Commande panneau LED (0-3,3V) | — | GPIO25 |
| AOP TL082CP | Amplification 0-3,3V → 0-10V (gain 3) | — | — |
| JRC6014C | Inverseur de tension (+12V → -12V) pour alimenter l'AOP | — | — |
| Panneau Black Dog PhytoMAX 4 | Éclairage horticole, accepte 0-10V | — | — |
| Encodeur rotatif | Interface utilisateur (NON implémenté) | — | À définir |
| Switchs | Modes/actions directes (NON implémentés) | — | À définir |

### Points importants sur l'ADS1115
- **GAIN_TWOTHIRDS** (plage ±6,144V, 0,1875 mV/bit) — choix délibéré car le flash du fluorimètre peut atteindre ~5V
- Anciennement GAIN_ONE (±4,096V) qui saturait lors des flashs intenses — problème résolu
- Seuil de déclenchement : 3V ≈ 16 000 en valeur brute ADS1115

### Circuit de contrôle (validé sur breadboard)
- Gain AOP : G = 1 + Rf/R1 = 1 + 36k/18k = 3 → 3,3V × 3 ≈ 9,9V ≈ 10V
- Condensateur 10 µF en parallèle sur Rf : filtre passe-bas Fc ≈ 0,44 Hz (anti-oscillation)
- Alimentation 12V tirée de l'intérieur du panneau LED

---

## Architecture logicielle

### Machine à états
```
VEILLE → (ADC0 ≥ 3V) → CAPTURE_POST → (60 échantillons) → TRAITEMENT_SD → VEILLE
```

### Trois blocs non-bloquants dans loop()
1. **Acquisition** : toutes les 50 ms (20 Hz)
2. **Sauvegarde SD** : déclenchée par les transitions d'état
3. **Affichage OLED** : toutes les secondes

### Tampon circulaire
- 80 échantillons × 2 canaux (int16_t[80][2])
- 20 pré-trigger (1s) + 60 post-trigger (3s)
- Avantage : capture le contexte AVANT le flash, sans prédiction

### Calcul ΔF/Fm'
1. Copie du canal ADC0 du tampon
2. Tri std::sort
3. Moyenne des 5% valeurs les plus basses → Fs
4. Moyenne des 5% valeurs les plus hautes → Fm'
5. ΔF/Fm' = (Fm' − Fs) / Fm'
- Résultat stocké dans `derniere_valeur_calculee` (global persistant)
- Les 5% sont configurables dans `config.h`

### Fichier CSV
- `/mes_donnees.csv` créé automatiquement
- Colonnes : `Temps_ms, ADC0, ADC1, Valeur_Calculee`
- **Attention** : Temps_ms = millis() depuis démarrage, PAS une heure absolue (pas de RTC)

---

## Ce qui est fonctionnel vs ce qui reste à faire

### Fonctionnel ✓
- Acquisition ADS1115 à 20 Hz (validé en labo)
- Calcul ΔF/Fm' en temps réel
- Sauvegarde CSV sur carte SD avec gestion d'erreur
- Circuit de contrôle 0-10V (validé sur breadboard)
- Affichage OLED (état + tensions)

### À faire ✗
- **Boucle de rétroaction** : ajuster DAC selon ΔF/Fm' → c'est l'OBJECTIF FINAL
- Interface utilisateur : encodeur rotatif + switchs
- Module RTC pour horodatage absolu
- Caractérisation panneau : courbe tension → puissance lumineuse
- Câblage de connexion au panneau (aucun câble standard disponible)
- Transfert sur PCB
- Validation expérimentale complète avec fluorimètre réel

---

## Points d'amélioration identifiés (pour la présentation)

1. **TL082CP → AOP rail-to-rail** : meilleure linéarité aux extrêmes de la plage
2. **DAC 8 bits (GPIO25) → DAC externe 12/16 bits** : résolution de commande plus fine
3. **RTC** : horodatage absolu dans le CSV
4. **Coquille dans le code** : "RAFFALE" au lieu de "RAFALE" dans l'affichage OLED

---

## Points de configuration clés (config.h)

Tous les paramètres sont dans `config.h` :
- Fréquence d'échantillonnage (20 Hz)
- Taille du pré-buffer (1s) et post-buffer (3s)
- Seuil de déclenchement (3V)
- Pourcentages pour le calcul (5% haut/bas)
- Gain ADS1115 (GAIN_TWOTHIRDS)

---

## Questions probables en soutenance

**Q : Pourquoi GAIN_TWOTHIRDS et pas un gain plus élevé pour plus de précision ?**
R : Le flash de saturation du fluorimètre peut atteindre ~5V. Avec GAIN_ONE (±4,096V), on saturait. GAIN_TWOTHIRDS (±6,144V) couvre toute la plage. La résolution de 0,1875 mV/bit reste amplement suffisante.

**Q : Pourquoi le tampon circulaire plutôt que de commencer à enregistrer dès le déclenchement ?**
R : On ne peut pas prédire le flash. Le tampon circulaire garantit qu'on a toujours 1s de données avant le déclenchement, ce qui permet de calculer Fs (fluorescence basale) correctement.

**Q : La boucle de rétroaction, comment vous envisagez de l'implémenter ?**
R : Après chaque calcul de ΔF/Fm', comparer la valeur obtenue à une valeur cible. Si ΔF/Fm' est trop bas (plante sous-saturée), augmenter la valeur DAC. Si trop haut (gaspillage), réduire. Un algorithme simple de type PID ou même un contrôleur bang-bang serait un premier pas.

**Q : Qu'est-ce qui empêche le déploiement aujourd'hui ?**
R : Principalement : (1) la boucle de rétroaction n'est pas implémentée, (2) le câblage de connexion au panneau Black Dog n'existe pas encore, (3) la validation sur plantes réelles n'a pas été faite.
