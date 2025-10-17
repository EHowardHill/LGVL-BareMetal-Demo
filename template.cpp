boot.S:

.section .multiboot
.align 4

/* Multiboot header */
.long 0x1BADB002         /* magic */
.long 0x00000007         /* flags: ALIGN + MEMINFO + VIDEO */
.long -(0x1BADB002 + 0x00000007)  /* checksum */
.long 0, 0, 0, 0, 0      /* unused */
.long 0                  /* mode_type (0 = linear graphics) */
.long 640                /* width */
.long 480                /* height */
.long 32                 /* depth (32-bit color) */

.section .bss
.align 16
stack_bottom:
.skip 65536  /* 64 KiB stack - increased for LVGL */
stack_top:

.section .text
.global _start
.type _start, @function

_start:
    /* Set up stack */
    movl $stack_top, %esp
    
    /* Save multiboot info */
    pushl %ebx  /* multiboot info pointer */
    pushl %eax  /* multiboot magic */
    
    /* Call kernel main */
    call kernel_main
    
    /* Hang if kernel returns */
    cli
1:  hlt
    jmp 1b

.size _start, . - _start

.section .note.GNU-stack,"",@progbits

build.sh:

#!/bin/bash

make -j$(nproc)

inttypes.h:

#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>

/* Printf format macros */
#define PRId8  "d"
#define PRId16 "d"
#define PRId32 "d"
#define PRId64 "lld"

#define PRIu8  "u"
#define PRIu16 "u"
#define PRIu32 "u"
#define PRIu64 "llu"

#define PRIx8  "x"
#define PRIx16 "x"
#define PRIx32 "x"
#define PRIx64 "llx"

#define PRIX8  "X"
#define PRIX16 "X"
#define PRIX32 "X"
#define PRIX64 "llX"

#endif /* _INTTYPES_H */

kernel.c:

#include <stdint.h>
#include <stddef.h>
#include "vbe.h"
#include "keyboard.h"
#include "lvgl_port.h"
#include "lvgl/lvgl.h"

/* Simple delay function */
static void delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 1000; i++);
}

/* Create the UI based on your example */
static void create_ui(void) {
    lv_obj_t *list = lv_obj_create(lv_screen_active());
    lv_obj_set_size(list, 640, 480);
    lv_obj_center(list);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    
    static lv_style_t style_normal;
    lv_style_init(&style_normal);
    lv_style_set_bg_color(&style_normal, lv_palette_main(LV_PALETTE_BLUE_GREY));
    lv_style_set_radius(&style_normal, 0);
    
    static lv_style_t style_checked;
    lv_style_init(&style_checked);
    lv_style_set_bg_color(&style_checked, lv_palette_main(LV_PALETTE_ORANGE));
    
    static lv_style_t style_focus;
    lv_style_init(&style_focus);
    lv_style_set_outline_width(&style_focus, 3);
    lv_style_set_outline_color(&style_focus, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_outline_pad(&style_focus, 2);
    
    /* Get the default group for keyboard navigation */
    lv_group_t *group = lv_group_get_default();
    
    lv_obj_t *first_btn = NULL;
    
    for (int i = 0; i < 10; i++) {
        lv_obj_t *btn = lv_button_create(list);
        lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_style(btn, &style_normal, LV_STATE_DEFAULT);
        lv_obj_add_style(btn, &style_checked, LV_STATE_CHECKED);
        lv_obj_add_style(btn, &style_focus, LV_STATE_FOCUSED);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "List item %d", i);
        
        /* Add button to keyboard navigation group */
        lv_group_add_obj(group, btn);
        
        if (i == 0) {
            first_btn = btn;
        }
    }
    
    /* Focus the first button */
    if (first_btn && group) {
        lv_group_focus_obj(first_btn);
    }
}

void kernel_main(uint32_t magic, void *mboot_info) {
    (void)magic; /* Unused - could verify multiboot signature */
    
    /* Initialize VBE framebuffer */
    vbe_init(mboot_info);
    
    /* Clear screen to black */
    vbe_clear(0x000000);
    
    /* Initialize keyboard (polling mode - no interrupts) */
    keyboard_init();
    
    /* Initialize LVGL port */
    lvgl_port_init();
    
    /* Create the UI */
    create_ui();
    
    /* Main loop */
    uint32_t tick_counter = 0;
    while (1) {
        /* Poll keyboard every frame */
        keyboard_handler();
        
        /* Update LVGL tick and handle tasks */
        lvgl_port_tick();
        tick_counter++;
        
        /* Handle LVGL tasks frequently for responsive input */
        if (tick_counter >= 5) {
            lv_timer_handler();
            tick_counter = 0;
        }
        
        /* Small delay (~1ms) */
        delay(1);
    }
}

keyboard.c:

#include "keyboard.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Scancode to ASCII mapping (US keyboard, simplified) */
static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
    0, /* Right shift */
    '*',
    0, /* Alt */
    ' ', /* Space */
};

static key_event_t key_queue[KEY_QUEUE_SIZE];
static volatile int queue_head = 0;
static volatile int queue_tail = 0;
static int extended_scancode = 0;

static void enqueue_key(key_event_t key) {
    int next = (queue_head + 1) % KEY_QUEUE_SIZE;
    if (next != queue_tail) {
        key_queue[queue_head] = key;
        queue_head = next;
    }
}

void keyboard_init(void) {
    /* Simple polling-based keyboard - no interrupts */
    queue_head = 0;
    queue_tail = 0;
    extended_scancode = 0;
}

void keyboard_handler(void) {
    /* Poll keyboard in main loop instead of using interrupts */
    /* Check if data is available */
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (!(status & 0x01)) {
        return; /* No data available */
    }
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    /* Check for extended scancode prefix */
    if (scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }
    
    /* Ignore 0xE1 multi-byte sequences */
    if (scancode == 0xE1) {
        extended_scancode = 0;
        return;
    }
    
    key_event_t key;
    key.scancode = scancode & 0x7F;
    key.pressed = !(scancode & 0x80);
    key.ascii = 0;
    
    /* Handle extended scancodes (arrow keys) */
    if (extended_scancode) {
        extended_scancode = 0;
        
        /* Only process key presses */
        if (!key.pressed) {
            return;
        }
        
        /* Map extended scancodes to special keys */
        switch (key.scancode) {
            case 0x48: key.ascii = 1; break;  /* Up arrow */
            case 0x50: key.ascii = 2; break;  /* Down arrow */
            case 0x4B: key.ascii = 3; break;  /* Left arrow */
            case 0x4D: key.ascii = 4; break;  /* Right arrow */
            default: return;
        }
    } else {
        /* Regular scancode - only process presses */
        if (!key.pressed) {
            return;
        }
        
        if (key.scancode < 128) {
            key.ascii = scancode_to_ascii[key.scancode];
        }
    }
    
    /* Only enqueue valid keys */
    if (key.ascii != 0) {
        enqueue_key(key);
    }
}

int keyboard_has_key(void) {
    return queue_head != queue_tail;
}

key_event_t keyboard_get_key(void) {
    key_event_t key = {0, 0, 0};
    
    if (queue_head != queue_tail) {
        key = key_queue[queue_tail];
        queue_tail = (queue_tail + 1) % KEY_QUEUE_SIZE;
    }
    
    return key;
}

keyboard.h:

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEY_QUEUE_SIZE 32

typedef struct {
    uint8_t scancode;
    uint8_t pressed;  /* 1 = pressed, 0 = released */
    char ascii;
} key_event_t;

void keyboard_init(void);
void keyboard_handler(void);
int keyboard_has_key(void);
key_event_t keyboard_get_key(void);

#endif

linker.ld:

ENTRY(_start)

SECTIONS
{
    . = 1M;

    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
}

lvgl_port.c:

#include "lvgl_port.h"
#include "vbe.h"
#include "keyboard.h"

static lv_display_t *disp;
static lv_indev_t *indev;

/* Display flush callback */
static void disp_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    vbe_info_t *vbe = vbe_get_info();
    
    int32_t x, y;
    uint32_t *fb_ptr;
    uint32_t *color_ptr = (uint32_t *)px_map;
    
    /* Bounds checking to prevent crashes */
    int32_t y1 = (area->y1 < 0) ? 0 : area->y1;
    int32_t y2 = (area->y2 >= (int32_t)vbe->height) ? (int32_t)vbe->height - 1 : area->y2;
    int32_t x1 = (area->x1 < 0) ? 0 : area->x1;
    int32_t x2 = (area->x2 >= (int32_t)vbe->width) ? (int32_t)vbe->width - 1 : area->x2;
    
    for (y = y1; y <= y2; y++) {
        fb_ptr = vbe->framebuffer + y * (vbe->pitch / 4) + x1;
        for (x = x1; x <= x2; x++) {
            /* LVGL 9.x uses 32-bit ARGB format directly */
            *fb_ptr++ = *color_ptr++;
        }
    }
    
    lv_display_flush_ready(display);
}

/* Keyboard input read callback */
static void keyboard_read_cb(lv_indev_t *indev_drv, lv_indev_data_t *data) {
    (void)indev_drv; /* Unused parameter */
    
    static uint32_t last_key = 0;
    static int press_sent = 0;
    
    /* Safety check */
    if (!data) {
        return;
    }
    
    if (keyboard_has_key() && !press_sent) {
        /* Get the key but don't dequeue yet */
        key_event_t key = keyboard_get_key();
        
        if (key.ascii != 0) {
            /* Map common keys to LVGL keys */
            switch (key.ascii) {
                case '\n':
                    last_key = LV_KEY_ENTER;
                    break;
                case '\t':
                    last_key = LV_KEY_NEXT;
                    break;
                case '\b':
                    last_key = LV_KEY_BACKSPACE;
                    break;
                case 27: /* ESC */
                    last_key = LV_KEY_ESC;
                    break;
                case 1: /* Up arrow */
                    last_key = LV_KEY_UP;
                    break;
                case 2: /* Down arrow */
                    last_key = LV_KEY_DOWN;
                    break;
                case 3: /* Left arrow */
                    last_key = LV_KEY_LEFT;
                    break;
                case 4: /* Right arrow */
                    last_key = LV_KEY_RIGHT;
                    break;
                default:
                    if (key.ascii >= 32 && key.ascii <= 126) {
                        last_key = key.ascii;
                    } else {
                        return; /* Invalid key */
                    }
                    break;
            }
            
            /* Report key press */
            data->key = last_key;
            data->state = LV_INDEV_STATE_PRESSED;
            press_sent = 1;
            data->continue_reading = false;
        }
    } else if (press_sent) {
        /* Send release event for the last key */
        data->key = last_key;
        data->state = LV_INDEV_STATE_RELEASED;
        press_sent = 0;
        data->continue_reading = false;
    } else {
        /* No key activity */
        data->state = LV_INDEV_STATE_RELEASED;
        data->continue_reading = false;
    }
}

void lvgl_port_init(void) {
    vbe_info_t *vbe = vbe_get_info();
    
    /* Initialize LVGL */
    lv_init();
    
    /* Allocate display buffer in BSS (static) - 1/20 screen size for safety */
    static lv_color_t buf1[640 * 480 / 20];
    
    disp = lv_display_create(vbe->width, vbe->height);
    lv_display_set_flush_cb(disp, disp_flush_cb);
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    /* Set color format to match our 32-bit framebuffer */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_ARGB8888);
    
    /* Create keyboard input device */
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, keyboard_read_cb);
    
    /* Create a default group for keyboard navigation */
    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(indev, group);
    
    /* Enable wrapping navigation */
    lv_group_set_wrap(group, true);
}

void lvgl_port_tick(void) {
    /* Increment tick by 1ms */
    lv_tick_inc(1);
}

lvgl_port.h:

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "lvgl/lvgl.h"

void lvgl_port_init(void);
void lvgl_port_tick(void);

#endif

lv_conf.h:

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Prevent LVGL from using inttypes.h */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB  
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB
#define LV_STDINT_INCLUDE <stdint.h>

/* Color settings */
#define LV_COLOR_DEPTH 32
#define LV_COLOR_16_SWAP 0
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_MIX_ROUND_OFS 0

/* Memory settings */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (96 * 1024U)
#define LV_MEM_ADR 0
#define LV_MEM_BUF_MAX_NUM 16

/* HAL settings */
#define LV_TICK_CUSTOM 0
#define LV_DPI_DEF 100

/* Drawing settings */
#define LV_DRAW_SW_SUPPORT_RGB565 0
#define LV_DRAW_SW_SUPPORT_RGB888 1

/* Binary decoder (disable for bare metal) */
#define LV_USE_BIN_DECODER 0
#define LV_BIN_DECODER_RAM_LOAD 0

/* Operating system and others */
#define LV_USE_OS LV_OS_NONE
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/* Font settings */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Widget usage */
#define LV_USE_BUTTON 1
#define LV_USE_LABEL 1

/* Layout usage */
#define LV_USE_FLEX 1
#define LV_USE_GRID 0

/* Themes */
#define LV_USE_THEME_DEFAULT 1

/* Logging */
#define LV_USE_LOG 0

/* Others */
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0
#define LV_USE_ASSERT_STYLE 0

#define LV_SPRINTF_CUSTOM 0
#define LV_SPRINTF_USE_FLOAT 0

#endif

Makefile:

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

README.md:

# LGVL-BareMetal-Demo



setup.sh:

#!/bin/bash

sudo apt update
sudo apt install build-essential nasm grub-pc-bin xorriso qemu-system-x86 git gcc-multilib g++-multilib

git clone --depth 1 --branch release/v9.2 https://github.com/lvgl/lvgl.git

stdlib.c:

#include <stddef.h>
#include <stdint.h>

/* Memory management for LVGL */
static uint8_t heap[96 * 1024] __attribute__((aligned(8)));
static size_t heap_index = 0;

void* malloc(size_t size) {
    /* Simple bump allocator - align to 8 bytes */
    size = (size + 7) & ~7;
    
    if (heap_index + size > sizeof(heap)) {
        return NULL;
    }
    
    void* ptr = &heap[heap_index];
    heap_index += size;
    return ptr;
}

void free(void* ptr) {
    /* No-op for simple bump allocator */
    (void)ptr;
}

void* realloc(void* ptr, size_t size) {
    /* Simple implementation - just allocate new */
    if (ptr == NULL) {
        return malloc(size);
    }
    
    void* new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    /* Copy old data - we don't track old size, so just copy size bytes */
    uint8_t* dst = new_ptr;
    uint8_t* src = ptr;
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }
    
    return new_ptr;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    
    if (ptr != NULL) {
        uint8_t* p = ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    
    return ptr;
}

/* String functions */
void* memset(void* s, int c, size_t n) {
    uint8_t* p = s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    
    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = s1;
    const uint8_t* p2 = s2;
    
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    size_t i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || !s1[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}

char* strcat(char* dest, const char* src) {
    size_t dest_len = strlen(dest);
    size_t i;
    
    for (i = 0; src[i]; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i;
    
    for (i = 0; i < n && src[i]; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    
    return dest;
}

/* Simple snprintf implementation for LVGL */
static void int_to_str(int value, char* buf, int* pos) {
    if (value < 0) {
        buf[(*pos)++] = '-';
        value = -value;
    }
    
    if (value == 0) {
        buf[(*pos)++] = '0';
        return;
    }
    
    char temp[12];
    int temp_pos = 0;
    
    while (value > 0) {
        temp[temp_pos++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (temp_pos > 0) {
        buf[(*pos)++] = temp[--temp_pos];
    }
}

int vsnprintf(char* buf, size_t size, const char* format, __builtin_va_list args) {
    size_t pos = 0;
    
    while (*format && pos < size - 1) {
        if (*format == '%') {
            format++;
            if (*format == 'd') {
                int value = __builtin_va_arg(args, int);
                int temp_pos = 0;
                char temp[32];
                int_to_str(value, temp, &temp_pos);
                for (int i = 0; i < temp_pos && pos < size - 1; i++) {
                    buf[pos++] = temp[i];
                }
            } else if (*format == 's') {
                char* str = __builtin_va_arg(args, char*);
                while (*str && pos < size - 1) {
                    buf[pos++] = *str++;
                }
            } else if (*format == 'c') {
                char c = (char)__builtin_va_arg(args, int);
                buf[pos++] = c;
            } else if (*format == '%') {
                buf[pos++] = '%';
            }
            format++;
        } else {
            buf[pos++] = *format++;
        }
    }
    
    buf[pos] = '\0';
    return pos;
}

int snprintf(char* buf, size_t size, const char* format, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    __builtin_va_end(args);
    return result;
}

/* GCC builtin support for 64-bit division */
long long __divdi3(long long a, long long b) {
    int neg = 0;
    if (a < 0) { a = -a; neg = !neg; }
    if (b < 0) { b = -b; neg = !neg; }
    
    long long quot = 0;
    long long rem = 0;
    
    for (int i = 63; i >= 0; i--) {
        rem = (rem << 1) | ((a >> i) & 1);
        if (rem >= b) {
            rem -= b;
            quot |= (1LL << i);
        }
    }
    
    return neg ? -quot : quot;
}

/* LVGL wrapper functions - map lv_* to our implementations */
void* lv_memcpy(void* dest, const void* src, size_t n) {
    return memcpy(dest, src, n);
}

void* lv_memset(void* s, int c, size_t n) {
    return memset(s, c, n);
}

void* lv_memmove(void* dest, const void* src, size_t n) {
    return memmove(dest, src, n);
}

int lv_memcmp(const void* s1, const void* s2, size_t n) {
    return memcmp(s1, s2, n);
}

size_t lv_strlen(const char* s) {
    return strlen(s);
}

char* lv_strcpy(char* dest, const char* src) {
    return strcpy(dest, src);
}

char* lv_strncpy(char* dest, const char* src, size_t n) {
    return strncpy(dest, src, n);
}

int lv_strcmp(const char* s1, const char* s2) {
    return strcmp(s1, s2);
}

int lv_strncmp(const char* s1, const char* s2, size_t n) {
    return strncmp(s1, s2, n);
}

char* lv_strcat(char* dest, const char* src) {
    return strcat(dest, src);
}

char* lv_strncat(char* dest, const char* src, size_t n) {
    return strncat(dest, src, n);
}

char* lv_strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

size_t lv_strlcpy(char* dst, const char* src, size_t size) {
    size_t src_len = strlen(src);
    if (size > 0) {
        size_t copy_len = (src_len < size - 1) ? src_len : size - 1;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }
    return src_len;
}

int lv_snprintf(char* buf, size_t size, const char* format, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    __builtin_va_end(args);
    return result;
}

int lv_vsnprintf(char* buf, size_t size, const char* format, __builtin_va_list args) {
    return vsnprintf(buf, size, format, args);
}

/* LVGL memory management wrappers */
void* lv_malloc_core(size_t size) {
    return malloc(size);
}

void lv_free_core(void* ptr) {
    free(ptr);
}

void* lv_realloc_core(void* ptr, size_t size) {
    return realloc(ptr, size);
}

void lv_mem_init(void) {
    /* Our bump allocator doesn't need initialization */
}

void lv_mem_deinit(void) {
    /* Our bump allocator doesn't need deinitialization */
}

void lv_mem_monitor_core(void* mon) {
    /* Not implemented for simple allocator */
    (void)mon;
}

int lv_mem_test_core(void) {
    /* Simple allocator always passes */
    return 1;
}

/* Stub for binary decoder (disabled in config) */
void lv_bin_decoder_init(void) {
    /* Binary decoder is disabled */
}

vbe.c:

#include "vbe.h"

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

static vbe_info_t vbe_info;

void vbe_init(void* mboot_info) {
    multiboot_info_t* mb_info = (multiboot_info_t*)mboot_info;
    
    vbe_info.framebuffer = (uint32_t*)(uintptr_t)mb_info->framebuffer_addr;
    vbe_info.width = mb_info->framebuffer_width;
    vbe_info.height = mb_info->framebuffer_height;
    vbe_info.pitch = mb_info->framebuffer_pitch;
    vbe_info.bpp = mb_info->framebuffer_bpp;
}

vbe_info_t* vbe_get_info(void) {
    return &vbe_info;
}

void vbe_put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)vbe_info.width || y < 0 || y >= (int)vbe_info.height)
        return;
    
    uint32_t* pixel = vbe_info.framebuffer + y * (vbe_info.pitch / 4) + x;
    *pixel = color;
}

void vbe_clear(uint32_t color) {
    for (uint32_t y = 0; y < vbe_info.height; y++) {
        for (uint32_t x = 0; x < vbe_info.width; x++) {
            vbe_put_pixel(x, y, color);
        }
    }
}

vbe.h:

#ifndef VBE_H
#define VBE_H

#include <stdint.h>

typedef struct {
    uint32_t* framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
} vbe_info_t;

void vbe_init(void* mboot_info);
vbe_info_t* vbe_get_info(void);
void vbe_put_pixel(int x, int y, uint32_t color);
void vbe_clear(uint32_t color);

#endif

