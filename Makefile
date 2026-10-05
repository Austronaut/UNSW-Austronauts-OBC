# Host build and tests for code that lives outside the CCS project.
#
#   make test           build and run all host unit tests
#   make msp430-check   compile lib/ax25 for MSP430 (16-bit int) with clang
#   make clean

CC        ?= cc
BUILD     := build

WARNINGS  := -Wall -Wextra -Werror -pedantic -Wconversion -Wsign-conversion -Wshadow
# ASan hangs at process start-up on macOS 26 (even for an empty main), so
# macOS defaults to UBSan only. Override with e.g. `make test SANITIZE=`.
ifeq ($(shell uname -s),Darwin)
SANITIZE  ?= -fsanitize=undefined -fno-sanitize-recover=all
else
SANITIZE  ?= -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
endif
CFLAGS    := -std=c99 -g -O1 $(WARNINGS) $(SANITIZE)

# ---------------------------------------------------------------- lib/ax25
AX25_DIR   := lib/ax25
AX25_INC   := -I$(AX25_DIR)/include
AX25_SRC   := $(wildcard $(AX25_DIR)/src/*.c)
AX25_HDR   := $(wildcard $(AX25_DIR)/include/ax25/*.h)
AX25_TESTS := bits fcs frame hdlc line codec
AX25_BINS  := $(addprefix $(BUILD)/ax25/test_,$(AX25_TESTS))

$(BUILD)/ax25/test_%: $(AX25_DIR)/test/test_%.c $(AX25_SRC) $(AX25_HDR) $(AX25_DIR)/test/minitest.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(AX25_INC) -o $@ $< $(AX25_SRC)

.PHONY: test test-ax25 msp430-check clean

test: test-ax25

test-ax25: $(AX25_BINS)
	@set -e; for t in $(AX25_BINS); do ./$$t; done

# ---------------------------------------------------------------- MSP430
# Apple's clang has no MSP430 backend; Homebrew LLVM does.
MSP430_CC     ?= $(firstword $(wildcard /opt/homebrew/opt/llvm/bin/clang /usr/local/opt/llvm/bin/clang) clang)
MSP430_CFLAGS := --target=msp430 -ffreestanding -std=c99 -Os $(WARNINGS)

msp430-check:
	@mkdir -p $(BUILD)/msp430
	@echo 'typedef char int_is_16_bit[(sizeof(int) == 2) ? 1 : -1];' | \
	    $(MSP430_CC) $(MSP430_CFLAGS) -x c -c - -o $(BUILD)/msp430/int_size.o
	@set -e; for src in $(AX25_SRC); do \
	    echo "  MSP430 CC $$src"; \
	    $(MSP430_CC) $(MSP430_CFLAGS) $(AX25_INC) -c $$src -o $(BUILD)/msp430/$$(basename $$src .c).o; \
	done
	@echo "msp430-check: lib/ax25 compiles for MSP430 (sizeof(int) == 2)"

clean:
	rm -rf $(BUILD)
