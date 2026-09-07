#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

// --- Polling interface ---

// Configures P5.5 (SW2) as input with pull-up.
void button_init(void);

// Returns true if the button is currently pressed.
bool button_is_pressed(void);

// --- Interupt interface ---

// callback runs from inside the ISR when the button is pressed.
void button_interrupt_init(void (*callback)(void));

#endif // BUTTON_H
