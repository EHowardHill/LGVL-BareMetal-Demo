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