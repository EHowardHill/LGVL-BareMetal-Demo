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
#define LV_MEM_CUSTOM 1 /* Changed from 0 to 1 */
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
/* LV_USE_STDLIB_... already defined at top */

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