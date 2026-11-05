# HAL Coding Conventions

C coding conventions for the OBC HAL layer. They are meant to keep the codebase consistent while it is small, so the rules here are deliberately few.

## 1. File Layout

All HAL code lives in `platform/hal/`. Every HAL file is prefixed with `hal_`:

```text
platform/
├── main.c
└── hal/
    ├── hal_status.h
    ├── hal_clock.h
    ├── hal_clock.c
    ├── hal_uart.h
    └── hal_uart.c
```

The `hal_` prefix keeps HAL files and symbols from colliding with other layers. For example, a future `services/uart.c` packet layer and `hal/hal_uart.c` can coexist without ambiguity.

Each module is a `.h`/`.c` pair with the same base name. The header holds the public API, and everything else stays in the `.c` file.

## 2. Includes

`platform/` (`${PROJECT_ROOT}`) is on the compiler include path, so project headers are always included **relative to `platform/`**, with quotes:

```c
#include "hal/hal_clock.h"   // correct
#include "hal_clock.h"       // wrong: only works from inside hal/
#include "../hal/hal_clock.h" // wrong: breaks if the file moves
```

**Include order** in a `.c` file:

1. Its own header first, which proves the header is self-contained.
2. Standard library headers (`<stdbool.h>`, `<stdint.h>`, ...).
3. Toolchain headers (`<msp430.h>`).
4. Other project headers (`"hal/..."`).

```c
#include "hal/hal_uart.h"

#include <stdbool.h>
#include <stdint.h>

#include <msp430.h>

#include "hal/hal_clock.h"
```

**Every header must compile on its own.** If a header uses `bool` or `uint8_t`, it includes `<stdbool.h>` or `<stdint.h>` itself instead of relying on the `.c` file to include them first. Public headers never include `<msp430.h>`; only implementation files touch registers.

## 3. Include Guards

Every header has an include guard named after its file in `UPPER_SNAKE_CASE`, with a matching comment on the `#endif`:

```c
#ifndef HAL_UART_H
#define HAL_UART_H

// ...

#endif // HAL_UART_H
```

Do not start guard names with an underscore. Names like `_HAL_UART_H` are reserved for the compiler and standard library.

## 4. Naming

| Thing                       | Style                         | Example                     |
|-----------------------------|-------------------------------|-----------------------------|
| Files                       | `lower_snake_case`, layer prefix | `hal_uart.c`             |
| Public functions            | `layer_module_verb[_object]`  | `hal_uart_init` |
| File-private functions      | `static`, `verb[_object]` | `static void clear_buffer(void)` |
| Variables / parameters      | `lower_snake_case`            | `byte_count`                |
| Public macros / constants          | `UPPER_SNAKE_CASE`, prefixed  | `HAL_CLOCK_SMCLK_HZ`        |
| Types (enums, structs)      | `layer_module_thing_t`        | `hal_uart_config_t`         |
| Enum values                 | `UPPER_SNAKE_CASE`, prefixed with the type name | `HAL_UART_PARITY_EVEN` |
| Enum count sentinel         | `..._COUNT`, last value, only where needed (see below) | `HAL_UART_INSTANCE_COUNT` |

Rules of thumb:

- **Module first, then verb**: `hal_uart_init`, not `hal_init_uart`. Autocomplete then groups everything in a module together.
- **Put units in names** when a value has one: `timeout_ms`, `HAL_CLOCK_SMCLK_HZ`, `length_bytes`. Unit mismatches are common and expensive bugs (particularly in flight software).
- Anything not needed outside its `.c` file is `static`. Only public symbols carry the `hal_` prefix.
- **Enum values carry their type's name** because C puts every enum value in one global namespace: `HAL_UART_PARITY_NONE` and `HAL_SPI_MODE_NONE` can coexist, but two plain `NONE`s cannot.
- Structs are declared as `typedef struct { ... } hal_module_thing_t;` with no struct tag, unless the struct refers to itself.
- Add a `_COUNT` sentinel only to enums used as array sizes or indices, or whose values are range-checked against it. Enums like `hal_uart_parity_t` don't need one.

## 5. Language and Types

- The project compiles as **C11** (`--c11`, both build configurations). C11 features such as `_Static_assert` and declarations inside `for` are allowed. Don't rely on TI-specific extensions where standard C11 works.
- Use `bool` from `<stdbool.h>` for true/false values in APIs. Do not use `int` flags or custom `TRUE`/`FALSE` macros.
- Use fixed-width integers from `<stdint.h>` instead of `int`, `short` or `long`. On MSP430, `int` is 16 bits, not 32, so code that assumes otherwise silently overflows. `size_t` is fine for lengths and sizes.
- Static and global variables with no initialiser start at 0, as standard C guarantees. Leave out `= 0` so the variable goes in `.bss` instead of `.data`. The project relies on this, so never build with `--zero_init=off`.

## 6. Function Contracts (Header Comments)

Every public function has a Doxygen comment directly above its declaration. The header is the documentation: a caller should never need to read the `.c` file to use a function safely.

```c
/**
 * @brief  Reads one sample from an ADC channel.
 *
 * @param[in]  channel  Input channel, 0 to 15.
 * @param[out] sample   Receives the 12-bit result on success. Must not be NULL.
 *
 * @pre    hal_adc_init() has been called.
 * @post   The ADC is idle and ready for the next conversion, whether or not
 *         the read succeeded.
 *
 * @par Blocking
 *      Yes. Waits for one conversion to finish, at most 20 us.
 * @par ISR-safe
 *      No.
 *
 * @retval HAL_STATUS_OK               sample holds a valid reading.
 * @retval HAL_STATUS_INVALID_ARG      channel is out of range or sample is NULL.
 * @retval HAL_STATUS_NOT_INITIALISED  hal_adc_init() has not been called.
 * @retval HAL_STATUS_TIMEOUT          The conversion did not finish within 20 us.
 *
 * On any error, sample is left unchanged.
 */
hal_status_t hal_adc_read(uint8_t channel, uint16_t *sample);
```

Write contracts as `/** ... */` blocks. Doxygen ignores ordinary `//` and `/* */` comments.

Tags, in this order:

| Tag                | Required             | What to write |
|--------------------|----------------------|---------------|
| `@brief`           | Always               | One line saying what it does, starting with a verb. |
| `@param[in]` / `[out]` / `[in,out]`   | Every parameter, if any | Direction: `[in]` if the function reads it, `[out]` if it writes it, `[in,out]` if both. State invalid values. |
| `@pre`             | Always               | What must be true before calling, such as init order or clock state. Write `None.` if there is nothing. |
| `@post`            | When useful          | The state guaranteed after the call returns, if it isn't obvious from `@brief`. |
| `@par Blocking`    | Always               | `No.`, or `Yes.` plus what it waits on and its worst-case bound. A blocking function with no known bound is a bug (one exception, below). |
| `@par ISR-safe`    | Always               | Whether the function may be called from an interrupt handler. If it is safe only under a condition, state the condition and why (see §8). |
| `@retval`          | Fixed set of returns | One line per possible value and what it means. Values not listed are never returned. |
| `@return`          | Computed returns     | What the value represents and its units, for functions that return a measurement or count rather than a fixed set of outcomes. |

`@par` creates a custom titled section. Doxygen has no built-in tags for blocking or interrupt context, so `Blocking` and `ISR-safe` are codebase specific.

Full contracts are required for public functions only.

**The one unbounded-blocking exception** is `hal_debug_trap()` in builds that define `HAL_DEBUG_TRAPS` (§7.3). It waits for a human at the debugger by design, and in builds without the symbol it returns immediately. **Debug builds are for attended debugging only.** Unattended tests and flight images must be built with the Release configuration.

**Public variables:** prefer a getter function. If a variable must be public, for example `hal_debug_trap_hold`, which the debugger writes, give it a `/** */` comment saying who writes it.

The module itself gets a `@file` comment at the top of its header, saying which peripheral instances it drives and any init ordering against other modules. Example:

```c
/**
 * @file   hal_uart.h
 * @brief  Driver for the eUSCI_A UART peripherals.
 *
 * Requires hal_clock_init() before hal_uart_init(), since baud
 * settings are derived from the SMCLK frequency.
 */
```

## 7. Error Handling

### 7.1 Status codes

HAL functions that can fail return `hal_status_t`, declared in `hal/hal_status.h` and shared by every HAL module:

| Value                        | Meaning |
|------------------------------|---------|
| `HAL_STATUS_OK`              | Success. Always `0`. |
| `HAL_STATUS_INVALID_ARG`     | An argument broke the function's contract, such as a NULL pointer or an out-of-range value. |
| `HAL_STATUS_NOT_INITIALISED` | The module, or a module it depends on, has not been initialised. |
| `HAL_STATUS_UNSUPPORTED`     | The request is valid but not implemented, such as an instance or setting the driver does not support yet. |
| `HAL_STATUS_TIMEOUT`         | The operation did not finish within its time budget. |

Codes are added only when a module needs one that no existing code describes. A function's `@retval` list (§6) names exactly the codes it can return.

Check results against `HAL_STATUS_OK`, not by truthiness:

```c
if (hal_uart_init(HAL_UART_INSTANCE_A0, &config) != HAL_STATUS_OK) { ... }
```

Functions that cannot fail return `void`, or a value with `@return` (§6). The exception is a function that a known requirement will make fallible, such as clock init on a board whose crystals can fail to start: it returns `hal_status_t` from the start, because changing a return type later breaks every caller. Its contract says why.

### 7.2 Three kinds of failure

| Kind                | Examples | Handling |
|---------------------|----------|----------|
| Programmer error    | NULL pointer, invalid instance, called before init | Trap in debug builds, then return an error code (§7.3). |
| Runtime condition   | Timeout | Return an error code. Never trap. |
| Hardware event      | UART framing or overrun error | Latched by the module and read through a module-specific query function. |

Programmer errors mean the calling code is wrong. Runtime conditions and hardware events are expected in normal operation, and the caller decides what to do about them.

### 7.3 Programmer errors

Every contract check calls `HAL_DEBUG_TRAP()` before returning its error:

```c
if (data == NULL) {
    HAL_DEBUG_TRAP();
    return HAL_STATUS_INVALID_ARG;
}
```

In every build, `HAL_DEBUG_TRAP()` increments a contract-violation count, which housekeeping telemetry can read with `hal_status_get_contract_violations()`.

Only builds that define `HAL_DEBUG_TRAPS` also stop execution at the faulty call, so the debugger shows where it happened; execution can be resumed from the debugger. The Debug build configuration defines `HAL_DEBUG_TRAPS` and the Release configuration does not, so trapping is opt-in: a build that leaves the symbol out is safe for flight.

The function returns the same error code in every build, so debug and flight builds take the same path after the check. A failed check must never halt flight software: it reports the error and lets the caller recover.

### 7.4 Out-parameters on failure

The contract states what happens to each `[out]` parameter on failure. The default is "left unchanged". A function that can partly succeed, such as writing some bytes before a timeout, reports its progress through an out-parameter (e.g. `size_t *bytes_written`).

### 7.5 Timeouts

Every blocking function takes a bound (§6 requires one):

- The bound covers the whole call, not each step inside it.
- `0` means try once without waiting.
- There is no "wait forever" value.

Until a timer HAL exists, timeouts count polling-loop iterations and are named `timeout_loops`. The contract states roughly how many CPU cycles one iteration takes, so callers can convert loops to time.

## 8. Shared State and Interrupts

These rules apply to any variable that both an interrupt handler (ISR) and other code access.

- **Shared variables are `volatile`**, so every access goes to memory instead of a copy the compiler kept in a register.
- **Know what is atomic.** The MSP430 reads or writes an 8-bit or 16-bit value in one instruction, which an interrupt cannot split. A 32-bit value takes two accesses, so an interrupt can land between them and the reader sees half the old value and half the new one. Pointers are 32 bits in this project's data model, so shared buffer positions are 8-bit or 16-bit indices, not pointers.
- **A single read or write of a shared 8-bit or 16-bit value needs no protection.** Anything else needs a critical section: a 32-bit access, or a read-check-update sequence on a value of any width. Don't rely on the compiler turning `x++` into one instruction.
- **Critical sections save and restore the interrupt state:**

  ```c
  unsigned short interrupt_state = __get_interrupt_state();
  __disable_interrupt();
  // ... as few instructions as possible ...
  __set_interrupt_state(interrupt_state);
  ```

  Never end one with `__enable_interrupt()`. Called from an ISR, that would re-enable interrupts inside the handler and cause the interrupt nesting this project avoids.
- **Keep critical sections short.** While interrupts are disabled, every pending interrupt waits.
