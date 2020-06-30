#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/stackctrl.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "mpconfigport.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "sdcard.h"
#include "storage.h"
#include "gccollect.h"
#include "pin_mux.h"
#include "clock_config.h"

#define INIT_LED_PIN_WITH_FUNC_PTR (0)
#define ENABLE_GC_OUTPUT		   (0)

void Systick_Init(void)
{   
    CLOCK_SetIpSrc(kCLOCK_Lpit0, kCLOCK_IpSrcFircAsync);//stupid error: miss this line, the lpit0 without a src-clock
	SystemSetupSystick(1000,7);
	SystemClearSystickFlag();
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    int32_t c = 0;
    while (c<=0) {
        c = GETCHAR();
    }
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        PUTCHAR(*str++);
    }
}
   
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
static const char fresh_boot_py[] =
"# boot.py -- run on boot-up\r\n"
;

static const char fresh_main_py[] =
"# main.py -- put your code here!\n"
;

static const char fresh_readme_txt[] =
"This is a MicroPython board\r\n"
;

// avoid inlining to avoid stack usage within main()
// all because the MBR block for the fatfs, the 0x0b, block size is not compatible
// and also set vfs->fatfs.part = 0 instead 1; // flash filesystem lives on first partition
fs_user_mount_t fs_user_mount_flash;
MP_NOINLINE STATIC bool init_flash_fs(uint reset_mode) {
    // init the vfs object
    fs_user_mount_t *vfs_fat = &fs_user_mount_flash;
    vfs_fat->flags = 0;
    pyb_flash_init_vfs(vfs_fat);
    // try to mount the flash
    FRESULT res = f_mount(&vfs_fat->fatfs);

    if (reset_mode == 3 || res == FR_NO_FILESYSTEM) {
        // no filesystem, or asked to reset it, so create a fresh one

        uint32_t start_tick = HAL_GetTick();

        uint8_t working_buf[_MAX_SS];
        res = f_mkfs(&vfs_fat->fatfs, FM_FAT, 0, working_buf, sizeof(working_buf));
        if (res == FR_OK) {
            // success creating fresh LFS
        } else {
            printf("PYB: can't create flash filesystem\n");
            return false;
        }

        // set label
        f_setlabel(&vfs_fat->fatfs, "pybflash");

        // create empty main.py
        FIL fp;
        //f_open(&vfs_fat->fatfs, &fp, "/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        //f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        // TODO check we could write n bytes
        //f_close(&fp);

        // create readme file
        f_open(&vfs_fat->fatfs, &fp, "/README.txt", FA_WRITE | FA_CREATE_ALWAYS);
        f_write(&fp, fresh_readme_txt, sizeof(fresh_readme_txt) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    } else if (res == FR_OK) {
        // mount sucessful
    } else {
    fail:
        printf("PYB: can't mount flash\n");
        return false;
    }

    // mount the flash device (there should be no other devices mounted at this point)
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
    if (vfs == NULL) {
        goto fail;
    }
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;

    // The current directory is used as the boot up directory.
    // It is set to the internal flash filesystem by default.
    MP_STATE_PORT(vfs_cur) = vfs;

    // Make sure we have a /flash/boot.py.  Create it if needed.
    FILINFO fno;
    res = f_stat(&vfs_fat->fatfs, "/boot.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file

        uint32_t start_tick = HAL_GetTick();

        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
        // TODO check we could write n bytes
        f_close(&fp);
    }

    return true;
}
#if MICROPY_HW_HAS_SDCARD
STATIC bool init_sdcard_fs(void) {
    bool first_part = true;
    for (int part_num = 1; part_num <= 4; ++part_num) {
        // create vfs object
        fs_user_mount_t *vfs_fat = m_new_obj_maybe(fs_user_mount_t);
        mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
        if (vfs == NULL || vfs_fat == NULL) {
            break;
        }
        vfs_fat->flags = FSUSER_FREE_OBJ;
        sdcard_init_vfs(vfs_fat, part_num);

        // try to mount the partition
        FRESULT res = f_mount(&vfs_fat->fatfs);

        if (res != FR_OK) {
            // couldn't mount
            m_del_obj(fs_user_mount_t, vfs_fat);
            m_del_obj(mp_vfs_mount_t, vfs);
        } else {
            // mounted via FatFs, now mount the SD partition in the VFS
            if (first_part) {
                // the first available partition is traditionally called "sd" for simplicity
                vfs->str = "/sd";
                vfs->len = 3;
            } else {
                // subsequent partitions are numbered by their index in the partition table
                if (part_num == 2) {
                    vfs->str = "/sd2";
                } else if (part_num == 2) {
                    vfs->str = "/sd3";
                } else {
                    vfs->str = "/sd4";
                }
                vfs->len = 4;
            }
            vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
            vfs->next = NULL;
            for (mp_vfs_mount_t **m = &MP_STATE_VM(vfs_mount_table);; m = &(*m)->next) {
                if (*m == NULL) {
                    *m = vfs;
                    break;
                }
            }

            #if MICROPY_HW_ENABLE_USB
            if (pyb_usb_storage_medium == PYB_USB_STORAGE_MEDIUM_NONE) {
                // if no USB MSC medium is selected then use the SD card
                pyb_usb_storage_medium = PYB_USB_STORAGE_MEDIUM_SDCARD;
            }
            #endif

            #if MICROPY_HW_ENABLE_USB
            // only use SD card as current directory if that's what the USB medium is
            if (pyb_usb_storage_medium == PYB_USB_STORAGE_MEDIUM_SDCARD)
            #endif
            {
                if (first_part) {
                    // use SD card as current directory
                    MP_STATE_PORT(vfs_cur) = vfs;
                }
            }
            first_part = false;
        }
    }

    if (first_part) {
        printf("PYB: can't mount SD card\n");
        return false;
    } else {
        return true;
    }
}
#endif

#include "lcd.h"
#include "icon.h"
int main(int argc, char **argv) {
	uint32_t retCode;
    // Initialization block taken from led_fade.c
    {
        BOARD_InitPins();
        BOARD_BootClockRUN();
        BOARD_InitDebugConsole();
		Systick_Init(); // init the systick timer
    }

	block_t icon_main;
	bool first_soft_reset = true;
soft_reset:    
    //note: the value from *.ld is the value store in the address of the ram, such as _estack is the value store in the real address in ram, so should use & to get the real address
#if INIT_LED_PIN_WITH_FUNC_PTR
	led_init0();
#endif
#if MICROPY_HW_DISPLAY_ICON
	icon_main.w = big_w;
	icon_main.h = big_h;
	icon_main.data = big;
	lcd_init(24000000);
	lcd_paint_block((240-icon_main.w)/2, (320-icon_main.h)/2, &icon_main);
#endif
    mp_stack_set_top(&_estack);
    mp_stack_set_limit(&_stack_size);
    gc_init(&_heap_start, &_heap_end);
    mp_init();
    bool mounted_flash = false;
#if MICROPY_HW_HAS_FLASH
	storage_init();
	mounted_flash = init_flash_fs(0);
#endif
	bool mounted_sdcard = false;
#if MICROPY_HW_HAS_SDCARD
	uint32_t sdcard_state;
	if(first_soft_reset)
		sdcard_state = sdcard_init();
	if(sdcard_state != kWithout_Sdcard){
		if (!mounted_flash || f_stat(&fs_user_mount_flash.fatfs, "/SKIPSD", NULL) != FR_OK)
			mounted_sdcard = init_sdcard_fs();
	}
#endif

	MP_STATE_PORT(pyb_config_main) = MP_OBJ_NULL;
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
    mp_obj_list_init(mp_sys_argv, 0);
	
     if (mounted_sdcard) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd_slash_lib));
    }
	if (mounted_flash) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));
    }
    mp_vfs_getcwd();
      // Run the main script from the current directory.
    const char *main_py;
    if (MP_STATE_PORT(pyb_config_main) == MP_OBJ_NULL) {
        main_py = "main.py";
    } else {
        main_py = mp_obj_str_get_str(MP_STATE_PORT(pyb_config_main));
    }
     mp_import_stat_t stat = mp_import_stat(main_py);
    if (stat == MP_IMPORT_STAT_FILE) {
        int ret = pyexec_file(main_py);
        if (!ret) {
            printf("ERROR!");
        }
    }
	//in fact, the pyexec_friendly_repl() will answer the input from the repl, such as CTRL_D, then do the related operation or quit the function with a exit_value code, judge the value jump to correspoding function
    retCode = pyexec_friendly_repl();
    mp_deinit();
	if(retCode & PYEXEC_FORCED_EXIT)
		goto soft_reset_exit;
	else
		goto unknown_exit;

soft_reset_exit:
	printf("PYB: soft reboot!\n");
#if MICROPY_PY_THREAD
    pyb_thread_deinit();
#endif
	first_soft_reset = false;
	goto soft_reset;

unknown_exit:
	printf("Unknown exit: hard reboot!\n");
	NVIC_SystemReset();
	
	printf("Hard reboot failed: dead-looping!\n");
	printf("Do the hand-reset to break the loop!\n");
	while(1);
    return 0;
}

void gc_collect(void) {
	void *dummy; //maybe not the real-sp but we can assume that the dummy is the last varible allocated in the stack, so it is the current sp.
	uint32_t sp; //the real sp
	__ASM volatile("addi %0, x2, 0" : "=r"(sp));
#if ENABLE_GC_OUTPUT
    printf("gc_collect\n");
#endif
    gc_collect_start();
	//gc_collect_root((void**)&__stack, ((uint32_t)&_ram_end - (uint32_t)&__stack) / sizeof(uint32_t) );
    gc_collect_root((void**)sp, ((uint32_t)&_estack - (uint32_t)sp) / sizeof(uint32_t) ); //sp and &sp have the same effect
    gc_collect_end();
#if ENABLE_GC_OUTPUT
    gc_dump_info();
#endif

}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
    while (1);
}

void NORETURN __fatal_error(const char *msg) {
    while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif

