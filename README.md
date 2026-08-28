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

1. Fork this repository.
2. Create a branch in your fork for the change.
3. Make and commit your changes.
4. Push the branch to your fork.
5. Open a Pull Request into `main`.
6. Merge only after review.

Example:

```bash
git checkout -b feat/uart-driver
git add .
git commit -m "feat: add UART initialisation"
git push -u origin feat/uart-driver
```

## Commit Messages

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

## Pull Requests

Pull Requests should:

- describe what was changed and why;
- contain only changes relevant to the PR;
- build successfully in CCS before review; and
- not include generated build output such as `Debug/` or `Release/`. Check gitignore for more info

If a change affects an interface used by another subsystem, document the interface change clearly in the Pull Request.

## Useful Resourcees
 - [MSP430 Code Examples] (https://github.com/ticepd/msp430-examples/tree/master)