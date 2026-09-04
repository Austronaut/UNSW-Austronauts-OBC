#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

// Configures P5.5 (SW2) as input with pull-up.
void button_init(void);

// Returns true if the button is currently pressed.
bool button_is_pressed(void);

#endif // BUTTON_H
