#include "encoder.h"
#include "config.h"
#include <Arduino.h>

// --- Rotation ---

volatile int _delta = 0;

void IRAM_ATTR isrEncoder() {
    // Anti-rebond matériel : ignore les transitions < 5 ms
    static unsigned long dernierTemps = 0;
    unsigned long maintenant = micros();
    if (maintenant - dernierTemps < DEBOUNCE_ENCODER_US) return;
    dernierTemps = maintenant;

    // DT HIGH au moment où CLK tombe → sens horaire
    if (digitalRead(PIN_ENCODER_DT)) _delta++;
    else                             _delta--;
}

void initialiserEncoder() {
    pinMode(PIN_ENCODER_CLK, INPUT);
    pinMode(PIN_ENCODER_DT,  INPUT);
    pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_CLK), isrEncoder, FALLING);
}

int lireEncoderDelta() {
    noInterrupts();
    int d = _delta;
    _delta = 0;
    interrupts();
    return d;
}

// --- Bouton ---

static unsigned long _tempsPresse  = 0;
static bool _etaitPresse   = false;
static bool _courtPresse   = false;
static bool _longPresse    = false;

void mettreAJourBouton() {
    bool presse = (digitalRead(PIN_ENCODER_SW) == LOW);

    if (presse && !_etaitPresse) {
        _tempsPresse  = millis();
        _etaitPresse  = true;
    }

    if (!presse && _etaitPresse) {
        unsigned long duree = millis() - _tempsPresse;
        _etaitPresse = false;
        if (duree >= SEUIL_APPUI_LONG)  _longPresse  = true;
        else if (duree >= SEUIL_DEBOUNCE) _courtPresse = true;
    }
}

bool boutonCourtPresse() {
    if (_courtPresse) { _courtPresse = false; return true; }
    return false;
}

bool boutonLongPresse() {
    if (_longPresse) { _longPresse = false; return true; }
    return false;
}
