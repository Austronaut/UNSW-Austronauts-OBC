#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>

// Configures P1.0 (Red) & P1.1 (Grn) as outputs. Red LED ON, Green LED OFF.
// Call before using any functions in this interface
void leds_init(void);

// Flips both P1.0 (Red) & P1.1 (Grn) LEDs' state. Call to alternate red/green. 
void leds_toggle(void);

// Configures P1.0 (Red)
void leds_set_red(bool on);

// Configures P1.1 (Green)
void leds_set_green(bool on);

#endif // LEDS_H
