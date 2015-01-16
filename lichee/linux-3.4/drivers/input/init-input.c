/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/sys_config.h>
#include <mach/clock.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/init-input.h>

/*****************I2c communication interface***************************************************/
int sw_i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;
	
	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;		
	
	ret=i2c_transfer(client->adapter, &msg,1);
	return ret;
}
EXPORT_SYMBOL(sw_i2c_write_bytes);

int i2c_read_bytes_addr8(struct i2c_client *client, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int ret=-1;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 1;		//data address
	msgs[0].buf   = buf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len-1;
	msgs[1].buf   = buf+1;
	
	ret=i2c_transfer(client->adapter, msgs, 2);
	return ret;
}
EXPORT_SYMBOL(i2c_read_bytes_addr8);

int i2c_read_bytes_addr16(struct i2c_client *client, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int ret=-1;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 2;		//data address
	msgs[0].buf   = buf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len-2;
	msgs[1].buf   = buf+2;
	
	ret=i2c_transfer(client->adapter, msgs, 2);
	return ret;
}
EXPORT_SYMBOL(i2c_read_bytes_addr16);

bool i2c_test(struct i2c_client * client)
{
        int ret,retry;
        uint8_t test_data[1] = { 0 };	//only write a data address.
        
        for(retry=0; retry < 5; retry++)
        {
                ret = sw_i2c_write_bytes(client, test_data, 1);	//Test i2c.
        	if (ret == 1)
        	        break;
        	msleep(10);
        }
        
        return ret==1 ? true : false;
} 
EXPORT_SYMBOL(i2c_test);
/*******************I2c communication interface end******************************************************/

/*******************CTP use interface********************************************************************/
bool ctp_get_int_port_rate(u32 gpio, u32 *clk)
{
        struct gpio_eint_debounce pdbc = {0,0};
        int ret = -1;
        ret = sw_gpio_eint_get_debounce(gpio, &pdbc);
        if(ret != 0){
                return false;
        }
        *clk = pdbc.clk_sel;
        return true;
}
EXPORT_SYMBOL(ctp_get_int_port_rate);

bool ctp_set_int_port_rate(u32 gpio, u32 clk)
{
        struct gpio_eint_debounce pdbc;
        int ret = -1;

        if((clk != 0) && (clk != 1)){
                return false;
        }
        
        ret = sw_gpio_eint_get_debounce(gpio, &pdbc);
        if(ret == 0){
                if(pdbc.clk_sel == clk){
                        return true;
                }
        }
        
        pdbc.clk_sel = clk;
        ret = sw_gpio_eint_set_debounce(gpio, pdbc);
        if(ret != 0){
                return false;
        }
        
        return true;
}
EXPORT_SYMBOL(ctp_set_int_port_rate);
bool ctp_get_int_port_deb(u32 gpio, u32 *clk_pre_scl)
{
        struct gpio_eint_debounce pdbc = {0,0};
        int ret = -1;
        
        ret = sw_gpio_eint_get_debounce(gpio, &pdbc);
        if(ret !=0){
                return false;
        }
        *clk_pre_scl = pdbc.clk_pre_scl;
        
        return true;
} 
EXPORT_SYMBOL(ctp_get_int_port_deb);
bool ctp_set_int_port_deb(u32 gpio, u32 clk_pre_scl)
{
        struct gpio_eint_debounce pdbc;
        int ret = -1;

        ret = sw_gpio_eint_get_debounce(gpio, &pdbc);
        if(ret ==0){
                if(pdbc.clk_pre_scl == clk_pre_scl){
                        return true;
                }
        }
        
        pdbc.clk_pre_scl = clk_pre_scl;
        ret = sw_gpio_eint_set_debounce(gpio, pdbc);
        if(ret != 0){
                return false;
        }
        
        return true;        
}
EXPORT_SYMBOL(ctp_set_int_port_deb);  
   

/**
 * ctp_wakeup - function
 *
 */
int ctp_wakeup(u32 gpio, int status, int ms)
{
       u32 gpio_status;
        
        gpio_status = sw_gpio_getcfg(gpio);
        if(gpio_status != 1){
                sw_gpio_setcfg(gpio, 1); //set output
        }

        if(status == 0){               
                if(ms == 0) { 
                        __gpio_set_value(gpio, 0);
                }else {
                        __gpio_set_value(gpio, 0);
                        msleep(ms);
                        __gpio_set_value(gpio, 1);
                }
        }
        if(status == 1){
                if(ms == 0) {
                        __gpio_set_value(gpio, 1);
                }else {
                        __gpio_set_value(gpio, 1); 
                        msleep(ms);
                        __gpio_set_value(gpio, 0); 
                }      
        }
        
        msleep(3);
         
        if(gpio_status != 1){
                sw_gpio_setcfg(gpio, gpio_status);
        }
	return 0;
}
EXPORT_SYMBOL(ctp_wakeup);
/**
 * ctp_key_light - function
 *
 */
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
int ctp_key_light(gpio, int status, int ms)
{
       u32 gpio_status;
        
        gpio_status = sw_gpio_getcfg(gpio);
        if(gpio_status != 1){
                sw_gpio_setcfg(gpio,1);
        }
        
        if(status == 0){
                 if(ms == 0) {
                        __gpio_set_value(gpio, 0);
                }else {
                        __gpio_set_value(gpio, 0);
                        msleep(ms);
                        __gpio_set_value(gpio, 1);
                }
        }
        if(status == 1){
                 if(ms == 0) {
                        __gpio_set_value(gpio, 1);
                }else{
                        __gpio_set_value(gpio, 1); 
                        msleep(ms);
                        __gpio_set_value(gpio, 0); 
                }      
        }
        msleep(10);  
        if(gpio_status != 1){
                sw_gpio_setcfg(gpio, gpio_status);
        } 
	return 0;
}
EXPORT_SYMBOL(ctp_key_light);
#endif
/****************CTP use interface end********************************************************************/

/*************script interface****************************************************************************/
script_item_u get_para_value(char* keyname, char* subname)
{
        script_item_u	val;
        script_item_value_type_e type = 0;

        
        if((!keyname) || (!subname)) {
               printk("keyname:%s  subname:%s \n", keyname, subname);
               goto script_get_item_err;
        }
                
        memset(&val, 0, sizeof(val));
        type = script_get_item(keyname, subname, &val);
         
        if((SCIRPT_ITEM_VALUE_TYPE_INT != type) && (SCIRPT_ITEM_VALUE_TYPE_STR != type) &&
          (SCIRPT_ITEM_VALUE_TYPE_PIO != type)) {
	        goto script_get_item_err;
	}
	
        return val;
        
script_get_item_err:
        printk("keyname:%s  subname:%s ,get error!\n", keyname, subname);
        val.val = -1;
	return val;
}

EXPORT_SYMBOL(get_para_value);

void get_str_para(char* name[], char* value[], int num)
{
        script_item_u	val;
        int ret = 0;
        
        if(num < 1) {
                printk("%s: num:%d ,error!\n", __func__, num);
                return;
        }
        
        while(ret < num)
        {     
                val = get_para_value(name[0],name[ret+1]); 
                if(val.val == -1) {                        
                        val.val = 0;
                        val.str = "";
                }
                ret ++;
                *value = val.str;
                
                if(ret < num)
                        value++;

        }
}

EXPORT_SYMBOL(get_str_para);

void get_int_para(char* name[], int value[], int num)
{
        script_item_u	val;
        int ret = 0;
        
        if(num < 1) {
                printk("%s: num:%d ,error!\n", __func__, num);
                return;
        }
        
        while(ret < num)
        {
                val = get_para_value(name[0],name[ret + 1]); 
                if(val.val == -1) {                        
                        val.val = 0;
                }
                value[ret] = val.val;
	        ret++;
        }
}
EXPORT_SYMBOL(get_int_para);

void get_pio_para(char* name[], struct gpio_config value[], int num)
{
        script_item_u	val;
        int ret = 0;
        
        if(num < 1) {
                printk("%s: num:%d ,error!\n", __func__, num);
                return;
        }
        
        while(ret < num)
        {     
                val = get_para_value(name[0],name[ret+1]); 
                if(val.val == -1) {                        
                        val.val = 0;
                        val.gpio.gpio = -1;
                }
                value[ret] = val.gpio;
	        ret++;
        }
}

EXPORT_SYMBOL(get_pio_para);
/*********************script interface end********************************************************/

/*********************************CTP*******************************************/

/**
 * ctp_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_fetch_sysconfig_para(enum input_sensor_type *ctp_type)
{
	int ret = -1;
	int value[10] = {0}; 
	script_item_u	val;
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);
	char *name[11] = {
                "ctp_para",
                "ctp_used",
                "ctp_twi_id",
                "ctp_screen_max_x",
                "ctp_screen_max_y",
                "ctp_revert_x_flag",
                "ctp_revert_y_flag",
                "ctp_exchange_x_y_flag",
                "ctp_int_port",
                "ctp_wakeup",
                "ctp_light",
	};
	

	pr_info("=====%s=====. \n", __func__);
	
	for(ret = 0; ret < 9; ret ++ ) {
                val = get_para_value(name[0],name[ret + 1]);
                if(val.val == -1) {
                        ret = -1;
	                goto script_get_item_err;
	        }
	        if(ret < 7) {
	                value[ret] = val.val;
//	                printk("%s:val.val:%d\n", __func__, val.val);
	        } else {
	                value[ret] = val.gpio.gpio; 
//	                  printk("%s:val.gpio.gpio:%d\n", __func__, val.gpio.gpio);
	        }
	}
	
        data->ctp_used          = value[0];	        
	data->twi_id            = value[1];
	data->screen_max_x      = value[2];
	data->screen_max_y      = value[3];
	data->revert_x_flag     = value[4];
	data->revert_y_flag     = value[5];
	data->exchange_x_y_flag = value[6];
	
	data->int_number        = value[7];
        data->wakeup_number     = value[8];
        
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
	val = get_para_value(name[0],name[10]);
	if(val.val == -1) {
                        ret = -1;
	                goto script_get_item_err;
	}
	data->light_number = val.gpio.gpio;
#endif
	
	return 0;

script_get_item_err:
	pr_notice("[init--ctp]=========script_get_item_err============\n");
	return ret;
}

/**
 * ctp_free_platform_resource - free ctp related resource
 * return value:
 */
static void ctp_free_platform_resource(void)
{
        int gpio_cnt, i;
	script_item_u *list = NULL;

	gpio_cnt = script_get_pio_list("ctp_para", &list);
	for(i = 0; i < gpio_cnt; i++)
		gpio_free(list[i].gpio.gpio);
}

/**
 * ctp_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ctp_init_platform_resource(enum input_sensor_type *ctp_type)
{	
	int cnt = 0, i = 0;
	script_item_u  *list = NULL;
					
	cnt = script_get_pio_list("ctp_para", &list);
	if (0 == cnt) {
	        printk("%s:[init--ctp]get gpio list failed\n", __func__);
		return -1;
	}				
					
        /* ÉêÇëgpio */
	for (i = 0; i < cnt; i++) 
	        if (0 != gpio_request(list[i].gpio.gpio, NULL)){
		        printk("%s:[init--ctp]gpio_request i:%d, gpio:%d failed\n",
		                 __func__, i, list[i].gpio.gpio);
		}

	/* ÅäÖÃgpio list */
	if (0 != sw_gpio_setall_range(&list[0].gpio, cnt))
	        printk("[init--ctp] sw_gpio_setall_range failed\n");      
	
	return 0;
}
/*********************************CTP END***************************************/

/*********************************GSENSOR***************************************/

/**
 * gsensor_free_platform_resource - free gsensor related resource
 * return value:
 */
static void gsensor_free_platform_resource(void)
{
}

/**
 * gsensor_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int gsensor_init_platform_resource(enum input_sensor_type *gsensor_type)
{
	return 0;
}

/**
 * gsensor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_fetch_sysconfig_para(enum input_sensor_type *gsensor_type)
{
	int ret = -1;
	int value[2];
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);
	char* name[3] = {
	        "gsensor_para",
	        "gsensor_used",
	        "gsensor_twi_id",
	        
	                
	        
	};
        
        get_int_para(name, value, 2);		
	data->sensor_used       = value[0];	
	data->twi_id            = value[1];
	
	if(data->sensor_used == 1)
	        ret = 0;
	
	return ret;
}

/*********************************GSENSOR END***********************************/

/********************************** GYR ****************************************/

/**
 * gyr_free_platform_resource - free gsensor related resource
 * return value:
 */
static void gyr_free_platform_resource(void)
{
}

/**
 * gyr_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int gyr_init_platform_resource(enum input_sensor_type *gyr_type)
{
	return 0;
}

/**
 * gyr_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gyr_fetch_sysconfig_para(enum input_sensor_type *gyr_type)
{
        int ret = -1;
	int value[2];
	struct sensor_config_info *data = container_of(gyr_type,
					struct sensor_config_info, input_type);
	char* name[3] = {
	        "gy_para",
	        "gy_used",
	        "gy_twi_id",
	        
	                
	        
	};
        
        get_int_para(name, value, 2);		
	data->sensor_used       = value[0];	
	data->twi_id            = value[1];
	
	if(data->sensor_used == 1)
	        ret = 0;
	
	return ret;
}

/********************************* GYR END *************************************/

/********************************** IR *****************************************/

/**
 * gyr_free_platform_resource - free gsensor related resource
 * return value:
 */
static void ir_free_platform_resource(void)
{
	 int gpio_cnt, i;
	script_item_u *list = NULL;

	gpio_cnt = script_get_pio_list("ir_para", &list);
	
	for(i = 0; i < gpio_cnt; i++)
		gpio_free(list[i].gpio.gpio);
}

/**
 * gyr_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ir_init_platform_resource(enum input_sensor_type *ir_type)
{
	int ret = -1;
	struct ir_config_info *data = container_of(ir_type,
					struct ir_config_info, input_type);

	if(0 != gpio_request(data->ir_gpio.gpio, NULL)) {	
		printk(KERN_ERR "ERROR: IR Gpio_request is failed\n");
	}
	if (0 != sw_gpio_setall_range(&(data->ir_gpio), 1)) {
		printk(KERN_ERR "IR gpio set err!");
		return ret;
	}
	
	return 0;
}

/**
 * gyr_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ir_fetch_sysconfig_para(enum input_sensor_type *ir_type)
{
	int ret = -1;
	int value[2] = {0}; 
	char * name[] = {
	        "ir_para",
	        "ir_used",
	        "ir_rate"	        
	};
	char * name1[] = {
	        "ir_para",
	        "ir_rx"
	        
	};
	struct ir_config_info *data = container_of(ir_type,
					struct ir_config_info, input_type);
		
	get_int_para(name, value, 2);
	data->ir_used = value[0];
	data->rate = value[1];
	
	get_pio_para(name1, &data->ir_gpio, 1);
	
	if(data->ir_used == 1)
	        ret = 0;
        
        return ret;
}

/********************************** IR END *************************************/

/*********************************LIGHTSENSOR***************************************/

/**
 * gsensor_free_platform_resource - free gsensor related resource
 * return value:
 */
static void ls_free_platform_resource(void)
{
}

/**
 * gsensor_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ls_init_platform_resource(enum input_sensor_type *gsensor_type)
{
	return 0;
}

/**
 * gsensor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ls_fetch_sysconfig_para(enum input_sensor_type *gsensor_type)
{
	int ret = -1;
	int value[2];
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);
	char* name[3] = {
	        "ls_para",
	        "ls_used",
	        "ls_twi_id",
	        
	                
	        
	};
        
        get_int_para(name, value, 2);		
	data->sensor_used       = value[0];	
	data->twi_id            = value[1];
	
	if(data->sensor_used == 1)
	        ret = 0;
	
	return ret;
}

/*********************************LIGHTSENSOR END***********************************/

/*********************************COMPASS***************************************/

/**
 * e_compass_free_platform_resource - free e_compass related resource
 * return value:
 */
static void e_compass_free_platform_resource(void)
{
}

/**
 * e_compass_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int e_compass_init_platform_resource(enum input_sensor_type *gsensor_type)
{
	return 0;
}

/**
 * e_compass_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int e_compass_fetch_sysconfig_para(enum input_sensor_type *gsensor_type)
{
	int ret = -1;
	int value[2];
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);
	char* name[3] = {
	        "compass_para",
	        "compass_used",
	        "compass_twi_id",
	        
	                
	        
	};
        
        get_int_para(name, value, 2);		
	data->sensor_used       = value[0];	
	data->twi_id            = value[1];
	
	if(data->sensor_used == 1)
	        ret = 0;
	
	return ret;
}

/*********************************COMPASS END***********************************/

int (* const fetch_sysconfig_para[])(enum input_sensor_type *input_type) = {
	ctp_fetch_sysconfig_para,
	gsensor_fetch_sysconfig_para,
	gyr_fetch_sysconfig_para,
	ir_fetch_sysconfig_para,
	ls_fetch_sysconfig_para,
	e_compass_fetch_sysconfig_para,
};

int (*init_platform_resource[])(enum input_sensor_type *input_type) = {
	ctp_init_platform_resource,
	gsensor_init_platform_resource,
	gyr_init_platform_resource,
	ir_init_platform_resource,
	ls_init_platform_resource,
	e_compass_init_platform_resource
};

void (*free_platform_resource[])(void) = {
	ctp_free_platform_resource,
	gsensor_free_platform_resource,
	gyr_free_platform_resource,
	ir_free_platform_resource,
	ls_free_platform_resource,
	e_compass_free_platform_resource
};


/**
 * input_free_platform_resource - free platform related resource
 * Input:
 * 	event:
 * return value:
 */
void input_free_platform_resource(enum input_sensor_type *input_type)
{
        (*free_platform_resource[*input_type])();
	return;
}
EXPORT_SYMBOL(input_free_platform_resource);

/**
 * input_init_platform_resource - initialize platform related resource
 * Input:
 * 	type:
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
int input_init_platform_resource(enum input_sensor_type *input_type)
{
	int ret = -1;
	
	ret = (*init_platform_resource[*input_type])(input_type);
	
	return ret;	
}
EXPORT_SYMBOL(input_init_platform_resource);

/**
 * input_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * Input:
 * 	type:
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
int input_fetch_sysconfig_para(enum input_sensor_type *input_type)
{
	int ret = -1;
	
	ret = (*fetch_sysconfig_para[*input_type])(input_type);
	
	return ret;
}
EXPORT_SYMBOL(input_fetch_sysconfig_para);
