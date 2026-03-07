SHELL := /bin/bash

ROOT := $(CURDIR)
BUILD_DIR := $(ROOT)/build
RESULTS_DIR := $(ROOT)/results
GUEST_BIN := $(BUILD_DIR)/sleepdemo
INITRAMFS := $(BUILD_DIR)/initramfs.cpio.gz
KERNEL ?= /boot/vmlinuz-$(shell uname -r)

.PHONY: all build clean initramfs run-qemu measure dirs

all: build

dirs:
	mkdir -p $(BUILD_DIR) $(RESULTS_DIR)

build: dirs $(GUEST_BIN)

$(GUEST_BIN): guest/sleepdemo.c
	gcc -static -O2 -Wall -Wextra -o $(GUEST_BIN) guest/sleepdemo.c

initramfs: build
	guest/build-initramfs.sh $(BUILD_DIR) $(GUEST_BIN) $(INITRAMFS)

run-qemu: initramfs
	guest/run-qemu.sh --kernel $(KERNEL) --initramfs $(INITRAMFS)

measure: initramfs
	scripts/measure_run.py --kernel $(KERNEL) --initramfs $(INITRAMFS) --results-dir $(RESULTS_DIR)

clean:
	rm -rf $(BUILD_DIR) $(RESULTS_DIR)

