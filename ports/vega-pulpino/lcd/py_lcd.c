#define _PY_lcd_C_
#include <stdio.h>
#include <string.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "bufhelper.h"
#include "lcd.h"
#include "py_lcd.h"

pyb_lcd_obj_t pyb_lcd_obj = { .base = {&pyb_lcd_type}, .font = LCD_FONT_1206};

void pyb_lcd_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_lcd_obj_t *self = self_in;
	if (self->font == LCD_FONT_1206) {
    	mp_printf(print, "LCD 320x240, FONT_1206");
	} else if (self->font == LCD_FONT_1608) {
		mp_printf(print, "LCD 320x240, FONT_1608");
	}
}

STATIC mp_obj_t pyb_lcd_init_helper(pyb_lcd_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_font,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = LCD_FONT_1608} },
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 3000000} },
		// <<<
    };
	
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	if ((args[0].u_int == LCD_FONT_1608) || (args[0].u_int == LCD_FONT_1206)){
		self->font = args[0].u_int;
		lcd_init(args[1].u_int);
	}else 
		nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
				"Font does not support!"));
    return mp_const_none;
}

STATIC mp_obj_t pyb_lcd_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) 
{
	// the make_new receipt the number 1 bigger than the init_helper.... and send to init less than the given value-number, so we need to send all the args to the init_helper, if we no need to speciffy the lcd channel given by the args[0];	
    // start the peripheral
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    pyb_lcd_init_helper(&pyb_lcd_obj, n_args , args, &kw_args);

    return (mp_obj_t)&pyb_lcd_obj;	
}

STATIC mp_obj_t pyb_lcd_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return pyb_lcd_init_helper(args[0], n_args - 1, args + 1, kw_args);
}


STATIC mp_obj_t pyb_lcd_deinit(mp_obj_t self_in) {	
	LPSPI_Deinit(LPSPI0);
    return mp_const_none;
}

STATIC mp_obj_t pyb_lcd_clear_screen(size_t n_args, const mp_obj_t *args) {
	uint16_t col = LCD_COLOR_BLACK;
	if (n_args == 2)
		col = mp_obj_get_int(args[1]);
	lcd_clear_screen(col);
	return mp_const_none;
}

STATIC mp_obj_t pyb_lcd_clear_block(size_t n_args, const mp_obj_t *args) {
	uint16_t col = LCD_COLOR_BLACK;
	mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
	if (n_args == 4)
		col = mp_obj_get_int(args[3]);
	lcd_clear_block(x, y, col);
	return mp_const_none;
}

/// \method lcd_clear_const_block([x, y, [color, [width, height]]])
/// CLear a part of lcd with the given color and [width, height]
STATIC mp_obj_t pyb_lcd_clear_const_block(size_t n_args, const mp_obj_t *args) {
	uint32_t w = 240;
	uint32_t h = 32;
	uint16_t col = LCD_COLOR_BLACK;
	mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
	if (n_args > 3)
		col = mp_obj_get_int(args[3]);
	if ((n_args > 4)){
		if ( n_args == 6){
			mp_int_t w = mp_obj_get_int(args[4]);
			mp_int_t h = mp_obj_get_int(args[5]);
			lcd_clear_const_block(x, y, w, h, col);
			return mp_const_none;
		} else
			nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
				"Please input both the [ Block_Width, Block_Heigth ]"));
	}
	lcd_clear_const_block(x, y, w, h, col);
	return mp_const_none;
}

STATIC mp_obj_t pyb_lcd_set_bkg_color(size_t n_args, const mp_obj_t *args) {
	uint16_t col = mp_obj_get_int(args[1]);
	lcd_clear_screen(col);
	return mp_const_none;
}

STATIC mp_obj_t pyb_lcd_put_text_xy(size_t n_args, const mp_obj_t *args) {
	pyb_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	const char *str = mp_obj_str_get_str(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
	uint16_t col = LCD_COLOR_WHITE;
	if (n_args>4)
		col = mp_obj_get_int(args[4]);
	lcd_display_string(x, y, str, self->font,col);
	return mp_const_none;
}

STATIC mp_obj_t pyb_lcd_put_text_block_xy(size_t n_args, const mp_obj_t *args) {
	pyb_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	const char *str = mp_obj_str_get_str(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
	uint16_t col = LCD_COLOR_WHITE;
	if (n_args>4)
		col = mp_obj_get_int(args[4]);
	lcd_display_block_string(x, y, str, self->font,col);
	return mp_const_none;
}


STATIC mp_obj_t pyb_lcd_put_char_xy(size_t n_args, const mp_obj_t *args) {
	pyb_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	const char *str = mp_obj_str_get_str(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
	char c = str[0];
	uint16_t col = LCD_COLOR_WHITE;
	if (n_args>4)
		col = mp_obj_get_int(args[4]);
	lcd_display_char(x, y, c, self->font, col);
	return mp_const_none;
}

STATIC mp_obj_t pyb_lcd_set_font(size_t n_args, const mp_obj_t *args) {
	pyb_lcd_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	if ( n_args == 1){
		if (self->font == LCD_FONT_1206) {
			return mp_obj_new_str("FONT_1206", 9);
		} else if (self->font == LCD_FONT_1608) {
			return mp_obj_new_str("FONT_1608", 9);
		}
	}
	else{
		mp_int_t fntNdx = mp_obj_get_int(args[1]);
		if ((fntNdx == LCD_FONT_1206) || (fntNdx == LCD_FONT_1608))
			self->font = fntNdx;
		else {
			nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
				"invalid font %d !", fntNdx));
		}
	}
	return mp_const_none;
}

// -----------------------------------------------------------------------------------------
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_lcd_init_obj, 1, pyb_lcd_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_lcd_deinit_obj, pyb_lcd_deinit);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_clear_screen_obj, 1, 2, pyb_lcd_clear_screen); 
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_clear_block_obj, 3, 4, pyb_lcd_clear_block);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_clear_const_block_obj, 3, 6, pyb_lcd_clear_const_block);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_set_bkg_color_obj, 2, 2, pyb_lcd_set_bkg_color); 
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_put_text_xy_obj, 4, 5, pyb_lcd_put_text_xy);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_put_text_block_xy_obj, 4, 5, pyb_lcd_put_text_block_xy);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_put_char_xy_obj, 4, 5, pyb_lcd_put_char_xy);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_lcd_set_font_obj, 1, 2, pyb_lcd_set_font);

STATIC const mp_rom_map_elem_t lcd_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_lcd_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pyb_lcd_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_clear_screen), MP_ROM_PTR(&pyb_lcd_clear_screen_obj) },
	{ MP_ROM_QSTR(MP_QSTR_clear_block), MP_ROM_PTR(&pyb_lcd_clear_block_obj) },
	{ MP_ROM_QSTR(MP_QSTR_clear_const_block), MP_ROM_PTR(&pyb_lcd_clear_const_block_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_bkg_color), MP_ROM_PTR(&pyb_lcd_set_bkg_color_obj) },

	{ MP_ROM_QSTR(MP_QSTR_put_text_xy), MP_ROM_PTR(&pyb_lcd_put_text_xy_obj) },
	{ MP_ROM_QSTR(MP_QSTR_put_text_block_xy), MP_ROM_PTR(&pyb_lcd_put_text_block_xy_obj) },
	{ MP_ROM_QSTR(MP_QSTR_put_char_xy), MP_ROM_PTR(&pyb_lcd_put_char_xy_obj) },
	{ MP_ROM_QSTR(MP_QSTR_set_font), MP_ROM_PTR(&pyb_lcd_set_font_obj) },

    // Const value
	{ MP_ROM_QSTR(MP_QSTR_FONT_1206), MP_ROM_INT(LCD_FONT_1206) },
	{ MP_ROM_QSTR(MP_QSTR_FONT_1608), MP_ROM_INT(LCD_FONT_1608) },
	{ MP_ROM_QSTR(MP_QSTR_WHITE), MP_ROM_INT(LCD_COLOR_WHITE) },
	{ MP_ROM_QSTR(MP_QSTR_BLACK), MP_ROM_INT(LCD_COLOR_BLACK ) },
	{ MP_ROM_QSTR(MP_QSTR_BLUE), MP_ROM_INT(LCD_COLOR_BLUE) },
	{ MP_ROM_QSTR(MP_QSTR_BRED), MP_ROM_INT(LCD_COLOR_BRED) },
	{ MP_ROM_QSTR(MP_QSTR_GRED), MP_ROM_INT(LCD_COLOR_GRED) },
	{ MP_ROM_QSTR(MP_QSTR_GBLUE), MP_ROM_INT(LCD_COLOR_GBLUE) },
	{ MP_ROM_QSTR(MP_QSTR_RED), MP_ROM_INT(LCD_COLOR_RED) },
	{ MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_ROM_INT(LCD_COLOR_MAGENTA) },
	{ MP_ROM_QSTR(MP_QSTR_GREEN), MP_ROM_INT(LCD_COLOR_GREEN) },
	{ MP_ROM_QSTR(MP_QSTR_CYAN), MP_ROM_INT(LCD_COLOR_CYAN) },
	{ MP_ROM_QSTR(MP_QSTR_YELLOW), MP_ROM_INT(LCD_COLOR_YELLOW) },
	{ MP_ROM_QSTR(MP_QSTR_BROWN), MP_ROM_INT(LCD_COLOR_BROWN) },
	{ MP_ROM_QSTR(MP_QSTR_BRRED), MP_ROM_INT(LCD_COLOR_BRRED) },
	{ MP_ROM_QSTR(MP_QSTR_GRAY), MP_ROM_INT(LCD_COLOR_GRAY) },

};

STATIC MP_DEFINE_CONST_DICT(lcd_locals_dict, lcd_locals_dict_table);

const mp_obj_type_t pyb_lcd_type = {
    { &mp_type_type },
    .name = MP_QSTR_LCD,
    .print = pyb_lcd_print,
    .make_new = pyb_lcd_make_new,
    .locals_dict = (mp_obj_dict_t*)&lcd_locals_dict,
};

