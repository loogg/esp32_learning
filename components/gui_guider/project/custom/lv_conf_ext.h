/*
* Copyright 2023 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/*
 * lv_conf_ext.h for custom lvconf file.
 * Created on: Feb 8, 2023
 * example :
 *	#undef LV_FONT_FMT_TXT_LARGE
 *  #define LV_FONT_FMT_TXT_LARGE 1
 */

#ifndef LV_CONF_EXT_H
#define LV_CONF_EXT_H


/* common code  begin  */


/* common code end */


#if LV_USE_GUIDER_SIMULATOR
/* code for simulator begin  */

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

/* code for simulator end */
#else
/* code for board begin */


/* code for board end */
#endif



#endif  /* LV_CONF_EXT_H */