#ifndef CLOCK_H
#define CLOCK_H

#define CLOCK_SMCLK_HZ 8000000UL

// Configures the master clock system to run at 8MHz.
// Call once, before initializing any peripheral whose timing
// depends on clock speed.
void clock_init_8mhz(void);

#endif // CLOCK_H
