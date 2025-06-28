/**
 * @file lv_ex_conf.h
 *
 */

#ifndef LV_EX_CONF_H
#define LV_EX_CONF_H


/*******************
 * GENERAL SETTING
 *******************/
#define LV_EX_PRINTF       1       /*Enable printf-ing data*/
#define LV_EX_KEYBOARD     1       /*Add PC keyboard support to some examples (`lv_drivers` repository is required)*/
#define LV_EX_MOUSEWHEEL   1       /*Add 'encoder' (mouse wheel) support to some examples (`lv_drivers` repository is required)*/


#define LV_FONT_CUSTOM_DECLARE extern lv_font_t *lv_ttf_font;

#define LV_USE_FS_STDIO        1
#define LV_FS_STDIO_LETTER     'S'
#define LV_FS_STDIO_CACHE_SIZE 4096

#define LV_USE_TINY_TTF 1
#define CONFIG_LV_TINY_TTF_FILE_SUPPORT 1

/*******************
 *   TEST USAGE
 *******************/
#define LV_USE_TESTS       1

/*******************
 * TUTORIAL USAGE
 *******************/
#define LV_USE_TUTORIALS   1

/*********************
 * APPLICATION USAGE
 *********************/

/* Test the graphical performance of your MCU
 * with different settings*/
#define LV_USE_BENCHMARK   1

/*A demo application with Keyboard, Text area, List and Chart
 * placed on Tab view */
#define LV_USE_DEMO        1
#if LV_USE_DEMO
#define LV_DEMO_WALLPAPER  1    /*Create a wallpaper too*/
#define LV_DEMO_SLIDE_SHOW 0    /*Automatically switch between tabs*/
#endif

/*MCU and memory usage monitoring*/
#define LV_USE_SYSMON      1

/*A terminal to display received characters*/
#define LV_USE_TERMINAL    1

/*Touch pad calibration with 4 points*/
#define LV_USE_TPCAL       1

#undef LV_FONT_MONTSERRAT_12
#undef LV_FONT_MONTSERRAT_14
#undef LV_FONT_MONTSERRAT_16
#undef LV_FONT_MONTSERRAT_18
#undef LV_FONT_MONTSERRAT_20
#undef LV_FONT_MONTSERRAT_22
#undef LV_FONT_MONTSERRAT_24
#undef LV_FONT_MONTSERRAT_26
#undef LV_FONT_MONTSERRAT_28
#undef LV_FONT_MONTSERRAT_30
#undef LV_FONT_MONTSERRAT_32

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1


#endif /*LV_EX_CONF_H*/

