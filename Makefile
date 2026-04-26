# Makefile

# Copyright (c) 2026 Omar Berrow

CC=ia16-elf-gcc
LD := ia16-elf-ld
OBJCOPY := ia16-elf-objcopy
SIZE := ia16-elf-size
NASM := nasm
CFLAGS := -O2
PAGER := less

DEPFILES := $(wildcard bin/*.d)

.PHONY: all
all: rom

bin/entry.o: src/entry.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/io.o: src/io.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/mem.asm.o: src/mem.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/ivt.o: src/ivt.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/mem.o: src/mem.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP -mcmodel=small -ffreestanding -fno-tree-loop-distribute-patterns $< -o $@
bin/main.o: src/main.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP -mcmodel=small -ffreestanding $< -o $@
bin/pic.o: src/pic.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP -mcmodel=small -ffreestanding $< -o $@
bin/uart.o: src/uart.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP -mcmodel=small -ffreestanding $< -o $@
bin/except.o: src/except.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP -mcmodel=small -ffreestanding $< -o $@
bin/lcd.o: src/lcd.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP -mcmodel=small -ffreestanding $< -o $@

bin/rom.elf: src/linker.ld bin/entry.o bin/main.o bin/io.o bin/mem.o bin/mem.asm.o bin/pic.o bin/ivt.o bin/except.o bin/uart.o bin/lcd.o | bin
	$(LD) -o$@ $(LDFLAGS) --noinhibit-exec -T $^

rom: bin/rom.elf | bin
	@$(OBJCOPY) -O binary --gap-fill 0xcc --pad-to 0x10000 bin/rom.elf rom
	@$(SIZE) -B bin/rom.elf

.PHONY: size
size: bin/rom.elf
	@$(SIZE) -B bin/rom.elf

bin:
	mkdir -p bin

clean:
	rm bin/*
	rm rom

disassemble: bin/rom.elf
	ia16-elf-objdump -d bin/rom.elf  -M intel-mnemonic,i8086 | $(PAGER)

ifneq ($(DEPFILES),)
include $(DEPFILES)
endif