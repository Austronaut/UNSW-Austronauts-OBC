# HAL Coding Conventions

C coding conventions for the OBC HAL layer. They are meant to keep the codebase consistent while it is small, so the rules here are deliberately few.

## 1. File Layout

All HAL code lives in `platform/hal/`. Every HAL file is prefixed with `hal_`:

```text
platform/
├── main.c
└── hal/
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

Rules of thumb:

- **Module first, then verb**: `hal_uart_init`, not `hal_init_uart`. Autocomplete then groups everything in a module together.
- **Put units in names** when a value has one: `timeout_ms`, `HAL_CLOCK_SMCLK_HZ`, `length_bytes`. Unit mismatches are common and expensive bugs (particularly in flight software).
- Anything not needed outside its `.c` file is `static`. Only public symbols carry the `hal_` prefix.

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
 * @retval true   sample holds a valid reading.
 * @retval false  channel is out of range, sample is NULL, or the conversion
 *                did not finish within 20 us. sample is left unchanged.
 */
bool hal_adc_read(uint8_t channel, uint16_t *sample);
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
