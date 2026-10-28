#include "hal/hal_status.h"

#include <stdbool.h>

// External linkage gives the flag one fixed address, so the debugger can
// find and write it from any stack frame.
volatile bool hal_debug_trap_hold;

void hal_debug_trap(void)
{
    // Re-arm: every trap starts held, even after an earlier one was escaped.
    hal_debug_trap_hold = true;

    // volatile forces a fresh read each pass, so a debugger write ends the loop.
    while (hal_debug_trap_hold) {
    }
}
