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
 * @brief  Holds execution until the debugger clears hal_debug_trap_hold.
 *
 * Called through HAL_DEBUG_TRAP() on a programmer error, so the debugger
 * stops with the faulty caller one frame below this one. Every call re-arms
 * the trap by setting hal_debug_trap_hold to true.
 *
 * @pre    The watchdog is stopped, or it will reset the device while held.
 * @post   hal_debug_trap_hold is false.
 *
 * @par Blocking
 *      Yes, with no bound: waits until a debugger clears
 *      hal_debug_trap_hold. This is the one deliberate exception to the
 *      bounded-blocking rule; it is only reached in builds without NDEBUG.
 * @par ISR-safe
 *      Yes. Holds the whole CPU when called from an ISR.
 */
void hal_debug_trap(void);

/** Calls hal_debug_trap() in debug builds; compiles to nothing with NDEBUG. */
#ifdef NDEBUG
#define HAL_DEBUG_TRAP() ((void)0)
#else
#define HAL_DEBUG_TRAP() hal_debug_trap()
#endif

#endif // HAL_STATUS_H
