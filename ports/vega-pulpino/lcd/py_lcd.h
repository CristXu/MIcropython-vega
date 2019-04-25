#ifndef _PY_LCD_H_
#define _PY_LCD_H_

typedef struct _pyb_lcd_obj_t{
	mp_obj_base_t base;
	uint32_t font;
} pyb_lcd_obj_t;
extern const mp_obj_type_t pyb_lcd_type;


#endif

