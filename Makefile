# Makefile

# Copyright (c) 2026 Omar Berrow

CC=ia16-elf-gcc
LD := ia16-elf-gcc
OBJCOPY := ia16-elf-objcopy
OBJDUMP := ia16-elf-objdump
SIZE := ia16-elf-size
NASM := nasm
NASMFLAGS += -g -F dwarf ${MACRO_DEFINITIONS}
CFLAGS += -O0 -g -march=i8088 -mcmodel=small -ffreestanding ${MACRO_DEFINITIONS}
PAGER := less
LDFLAGS += -g -ffreestanding -nostdlib -Wl,--no-gc-sections -mcmodel=small

DEPFILES := $(wildcard bin/*.d)

.PHONY: all
all: rom bin/rom.dbg.elf
	@echo Do not run this code on the breadboard version of this circuit unless you have made sure -DNO_PROBE=1 has been passed!

bin/entry.o: src/entry.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/io.o: src/io.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/mem.asm.o: src/mem.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/ivt.o: src/ivt.asm | bin
	$(NASM) $(NASMFLAGS) -MP -MD $(@:.o=.d) -felf32 $< -o $@
bin/mem.o: src/mem.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP -fno-tree-loop-distribute-patterns $< -o $@
bin/main.o: src/main.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
bin/pic.o: src/pic.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
bin/uart.o: src/uart.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
bin/except.o: src/except.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
bin/gdb.o: src/gdb.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
bin/probe.o: src/probe.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
bin/lcd.o: src/lcd.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@
bin/i8255.o: src/i8255.c | bin
	$(CC) -c $(CFLAGS) -MMD -MP $< -o $@

BINS = bin/entry.o bin/main.o bin/io.o bin/mem.o bin/mem.asm.o bin/pic.o bin/ivt.o bin/except.o bin/uart.o bin/gdb.o bin/probe.o bin/lcd.o bin/i8255.o
LIBS = -lgcc

bin/rom.elf: src/linker.ld ${BINS} | bin
	$(LD) -o$@ $(LDFLAGS) -Wl,--no-check-sections -T $^ ${LIBS}
bin/rom.dbg.elf: src/linker.dbg.ld ${BINS} | bin
	$(LD) -o$@ $(LDFLAGS) -Wl,--no-check-sections -Wl,-noinhibit-exec -T $^ ${LIBS} 2> /dev/null

rom: bin/rom.elf | bin
	@$(OBJCOPY) -O binary --gap-fill 0xff --pad-to 0x10000 bin/rom.elf rom
	@$(SIZE) -xB bin/rom.elf

.PHONY: size
size: bin/rom.elf
	@$(SIZE) -xB bin/rom.elf

bin:
	@mkdir -p bin

.PHONY: clean
clean:
	@rm bin/*
	@rm rom

.PHONY: disassemble
disassemble: bin/rom.elf
	@$(OBJDUMP) -S -d bin/rom.elf -M intel-mnemonic,i8086 | $(PAGER)

ifneq ($(DEPFILES),)
include $(DEPFILES)
endif
