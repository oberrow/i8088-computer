# Makefile

# Copyright (c) 2026 Omar Berrow

CC=ia16-elf-gcc
LD := ia16-elf-ld
OBJCOPY := ia16-elf-objcopy
SIZE := ia16-elf-size
NASM := nasm
CFLAGS := -O2

all: rom

bin/entry.o: src/entry.asm | bin
	$(NASM) $(NASMFLAGS) -felf32 $< -o $@
bin/io.o: src/io.asm | bin
	$(NASM) $(NASMFLAGS) -felf32 $< -o $@
bin/main.o: src/main.c | bin
	$(CC) -c $(CFLAGS) -ffreestanding $< -o $@
bin/mem.o: src/mem.c | bin
	$(CC) -c $(CFLAGS) -ffreestanding -fno-tree-loop-distribute-patterns $< -o $@

bin/rom.elf: src/linker.ld bin/entry.o bin/main.o bin/io.o bin/mem.o | bin
	$(LD) -o$@ $(LDFLAGS) -T $^

rom: bin/rom.elf | bin
	$(OBJCOPY) -O binary --gap-fill 0xcc --pad-to 0x10000 bin/rom.elf rom
	$(SIZE) bin/rom.elf

bin:
	mkdir -p bin

clean:
	rm bin/*
