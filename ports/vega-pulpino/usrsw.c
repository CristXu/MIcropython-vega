#include <stdio.h>
  
#include "py/nlr.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "usrsw.h"
#include "systick.h"

typedef enum {
	SW2 = 1,
	SW3,
	SW4,
	SW5,
} SW_NAME;

/* Define the init structure for the input sw*/
const port_pin_config_t port_sw_config = {
    kPORT_PullUp,                                            /* Internal pull-up resistor is enabled */
    kPORT_FastSlewRate,                                      /* Fast slew rate is configured */
    kPORT_PassiveFilterDisable,                              /* Passive filter is disabled */
    kPORT_OpenDrainDisable,                                  /* Open drain is disabled */
    kPORT_LowDriveStrength,                                  /* Low drive strength is configured */
    kPORT_MuxAsGpio,                                         /* Pin is configured as PTA0 */
    kPORT_UnlockRegister                                     /* Pin Control Register fields [15:0] are not locked */
  };

/* Define the init structure for the input switch pin */
gpio_pin_config_t sw_config = {
        kGPIO_DigitalInput, 0,
    };

//use pin-struct to do the gpio init:
void Init_SW_Pin(pin_obj_t *pin)
{
	CLOCK_EnableClock(kCLOCK_PortE); 
	PORT_SetPinConfig(pin->port, pin->pin, &port_sw_config);
	PORT_SetPinMux(pin->port, pin->pin, kPORT_MuxAsGpio);
    GPIO_PinInit(pin->gpio, pin->pin, &sw_config);
}  

static pin_obj_t pyb_pin_sw_obj[4] = {
	{.port = PORTA, .gpio = GPIOA, .pin = PIN0_IDX},
	{.port = PORTE, .gpio = GPIOE, .pin = PIN8_IDX}, 
	{.port = PORTE, .gpio = GPIOE, .pin = PIN9_IDX}, 
	{.port = PORTE, .gpio = GPIOE, .pin = PIN12_IDX},
};

pyb_sw_obj_t pyb_sw_obj[4] = {
	{.base = &pyb_sw_type, .sw_id = PYB_SW_0, .sw_pin = &pyb_pin_sw_obj[0]},
	{.base = &pyb_sw_type, .sw_id = PYB_SW_1, .sw_pin = &pyb_pin_sw_obj[1]},
	{.base = &pyb_sw_type, .sw_id = PYB_SW_2, .sw_pin = &pyb_pin_sw_obj[2]},
	{.base = &pyb_sw_type, .sw_id = PYB_SW_3, .sw_pin = &pyb_pin_sw_obj[3]},
};

mp_obj_t sw_value(mp_obj_t self) {
	pyb_sw_obj_t *s = (pyb_sw_obj_t*)self;
	const pin_obj_t *pPin = s->sw_pin;	
	uint32_t val = GPIO_ReadPinInput(pPin->gpio, pPin->pin);
	return mp_obj_new_bool(MICROPY_HW_USRSW_PRESSED == val);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sw_value_obj, sw_value);

mp_obj_t pyb_switch_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t* args){
	mp_arg_check_num(n_args, n_kw, 0, 0, false);
	return mp_obj_get_int(sw_value(self_in)) ? mp_const_true : mp_const_false;
}

void sw_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_sw_obj_t *self = self_in;
    mp_printf(print, "SW(%lu)", self->sw_id);
}


STATIC mp_obj_t sw_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    // get sw number
    mp_int_t sw_id = mp_obj_get_int(args[0]);

    // check sw number
    if (!(1 <= sw_id && sw_id <= NUM_SW)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "SW(%d) does not exist", sw_id));
    }
	Init_SW_Pin(pyb_sw_obj[sw_id - 1].sw_pin);
    // return static led object
    return (mp_obj_t)&pyb_sw_obj[sw_id - 1];
}
  
STATIC const mp_rom_map_elem_t sw_locals_dict_table[] = { 
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&sw_value_obj) },
	// const SW name
	{ MP_ROM_QSTR(MP_QSTR_SW3), MP_ROM_INT(SW3) },
	{ MP_ROM_QSTR(MP_QSTR_SW4), MP_ROM_INT(SW4) },
	{ MP_ROM_QSTR(MP_QSTR_SW5), MP_ROM_INT(SW5) },	
};
  
STATIC MP_DEFINE_CONST_DICT(sw_locals_dict, sw_locals_dict_table);
const mp_obj_type_t pyb_sw_type = {
    { &mp_type_type },
	.name = MP_QSTR_Switch,
	.call = pyb_switch_call,
	.print = sw_obj_print,
	.make_new = sw_obj_make_new,
	.locals_dict = (mp_obj_dict_t*)&sw_locals_dict,
};
