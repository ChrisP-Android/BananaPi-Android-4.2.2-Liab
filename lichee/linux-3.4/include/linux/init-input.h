/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef _INIT_INPUT_H
#define _INIT_INPUT_H
#include <linux/i2c.h>
#include <mach/gpio.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

enum{
        DEBUG_INIT              = 1U << 0,
        DEBUG_SUSPEND           = 1U << 1,
        DEBUG_INT_INFO          = 1U << 2,
        DEBUG_X_Y_INFO          = 1U << 3,
        DEBUG_KEY_INFO          = 1U << 4,
        DEBUG_WAKEUP_INFO       = 1U << 5,
        DEBUG_OTHERS_INFO       = 1U << 6, 
                     
};

enum {
	DEBUG_CONTROL_INFO      = 1U << 2,
	DEBUG_DATA_INFO         = 1U << 3,
};

enum {
	DEBUG_REPORT_DATA = 1U << 3,
	DEBUG_INT = 1U << 4,
};


enum input_sensor_type{
	CTP_TYPE,
	GSENSOR_TYPE,
	GYR_TYPE,
	IR_TYPE,
	LS_TYPE,
	COMPASS_TYPE,
	MOTOR_TYPE
};

struct sensor_config_info{
	enum input_sensor_type input_type;
	int sensor_used;
	__u32 twi_id;
};

struct ir_config_info{
	enum input_sensor_type input_type;
	int ir_used;
	struct gpio_config ir_gpio;
	unsigned long rate;
};

struct ctp_config_info{	
        enum input_sensor_type  input_type;
        	
	int     ctp_used;
	__u32   twi_id;
	int     screen_max_x;
	int     screen_max_y;
	int     revert_x_flag;
	int     revert_y_flag;	
	int     exchange_x_y_flag;
	
	u32     int_number;
	u32     wakeup_number;
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
        u32     light_number;
#endif  	        
};

script_item_u get_para_value(char* keyname, char* subname);
void get_str_para(char* name[], char* value[], int num);
void get_int_para(char* name[], int value[], int num);
void get_pio_para(char* name[], struct gpio_config value[], int num);
int input_fetch_sysconfig_para(enum input_sensor_type *input_type);
void input_free_platform_resource(enum input_sensor_type *input_type);
int input_init_platform_resource(enum input_sensor_type *input_type);

int sw_i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len);
int i2c_read_bytes_addr8(struct i2c_client *client, uint8_t *buf, uint16_t len);
int i2c_read_bytes_addr16(struct i2c_client *client, uint8_t *buf, uint16_t len);
bool i2c_test(struct i2c_client * client);
bool ctp_get_int_port_rate(u32 gpio, u32 *clk);
bool ctp_set_int_port_rate(u32 gpio, u32 clk);
bool ctp_get_int_port_deb(u32 gpio, u32 *clk_pre_scl);
bool ctp_set_int_port_deb(u32 gpio, u32 clk_pre_scl);
int ctp_wakeup(u32 gpio, int status, int ms);

#ifdef TOUCH_KEY_LIGHT_SUPPORT 
int ctp_key_light(int status,int ms);
#endif

#endif
