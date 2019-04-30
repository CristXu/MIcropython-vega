#include "py/obj.h"
#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_gpio.h"

#define PIN0_IDX 0u
#define PIN8_IDX 8u
#define PIN9_IDX 9u
#define PIN12_IDX 12u

#define NUM_SW 4

#define MICROPY_HW_USRSW_PRESSED (0)
typedef enum {
	PYB_SW_0 = 1,
	PYB_SW_1,
	PYB_SW_2,
	PYB_SW_3,
} pyb_sw_t;

typedef struct _pyb_sw_obj_t{
	mp_obj_base_t base;
	pyb_sw_t sw_id;
	pin_obj_t *sw_pin;
} pyb_sw_obj_t;

extern const mp_obj_type_t pyb_sw_type;
