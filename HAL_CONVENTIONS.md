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
| Enum count sentinel         | `..._COUNT`, last value       | `HAL_UART_INSTANCE_COUNT`   |

Rules of thumb:

- **Module first, then verb**: `hal_uart_init`, not `hal_init_uart`. Autocomplete then groups everything in a module together.
- **Put units in names** when a value has one: `timeout_ms`, `HAL_CLOCK_SMCLK_HZ`, `length_bytes`. Unit mismatches are common and expensive bugs (particularly in flight software).
- Anything not needed outside its `.c` file is `static`. Only public symbols carry the `hal_` prefix.
- **Enum values carry their type's name** because C puts every enum value in one global namespace: `HAL_UART_PARITY_NONE` and `HAL_SPI_MODE_NONE` can coexist, but two plain `NONE`s cannot.
- Structs are declared as `typedef struct { ... } hal_module_thing_t;` with no struct tag, unless the struct refers to itself.

## 5. Types

- Use `bool` from `<stdbool.h>` for true/false values in APIs. Do not use `int` flags or custom `TRUE`/`FALSE` macros.
- Use fixed-width integers from `<stdint.h>` instead of `int`, `short` or `long`. On MSP430, `int` is 16 bits, not 32, so code that assumes otherwise silently overflows. `size_t` is fine for lengths and sizes.

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
 * @retval HAL_OK                   sample holds a valid reading.
 * @retval HAL_ERR_INVALID_ARG      channel is out of range or sample is NULL.
 * @retval HAL_ERR_NOT_INITIALISED  hal_adc_init() has not been called.
 * @retval HAL_ERR_TIMEOUT          The conversion did not finish within 20 us.
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
| `@par Blocking`    | Always               | `No.`, or `Yes.` plus what it waits on and its worst-case bound. A blocking function with no known bound is a bug. |
| `@par ISR-safe`    | Always               | Whether the function may be called from an interrupt handler. |
| `@retval`          | Fixed set of returns | One line per possible value and what it means. Values not listed are never returned. |
| `@return`          | Computed returns     | What the value represents and its units, for functions that return a measurement or count rather than a fixed set of outcomes. |

`@par` creates a custom titled section. Doxygen has no built-in tags for blocking or interrupt context, so `Blocking` and `ISR-safe` are codebase specific.

Full contracts are required for public functions only.

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

| Value                     | Meaning |
|---------------------------|---------|
| `HAL_OK`                  | Success. Always `0`. |
| `HAL_ERR_INVALID_ARG`     | An argument broke the function's contract, such as a NULL pointer or an out-of-range value. |
| `HAL_ERR_NOT_INITIALISED` | The module, or a module it depends on, has not been initialised. |
| `HAL_ERR_UNSUPPORTED`     | The request is valid but not implemented, such as an instance or setting the driver does not support yet. |
| `HAL_ERR_TIMEOUT`         | The operation did not finish within its time budget. |

Codes are added only when a module needs one that no existing code describes. A function's `@retval` list (§6) names exactly the codes it can return.

Check results against `HAL_OK`, not by truthiness:

```c
if (hal_uart_init(HAL_UART_INSTANCE_A0, &config) != HAL_OK) { ... }
```

Functions that cannot fail return `void`, or a value with `@return` (§6).

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
    return HAL_ERR_INVALID_ARG;
}
```

In debug builds, `HAL_DEBUG_TRAP()` stops execution at the faulty call so the debugger shows where it happened. Execution can be resumed from the debugger. In builds that define `NDEBUG`, it compiles to nothing.

The function returns the same error code in every build, so debug and flight builds take the same path after the check. A failed check must never halt flight software: it reports the error and lets the caller recover.

### 7.4 Out-parameters on failure

The contract states what happens to each `[out]` parameter on failure. The default is "left unchanged". A function that can partly succeed, such as writing some bytes before a timeout, reports its progress through an out-parameter (e.g. `size_t *bytes_written`).

### 7.5 Timeouts

Every blocking function takes a bound (§6 requires one):

- The bound covers the whole call, not each step inside it.
- `0` means try once without waiting.
- There is no "wait forever" value.

Until a timer HAL exists, timeouts count polling-loop iterations and are named `timeout_loops`. The contract states roughly how many CPU cycles one iteration takes, so callers can convert loops to time.
