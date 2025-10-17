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