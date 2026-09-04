#ifndef LEDS_H
#define LEDS_H

// Configures P1.0 (Red) & P1.6 (Grn) as outputs. Red LED ON, Green LED OFF.
// Call before using leds_toggle.
void leds_init(void);

// Flips both LEDs' state. Call to alternate red/green. 
void leds_toggle(void);

#endif // LEDS_H
