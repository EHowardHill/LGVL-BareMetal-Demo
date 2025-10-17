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