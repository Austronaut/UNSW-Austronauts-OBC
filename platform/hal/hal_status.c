#include "hal/hal_status.h"

#include <stdbool.h>
#include <stdint.h>

#include <msp430.h>

// External linkage gives the flag one fixed address, so the debugger can
// find and write it from any stack frame.
volatile bool hal_debug_trap_hold;

// Read through hal_status_get_contract_violations() so no caller can reset
// it. Stops at UINT16_MAX: wrapping to 0 would report a broken system as
// healthy.
static volatile uint16_t contract_violations;

void hal_debug_trap(void)
{
    // Critical section: an ISR that also hits a trap must not interleave
    // with this read-modify-write. Restoring the saved state (rather than
    // enabling interrupts) keeps interrupts off when called from an ISR.
    unsigned short interrupt_state = __get_interrupt_state();
    __disable_interrupt();
    if (contract_violations != UINT16_MAX) {
        contract_violations++;
    }
    __set_interrupt_state(interrupt_state);

#ifdef HAL_DEBUG_TRAPS
    // Re-arm: every trap starts held, even after an earlier one was escaped.
    hal_debug_trap_hold = true;

    // volatile forces a fresh read each pass, so a debugger write ends the loop.
    while (hal_debug_trap_hold) {
    }
#endif
}

uint16_t hal_status_get_contract_violations(void)
{
    return contract_violations;
}
