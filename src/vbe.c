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