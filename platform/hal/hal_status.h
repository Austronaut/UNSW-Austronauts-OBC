/**
 * @file   hal_status.h
 * @brief  Status codes and contract-check support shared by all HAL modules.
 *
 * See HAL_CONVENTIONS.md section 7 for when each code is returned and when
 * HAL_DEBUG_TRAP() is used.
 */

#ifndef HAL_STATUS_H
#define HAL_STATUS_H

#include <stdbool.h>
#include <stdint.h>

/** Result of a HAL operation that can fail. */
typedef enum {
    HAL_OK = 0,                 /**< Success. */
    HAL_ERR_INVALID_ARG,        /**< An argument broke the function's contract. */
    HAL_ERR_NOT_INITIALISED,    /**< The module or a dependency is not initialised. */
    HAL_ERR_UNSUPPORTED,        /**< Valid request that the driver does not implement. */
    HAL_ERR_TIMEOUT             /**< The operation did not finish within its budget. */
} hal_status_t;

/**
 * True while hal_debug_trap() is holding execution. Write 0 from the
 * debugger (Watch view, or the Memory Browser at &hal_debug_trap_hold) to
 * resume. For debugger use only; only hal_status.c writes it in code.
 */
extern volatile bool hal_debug_trap_hold;

/**
 * @brief  Records a programmer error and, in debug builds, holds execution.
 *
 * Called through HAL_DEBUG_TRAP() when a contract check fails. Every call
 * increments the contract-violation count. When HAL_DEBUG_TRAPS is defined
 * (the Debug build configuration only), it then holds until the debugger
 * clears hal_debug_trap_hold, with the faulty caller one frame below. Each
 * call re-arms the hold.
 *
 * @pre    With HAL_DEBUG_TRAPS defined, the watchdog is stopped, or it will
 *         reset the device while held.
 * @post   The contract-violation count is one higher, unless it was already
 *         UINT16_MAX. hal_debug_trap_hold is false.
 *
 * @par Blocking
 *      With HAL_DEBUG_TRAPS defined: yes, with no bound, until a debugger
 *      clears hal_debug_trap_hold. This is the one deliberate exception to
 *      the bounded-blocking rule. Without it: no.
 * @par ISR-safe
 *      Yes. With HAL_DEBUG_TRAPS defined, holds the whole CPU when called
 *      from an ISR.
 */
void hal_debug_trap(void);

/**
 * @brief  Returns how many contract violations have occurred since reset.
 *
 * @pre    None.
 *
 * @par Blocking
 *      No.
 * @par ISR-safe
 *      Yes. The count is 16 bits, which this CPU reads in one access.
 *
 * @return Number of HAL_DEBUG_TRAP() calls since reset, saturating at
 *         UINT16_MAX.
 */
uint16_t hal_status_get_contract_violations(void);

/** Reports a failed contract check. See hal_debug_trap(). */
#define HAL_DEBUG_TRAP() hal_debug_trap()

#endif // HAL_STATUS_H
