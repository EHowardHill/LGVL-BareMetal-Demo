CC = gcc
AS = as
LD = ld

CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib \
         -mno-red-zone -fno-exceptions -Wall -Wextra -O2 -g -I.
ASFLAGS = --32
LDFLAGS = -m elf_i386 -nostdlib -T linker.ld
LIBGCC := $(shell $(CC) -m32 -print-libgcc-file-name)

# LVGL configuration
LVGL_DIR = lvgl
CFLAGS += -I$(LVGL_DIR) -DLV_CONF_INCLUDE_SIMPLE

# Kernel source files
KERNEL_SOURCES = kernel.c vbe.c keyboard.c lvgl_port.c stdlib.c
BOOT_ASM = boot.S

# Auto-discover LVGL source files (excluding examples, demos, tests, clib)
LVGL_SOURCES := $(shell find $(LVGL_DIR)/src -name '*.c' \
	! -path '*/examples/*' \
	! -path '*/demos/*' \
	! -path '*/tests/*' \
	! -path '*/libs/*' \
	! -path '*/stdlib/clib/*' \
	! -path '*/drivers/*' \
	! -name 'lv_demo*.c' \
	! -name 'lv_example*.c' \
	! -name 'lv_bin_decoder*.c')

# Debug: Show what files are being compiled
$(info LVGL sources found: $(words $(LVGL_SOURCES)) files)

OBJECTS = $(KERNEL_SOURCES:.c=.o) $(LVGL_SOURCES:.c=.o) boot.o

all: kernel.elf iso

boot.o: $(BOOT_ASM)
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^ -L$(dir $(LIBGCC)) -lgcc

iso: kernel.elf
	mkdir -p isodir/boot/grub
	cp kernel.elf isodir/boot/
	echo 'set timeout=0' > isodir/boot/grub/grub.cfg
	echo 'set default=0' >> isodir/boot/grub/grub.cfg
	echo 'menuentry "LVGL Kernel" {' >> isodir/boot/grub/grub.cfg
	echo '    multiboot /boot/kernel.elf' >> isodir/boot/grub/grub.cfg
	echo '    boot' >> isodir/boot/grub/grub.cfg
	echo '}' >> isodir/boot/grub/grub.cfg
	grub-mkrescue -o kernel.iso isodir

run: iso
	qemu-system-i386 -cdrom kernel.iso -vga std -m 128M

clean:
	find . -name '*.o' -delete
	rm -f lvgl/src/stdlib/clib/*.o
	rm -f kernel.elf kernel.iso
	rm -rf isodir

.PHONY: all iso run clean