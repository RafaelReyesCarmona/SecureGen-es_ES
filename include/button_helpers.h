#pragma once

#include <Arduino.h>
#include "board_config.h"

// Button reading helpers with rotation support.
// Use instead of digitalRead(BUTTON_1/2) everywhere.
// Automatically swaps buttons when display is flipped 180° (rotation == 3).

// Global rotation state (loaded once at startup in main.cpp)
extern uint8_t g_displayRotation;

inline bool readBtn1() {
    bool p1 = (digitalRead(BUTTON_1) == LOW);
    bool p2 = (digitalRead(BUTTON_2) == LOW);
    return (g_displayRotation == 3) ? p2 : p1;
}

inline bool readBtn2() {
    bool p1 = (digitalRead(BUTTON_1) == LOW);
    bool p2 = (digitalRead(BUTTON_2) == LOW);
    return (g_displayRotation == 3) ? p1 : p2;
}
