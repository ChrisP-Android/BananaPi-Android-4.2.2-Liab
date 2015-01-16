/* drivers/input/touchscreen/gt811.h
 * 
 * 2010 - 2012 Goodix Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version:1.0
 *
 */

#ifndef _LINUX_GOODIX_TOUCH_H
#define	_LINUX_GOODIX_TOUCH_H

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>
//#include <mach/gpio.h>
#include <linux/earlysuspend.h>

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/delay.h>
 
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#include <asm/io.h>

#include <mach/gpio.h> 
#include <linux/init-input.h>
#include <linux/pm.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif
#define GTP_CREATE_WR_NODE 0
extern s32 gup_update_proc(void*);
extern struct i2c_client * i2c_connect_client;
extern s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len);
extern s32 gtp_i2c_write(struct i2c_client *client,u8 *data,s32 len);
extern u16 show_len;
extern u16 total_len;

struct goodix_ts_data {
    spinlock_t irq_lock;
    struct i2c_client *client;
    struct input_dev  *input_dev;
    struct hrtimer timer;
    struct work_struct  work;
    struct early_suspend early_suspend;
    s32 irq_is_disable;
    s32 use_irq;
    u16 abs_x_max;
    u16 abs_y_max;
    u16 version;
    u8  max_touch_num;
    u8  int_trigger_type;
    u8  green_wake_mode;
    u8  chip_type;
    u8  enter_update;
    u8  gtp_is_suspend;
};

extern s32 gtp_read_version(struct goodix_ts_data *ts);
extern s32 gup_downloader( struct goodix_ts_data *ts, u8 *data);
extern s32 gtp_init_panel(struct goodix_ts_data *ts);
extern void gtp_irq_disable(struct goodix_ts_data *ts);
extern void gtp_irq_enable(struct goodix_ts_data *ts);

//***************************PART1:ON/OFF define*******************************
#define GTP_DEBUG_ON          1
#define GTP_DEBUG_ARRAY_ON    0
#define GTP_DEBUG_FUNC_ON     0
#define GTP_CUSTOM_CFG        0
#define GTP_DRIVER_SEND_CFG   1
#define GTP_HAVE_TOUCH_KEY    0
#define GTP_POWER_CTRL_SLEEP  0
#define GTP_AUTO_UPDATE       0
#define GTP_CHANGE_X2Y        1
#define GTP_ESD_PROTECT       0

//***************************PART2:TODO define**********************************
//STEP_1(REQUIRED):Change config table.
/*TODO: puts the config info corresponded to your TP here, the following is just 
a sample config, send this config should cause the chip cannot work normally*/
 //S738 dpt yiju gandy
#define CTP_CFG_GROUP_DPT1 {\
        (u8)(GTP_REG_CONFIG_DATA>>8),(u8)GTP_REG_CONFIG_DATA,\
        0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,\
        0x10,0x12,0x00,0x00,0x10,0x00,0x20,0x00,\
        0x30,0x00,0x40,0x00,0x50,0x00,0x60,0x00,\
        0x70,0x00,0x80,0x00,0x90,0x00,0xA0,0x00,\
        0xB0,0x00,0xC0,0x00,0xD0,0x00,0xE0,0x00,\
        0xF0,0x00,0x1B,0x03,0x88,0x88,0x88,0x22,\
        0x22,0x22,0x10,0x10,0x0A,0x50,0x30,0x05,\
        0x03,0x00,0x05,0x58,0x02,0x00,0x04,0x00,\
        0x00,0x54,0x54,0x60,0x60,0x00,0x00,0x35,\
        0x14,0x25,0x0F,0x80,0x00,0x00,0x00,0x00,\
        0x14,0x10,0x60,0x06,0x00,0x00,0x00,0x00,\
        0x00,0x00,0x00,0x00,0x00,0x00,0x0D,0x00,\
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        0x00,0x01\
}


//s738 
#define CTP_CFG_GROUP {\
	(u8)(GTP_REG_CONFIG_DATA>>8),(u8)GTP_REG_CONFIG_DATA,\
        0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,\
        0x10,0x12,0x00,0x00,0x10,0x00,0x20,0x00,\
        0x30,0x00,0x40,0x00,0x50,0x00,0x60,0x00,\
        0x70,0x00,0x80,0x00,0x90,0x00,0xA0,0x00,\
        0xB0,0x00,0xC0,0x00,0xD0,0x00,0xE0,0x00,\
        0xF0,0x00,0x1B,0x03,0x68,0x68,0x68,0x19,\
        0x19,0x19,0x10,0x10,0x0A,0x50,0x30,0x05,\
        0x03,0x00,0x05,0x58,0x02,0x00,0x04,0x00,\
        0x00,0x66,0x66,0x60,0x60,0x00,0x00,0x23,\
        0x14,0x25,0x0F,0x80,0x00,0x00,0x00,0x00,\
        0x14,0x10,0x60,0x06,0x00,0x00,0x00,0x00,\
        0x00,0x00,0x00,0x00,0x00,0x00,0x0D,0x00,\
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        0x00,0x01\
}

//TODO puts your group2 config info here,if need.
#define CTP_CFG_GROUP2 {\
    }
//TODO puts your group3 config info here,if need.
#define CTP_CFG_GROUP3 {\
    }

//STEP_2(REQUIRED):Change I/O define & I/O operation mode.
#define GTP_INT_PORT    S3C64XX_GPN(15)
#define GTP_RST_PORT    S3C64XX_GPL(10)
#define GTP_INT_IRQ     gpio_to_irq(GTP_INT_PORT)
#define GTP_INT_CFG     S3C_GPIO_SFN(2)

#define GTP_GPIO_AS_INPUT(pin)          do{\
                                            gpio_direction_input(pin);\
                                            s3c_gpio_setpull(pin, S3C_GPIO_PULL_NONE);\
                                        }while(0)
#define GTP_GPIO_AS_INT(pin)            do{\
                                            gpio_direction_input(pin);\
                                            s3c_gpio_setpull(pin, S3C_GPIO_PULL_NONE);\
                                            s3c_gpio_cfgpin(pin, GTP_INT_CFG);\
                                        }while(0)
#define GTP_GPIO_GET_VALUE(pin)         gpio_get_value(pin)
#define GTP_GPIO_OUTPUT(pin,level)      gpio_direction_output(pin,level)
#define GTP_GPIO_REQUEST(pin, label)    gpio_request(pin, label)
#define GTP_GPIO_FREE(pin)              gpio_free(pin)
#define GTP_IRQ_TAB                     {IRQ_TYPE_EDGE_FALLING,IRQ_TYPE_EDGE_RISING}

//STEP_3(optional):Custom set some config by themself,if need.
#if GTP_CUSTOM_CFG
    #define GTP_MAX_WIDTH    480
    #define GTP_MAX_HEIGHT   800
    #define GTP_MAX_TOUCH    5
    #define GTP_INT_TRIGGER  0
    #define GTP_REFRESH      0
#else
    #define GTP_MAX_WIDTH    4096
    #define GTP_MAX_HEIGHT   4096
    #define GTP_MAX_TOUCH    5
    #define GTP_INT_TRIGGER  0
    #define GTP_REFRESH      0
#endif
#define GTP_ESD_CHECK_CIRCLE  2000


//STEP_4(optional):If this project have touch key,Set touch key config.                                    
#if GTP_HAVE_TOUCH_KEY
#define GTP_KEY_TAB {KEY_MENU, KEY_HOME, KEY_SEND}
#endif

//***************************PART3:OTHER define*********************************
#define GTP_DRIVER_VERSION    "V1.0<2012/05/01>"
#define GTP_I2C_NAME          "gt811"	//"Goodix-TS"
#define GTP_POLL_TIME		      10
#define GTP_ADDR_LENGTH       2
#define GTP_CONFIG_LENGTH     106
#define FAIL                  0
#define SUCCESS               1

#if GTP_MAX_TOUCH <= 3
#define GTP_READ_BYTES 2+2+GTP_MAX_TOUCH*5
#elif GTP_MAX_TOUCH == 4
#define GTP_READ_BYTES 2+28
#elif GTP_MAX_TOUCH == 5
#define GTP_READ_BYTES 2+34
#endif
//Register define
#define GTP_REG_CONFIG_DATA   0x6A2
#define GTP_REG_COOR          0x721
#define GTP_REG_SLEEP         0x692
#define GTP_REG_SENSOR_ID     0x710
#define GTP_REG_VERSION       0x715
//Log define

#if 0
#define PRINT_GTP_INFO
#define PRINT_GTP_ERROR
#define PRINT_GTP_DEBUG
#define PRINT_GTP_DEBUG_ARRAY
#define PRINT_GTP_DEBUG_FUNC
#endif

#ifdef PRINT_GTP_INFO
#define GTP_INFO(fmt,arg...)           printk("<<-GTP-INFO->> "fmt"\n",##arg)
#else
#define GTP_INFO(fmt,arg...)	//
#endif

#ifdef PRINT_GTP_ERROR
#define GTP_ERROR(fmt,arg...)          printk("<<-GTP-ERROR->> "fmt"\n",##arg)
#else
#define GTP_ERROR(fmt,arg...)	//
#endif

#ifdef PRINT_GTP_DEBUG
#define GTP_DEBUG(fmt,arg...)          do{\
                                         if(GTP_DEBUG_ON)\
                                         printk("<<-GTP-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                       }while(0)
#else
#define GTP_DEBUG(fmt,arg...)	//
#endif

#ifdef PRINT_GTP_DEBUG_ARRAY
#define GTP_DEBUG_ARRAY(array, num)    do{\
                                         s32 i;\
                                         u8* a = array;\
                                         if(GTP_DEBUG_ARRAY_ON)\
                                         {\
                                            printk("<<-GTP-DEBUG-ARRAY->>\n");\
                                            for (i = 0; i < (num); i++)\
                                            {\
                                                printk("%02x   ", (a)[i]);\
                                                if ((i + 1 ) %10 == 0)\
                                                {\
                                                    printk("\n");\
                                                }\
                                            }\
                                            printk("\n");\
                                        }\
                                       }while(0)
#else
#define GTP_DEBUG_ARRAY(array, num)	//
#endif

#ifdef PRINT_GTP_DEBUG_FUNC
#define GTP_DEBUG_FUNC()               do{\
                                         if(GTP_DEBUG_FUNC_ON)\
                                         printk("<<-GTP-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)
#else
#define GTP_DEBUG_FUNC()	//
#endif

#define GTP_SWAP(x, y)                 do{\
                                         typeof(x) z = x;\
                                         x = y;\
                                         y = z;\
                                       }while (0)

//****************************PART4:UPDATE define*******************************
#define  PACK_SIZE                  64
#define  NVRAM_LEN                  0x0FF0   //	nvram total space
#define  NVRAM_BOOT_SECTOR_LEN      0x0100	// boot sector 
#define  NVRAM_UPDATE_START_ADDR    0x4100
#define  NVRAM_STORE_CMD            0x19
#define  NVRAM_STORE_MAX_LEN        0xFB0

#define  BIT_NVRAM_STROE            0
#define  BIT_NVRAM_RECALL           1
#define  BIT_NVRAM_LOCK             2
#define  REG_NVRCS_H                0x12
#define  REG_NVRCS_L                0x01
//*****************************End of Part III********************************


#endif /* _LINUX_GOODIX_TOUCH_H */
