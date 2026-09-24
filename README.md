# OBC Software

Software repository for the AUStronauts On-Board Computer (OBC).

The current platform targets the **TI MSP430FR5994** and is developed using **Texas Instruments Code Composer Studio (CCS)**.

## Development Setup

### Requirements

- Git
- Texas Instruments Code Composer Studio (CCS)
- MSP430 toolchain and device support installed through CCS

The CCS project is located in:

`platform/`

Import this root directory as an existing CCS project.

### MSP430 Headers

MSP430-specific headers such as:

```c
#include <msp430.h>
```

are supplied by the TI MSP430 toolchain and are **not included in this repository**.

The project should therefore be built using CCS with MSP430 device/compiler support installed. Do not copy TI toolchain headers into the repository.

Project-specific headers and source files developed by the OBC team must be committed to the repository.

## Git Workflow

The `main` branch represents the shared working version of the OBC software.

**Do not push directly to `main`.**

For all changes:

1. Pull the latest version of `main`.
2. Create a new branch for your change.
3. Make and commit your changes.
4. Push the branch to remote github
5. Open a Pull Request into `main`.
6. Merge only after review.

Example:

```bash
git checkout main
git pull
git checkout -b feat/uart-driver

git add .
git commit -m "feat: add UART initialisation"
git push -u origin feat/uart-driver
```
### Branch Conventions
**For OBC development work**, branches should use the format:
`<layer>/<system>/<short-description>`
For Example:
```text
HAL/UART/initial-uart-drivers
HAL/CAN/add-can-transmit
Services/Comms/packet-decoder
Services/Storage/fram-interface
RTOS/Tasks/telemetry-task
App/EPS/housekeeping-telemetry
```
The first component identifies the architectural layer, the second identifies the relevant subsystem or component, and the final component briefly describes the work being undertaken.
As a reminder, our 5 architectural layers are:
- `HAL` - Hardware abstraction Layer
- `RTOS` - FreeRTOS libraries and components
- `Services` - Shared libraries for the system
- `Apps` - High level system functionality


**For practice or exploratory work,** branches should use the format:
`sandbox/<name>/<short-description>`
For Example:
```text
sandbox/gus/uart-loopback
sandbox/vedang/freertos-tasks
sandbox/foxy/gpio-testing
```

### Commit Conventions

This project uses [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/#summary).

Common commit types include:

- `feat:` — new functionality
- `fix:` — bug fix
- `docs:` — documentation only
- `refactor:` — code restructuring without changing behaviour
- `test:` — tests
- `chore:` — maintenance or configuration

Examples:

```text
feat: add UART initialisation
fix: correct watchdog timeout configuration
docs: document CCS project setup
refactor: separate GPIO configuration from main
test: add packet codec tests
```

Keep commits focused on a single logical change where practical.

### Pull Requests

Pull Requests should:

- describe what was changed and why;
- contain only changes relevant to the PR;
- build successfully in CCS before review; and
- not include generated build output such as `Debug/` or `Release/`. Check gitignore for more info

If a change affects an interface used by another subsystem, document the interface change clearly in the Pull Request.

## Useful Resourcees
 - [MSP430 Code Examples](https://github.com/ticepd/msp430-examples/tree/master)
 - [Initial GPIO HAL Specification](https://peat-oboe-adb.notion.site/0-1-Initial-GPIO-HAL-3d6f31e5ae3880f1bed4ebf7fad157b9?source=copy_link)
