/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */



#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/input/mt.h>

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#include <mach/gpio.h>
#include <linux/init-input.h>

#include "gslX680.h"
#include "gsl1680.h"        //resolution:1024*600
#include "gsl1680_1.h"      //resolution:800*480
#include "gsl1680_2.h"      //resolution:1024*600 
//gsl1680_2.h and gsl1680.h difference lies in the x axis, y axis and interrupt the parameters of the differences
#include "gsl2680.h"        //resolution:1024*768
#include "gsl3680.h"        //resolution:2048*1563


struct gslX680_fw_array {
	const char* name;
	unsigned int size;
	const struct fw_data *fw;
} gslx680_fw_grp[] = {
	{"gsl1680",     ARRAY_SIZE(GSL1680_FW),   GSL1680_FW},
	{"gsl1680_1",   ARRAY_SIZE(GSL1680_FW_1), GSL1680_FW_1},
	{"gsl1680_2",   ARRAY_SIZE(GSL1680_FW_2), GSL1680_FW_2},
	{"gsl2680",     ARRAY_SIZE(GSL2680_FW),   GSL2680_FW},
	{"gsl3680",     ARRAY_SIZE(GSL3680_FW),   GSL3680_FW},
};

#define FOR_TSLIB_TEST
//#define GSL_TIMER
//#define PRINT_POINT_INFO
//#define REPORT_DATA_ANDROID_4_0
static u32 gslX680_debug_mask = 1;

//#define HAVE_TOUCH_KEY

#define GSLX680_I2C_NAME 	"gslX680"
#define GSLX680_I2C_ADDR 	0x40


#define GSL_DATA_REG		0x80
#define GSL_STATUS_REG		0xe0
#define GSL_PAGE_REG		0xf0

#define PRESS_MAX    			255
#define MAX_FINGERS 		10//5 //最大手指个数
#define MAX_CONTACTS 		10
#define DMA_TRANS_LEN		0x20


#ifdef HAVE_TOUCH_KEY
static u16 key = 0;
static int key_state_flag = 0;
struct key_data {
	u16 key;
	u16 x_min;
	u16 x_max;
	u16 y_min;
	u16 y_max;
};

#define KEY_BACK	1
#define KEY_HOME	2
#define KEY_MENU	3
#define KEY_SEARCH	4

const u16 key_array[]={
                                      KEY_BACK,
                                      KEY_HOME,
                                      KEY_MENU,
                                      KEY_SEARCH,
                                     };
#define MAX_KEY_NUM     (sizeof(key_array)/sizeof(key_array[0]))

struct key_data gsl_key_data[MAX_KEY_NUM] = {
	{KEY_BACK, 2048, 2048, 2048, 2048},
	{KEY_HOME, 2048, 2048, 2048, 2048},
	{KEY_MENU, 2048, 2048, 2048, 2048},
	{KEY_SEARCH, 2048, 2048, 2048, 2048},
};
#endif

struct gsl_ts_data {
	u8 x_index;
	u8 y_index;
	u8 z_index;
	u8 id_index;
	u8 touch_index;
	u8 data_reg;
	u8 status_reg;
	u8 data_size;
	u8 touch_bytes;
	u8 update_data;
	u8 touch_meta_data;
	u8 finger_size;
};

static struct gsl_ts_data devices[] = {
	{
		.x_index = 6,
		.y_index = 4,
		.z_index = 5,
		.id_index = 7,
		.data_reg = GSL_DATA_REG,
		.status_reg = GSL_STATUS_REG,
		.update_data = 0x4,
		.touch_bytes = 4,
		.touch_meta_data = 4,
		.finger_size = 70,
	},
};

struct gsl_ts {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct gsl_ts_data *dd;
	u8 *touch_data;
	u8 device_id;
	u8 prev_touches;
	bool is_suspended;
	bool int_pending;
	struct mutex sus_lock;
	int irq;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef GSL_TIMER
	struct timer_list gsl_timer;
#endif

};

static struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("[CTP]:"fmt, ## arg)

static u32 id_sign[MAX_CONTACTS+1] = {0};
static u8 id_state_flag[MAX_CONTACTS+1] = {0};
static u8 id_state_old_flag[MAX_CONTACTS+1] = {0};
static u16 x_old[MAX_CONTACTS+1] = {0};
static u16 y_old[MAX_CONTACTS+1] = {0};
static u16 x_new = 0;
static u16 y_new = 0;


///////////////////////////////////////////////
//specific tp related macro: need be configured for specific tp

#define CTP_IRQ_NUMBER                  (config_info.int_number)
#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)
#define CTP_NAME			GSLX680_I2C_NAME
#define SCREEN_MAX_X		        (screen_max_x)
#define SCREEN_MAX_Y		        (screen_max_y)


static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
static char* fwname;
static int fw_index = -1;

static __u32 twi_id = 0;

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x40,I2C_CLIENT_END};

static void glsX680_init_events(struct work_struct *work);
static void glsX680_resume_events(struct work_struct *work);
struct workqueue_struct *gslX680_wq;
struct workqueue_struct *gslX680_resume_wq;
static DECLARE_WORK(glsX680_init_work, glsX680_init_events);
static DECLARE_WORK(glsX680_resume_work, glsX680_resume_events);
struct i2c_client *glsX680_i2c;
struct gsl_ts *ts_init;

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;

	if(twi_id == adapter->nr){
                dprintk(DEBUG_INIT,"%s: addr= %x\n",__func__,client->addr);
                ret = i2c_test(client);
                if(!ret){
        		printk("%s:I2C connection might be something wrong \n",__func__);
        		return -ENODEV;
		}else{
			strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
			return 0;
	        }

	}else{
		return -ENODEV;
	}
}



static ssize_t gslX680_debug_enable_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return sprintf(buf, "0x%x", gslX680_debug_mask);
}

static ssize_t gslX680_debug_enable_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	if (buf[0] >= '0' && buf[0] <= '9')
	{
		gslX680_debug_mask = (buf[0] - '0');
	}
	else if (buf[0] >= 'a' && buf[0] <= 'f')
	{
		gslX680_debug_mask = 0x0A + (buf[0] - 'a');
	}
	else
	{
		gslX680_debug_mask = 0;
	}
	return count;
}

static DEVICE_ATTR(debug_enable, 0664, gslX680_debug_enable_show, gslX680_debug_enable_store);


static int gslX680_chip_init(void)
{
         ctp_wakeup(config_info.wakeup_number, 1, 0);
         msleep(20);
         return 0;
}

static int gslX680_shutdown_low(void)
{
        ctp_wakeup(config_info.wakeup_number, 0, 0);
	return 0;
}

static int gslX680_shutdown_high(void)
{
        ctp_wakeup(config_info.wakeup_number, 1, 0);
	return 0;
}

static inline u16 join_bytes(u8 a, u8 b)
{
	u16 ab = 0;
	ab = ab | a;
	ab = ab << 8 | b;
	return ab;
}

static u32 gsl_write_interface(struct i2c_client *client, const u8 reg, u8 *buf, u32 num)
{
	struct i2c_msg xfer_msg[1];

	buf[0] = reg;

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = num + 1;
	xfer_msg[0].flags = client->flags & I2C_M_TEN;
	xfer_msg[0].buf = buf;

	return i2c_transfer(client->adapter, xfer_msg, 1) == 1 ? 0 : -EFAULT;
}

static __inline__ void fw2buf(u8 *buf, const u32 *fw)
{
	u32 *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static int gsl_find_fw_idx(const char* name)
{
	int i = 0;
	
	for (i=0; i<ARRAY_SIZE(gslx680_fw_grp); i++) {
		if (!strcmp(name, gslx680_fw_grp[i].name))
			return i;
	}
	return -1;
}

static void gsl_load_fwx680(struct i2c_client *client)
{
	u8 buf[DMA_TRANS_LEN*4 + 1] = {0};
	u8 send_flag = 1;
	u8 *cur = buf + 1;
	u32 source_line = 0;
	u32 source_len = gslx680_fw_grp[fw_index].size;
	const struct fw_data *fw = gslx680_fw_grp[fw_index].fw;

	dprintk(DEBUG_INIT,"=============gsl_load_fw start==============\n");

	for (source_line = 0; source_line < source_len; source_line++) {
		/* init page trans, set the page val */
		if (GSL_PAGE_REG == fw[source_line].offset){
			fw2buf(cur, &fw[source_line].val);
			gsl_write_interface(client, GSL_PAGE_REG, buf, 4);
			send_flag = 1;
		}
		else {
			if (1 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20))
				buf[0] = (u8)fw[source_line].offset;

			fw2buf(cur, &fw[source_line].val);
			cur += 4;

			if (0 == send_flag % (DMA_TRANS_LEN < 0x20 ? DMA_TRANS_LEN : 0x20)) {
				gsl_write_interface(client, buf[0], buf, cur - buf - 1);
				cur = buf + 1;
			}
			send_flag++;
		}
	}
	dprintk(DEBUG_INIT,"=============gsl_load_fw end==============\n");
}

static int gsl_ts_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125){
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	tmp_buf[0] = addr;
	bytelen++;

	if (datalen != 0 && pdata != NULL){
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}

	ret = i2c_master_send(client, tmp_buf, bytelen);
	return ret;
}

static int gsl_ts_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;

	if (datalen > 126){
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	ret = gsl_ts_write(client, addr, NULL, 0);
	if (ret < 0){
		printk("%s set data address fail!\n", __func__);
		return ret;
	}

	return i2c_master_recv(client, pdata, datalen);
}

static void gls_ts_irq_onoff(u32 onoff)
{
#ifndef GSL_TIMER
	if (onoff)
		sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,1);
	else
		sw_gpio_eint_set_enable(CTP_IRQ_NUMBER,0);
#endif
}

static void startup_chip(struct i2c_client *client)
{
	u8 tmp = 0x00;
	gsl_ts_write(client, 0xe0, &tmp, 1);
	msleep(10);
}

static void reset_chip(struct i2c_client *client)
{
	u8 buf[4] = {0x00};
	u8 tmp = 0x88;
	gsl_ts_write(client, 0xe0, &tmp, sizeof(tmp));
	msleep(10);

	tmp = 0x04;
	gsl_ts_write(client, 0xe4, &tmp, sizeof(tmp));
	msleep(10);

	gsl_ts_write(client, 0xbc, buf, sizeof(buf));
	msleep(10);
}

static void clr_reg(struct i2c_client *client)
{
	u8 write_buf[4]	= {0};

	write_buf[0] = 0x88;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1);
	msleep(20);
	write_buf[0] = 0x01;
	gsl_ts_write(client, 0x80, &write_buf[0], 1);
	msleep(5);
	write_buf[0] = 0x04;
	gsl_ts_write(client, 0xe4, &write_buf[0], 1);
	msleep(5);
	write_buf[0] = 0x00;
	gsl_ts_write(client, 0xe0, &write_buf[0], 1);
	msleep(20);
}

static void init_chip(struct i2c_client *client)
{
	gslX680_shutdown_low();
	msleep(50);
	gslX680_shutdown_high();
	msleep(30);
	//test_i2c(client);
	clr_reg(client);
	reset_chip(client);
	gsl_load_fwx680(client);
	startup_chip(client);
	reset_chip(client);
	startup_chip(client);
}

static void check_mem_data(struct i2c_client *client)
{
	u8 read_buf[4]  = {0};

	msleep(30);
	gsl_ts_read(client,0xb0, read_buf, sizeof(read_buf));

	if (read_buf[3] != 0x5a || read_buf[2] != 0x5a || read_buf[1] != 0x5a || read_buf[0] != 0x5a)
	{
		printk("#########check mem read 0xb0 = %x %x %x %x #########\n", read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
		init_chip(client);
	}
}

static void record_point(u16 x, u16 y , u8 id)
{
	u16 x_err =0;
	u16 y_err =0;

	id_sign[id]=id_sign[id]+1;

	if(id_sign[id]==1){
		x_old[id]=x;
		y_old[id]=y;
	}

	x = (x_old[id] + x)/2;
	y = (y_old[id] + y)/2;

	if(x>x_old[id]){
		x_err=x -x_old[id];
	}
	else{
		x_err=x_old[id]-x;
	}

	if(y>y_old[id]){
		y_err=y -y_old[id];
	}
	else{
		y_err=y_old[id]-y;
	}

	if( (x_err > 3 && y_err > 1) || (x_err > 1 && y_err > 3) ){
		x_new = x;     x_old[id] = x;
		y_new = y;     y_old[id] = y;
	}
	else{
		if(x_err > 3){
			x_new = x;     x_old[id] = x;
		}
		else
			x_new = x_old[id];
		if(y_err> 3){
			y_new = y;     y_old[id] = y;
		}
		else
			y_new = y_old[id];
	}

	if(id_sign[id]==1){
		x_new= x_old[id];
		y_new= y_old[id];
	}

}

#ifdef HAVE_TOUCH_KEY
static void report_key(struct gsl_ts *ts, u16 x, u16 y)
{
	u16 i = 0;
	for(i = 0; i < MAX_KEY_NUM; i++) {
		if((gsl_key_data[i].x_min < x) && (x < gsl_key_data[i].x_max)&&(gsl_key_data[i].y_min < y) &&\
		  (y < gsl_key_data[i].y_max)){
			key = gsl_key_data[i].key;
			input_report_key(ts->input, key, 1);
			input_sync(ts->input);
			key_state_flag = 1;
			break;
		}
	}
}
#endif

static void report_data(struct gsl_ts *ts, u16 x, u16 y, u8 pressure, u8 id)
{
	dprintk(DEBUG_X_Y_INFO,"source data :id=%d,x=%d,y=%d\n",id,x,y);

	if(1 == exchange_x_y_flag){
                swap(x, y);
        }
        if(1 == revert_x_flag){
                x = SCREEN_MAX_X - x;
        }
        if(1 == revert_y_flag){
                y = SCREEN_MAX_Y - y;
        }        

	dprintk(DEBUG_X_Y_INFO,"report data :id=%d,x=%d,y=%d\n",id,x,y);

	if(x>=SCREEN_MAX_X||y>=SCREEN_MAX_Y)
	{
	#ifdef HAVE_TOUCH_KEY
		report_key(ts,x,y);
	#endif
		return;
	}

#ifdef REPORT_DATA_ANDROID_4_0
	input_mt_slot(ts->input, id);
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
#else
	input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, pressure);
	input_report_abs(ts->input, ABS_MT_POSITION_X,x);
	input_report_abs(ts->input, ABS_MT_POSITION_Y, y);
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, 1);
	input_mt_sync(ts->input);
#endif
}

static void process_gslX680_data(struct gsl_ts *ts)
{
	u8 id, touches;
	u16 x, y;
	int i = 0;

	touches = ts->touch_data[ts->dd->touch_index];
	for(i=1;i<=MAX_CONTACTS;i++){
		if(touches == 0)
			id_sign[i] = 0;
		id_state_flag[i] = 0;
	}
	for(i= 0;i < (touches > MAX_FINGERS ? MAX_FINGERS : touches);i ++){
		x = join_bytes( ( ts->touch_data[ts->dd->x_index  + 4 * i + 1] & 0xf),
				ts->touch_data[ts->dd->x_index + 4 * i]);
		y = join_bytes(ts->touch_data[ts->dd->y_index + 4 * i + 1],
				ts->touch_data[ts->dd->y_index + 4 * i ]);
		id = ts->touch_data[ts->dd->id_index + 4 * i] >> 4;

		if(1 <=id && id <= MAX_CONTACTS){
		        record_point(x, y ,id);
			report_data(ts, x_new, y_new, 10, id);
			id_state_flag[id] = 1;
		}
	}
	for(i=1;i<=MAX_CONTACTS;i++){
		if( (0 == touches) || ((0 != id_state_old_flag[i]) && (0 == id_state_flag[i])) ){
		#ifdef REPORT_DATA_ANDROID_4_0
			input_mt_slot(ts->input, i);
			input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input, MT_TOOL_FINGER, false);
		#endif
			id_sign[i]=0;
		}
		id_state_old_flag[i] = id_state_flag[i];
	}
#ifndef REPORT_DATA_ANDROID_4_0
	if(0 == touches){
	        input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(ts->input);
	#ifdef HAVE_TOUCH_KEY
		if(key_state_flag){
        		input_report_key(ts->input, key, 0);
			input_sync(ts->input);
			key_state_flag = 0;
		}
	#endif
	}
#endif
	input_sync(ts->input);
	ts->prev_touches = touches;
}


static void gsl_ts_xy_worker(struct work_struct *work)
{
	int rc;
	u8 read_buf[4] = {0};
	struct gsl_ts *ts = container_of(work, struct gsl_ts,work);

	dprintk(DEBUG_X_Y_INFO,"---gsl_ts_xy_worker---\n");

	/* read data from DATA_REG */
	rc = gsl_ts_read(ts->client, 0x80, ts->touch_data, ts->dd->data_size);
	dprintk(DEBUG_X_Y_INFO,"---touches: %d ---\n",ts->touch_data[0]);
	if (rc < 0) {
		dev_err(&ts->client->dev, "read failed\n");
		goto schedule;
	}

	if (ts->touch_data[ts->dd->touch_index] == 0xff) {
		goto schedule;
	}

	rc = gsl_ts_read( ts->client, 0xbc, read_buf, sizeof(read_buf));
	if (rc < 0) {
		dev_err(&ts->client->dev, "read 0xbc failed\n");
		goto schedule;
	}
	dprintk(DEBUG_X_Y_INFO,"reg %x : %x %x %x %x\n",0xbc, read_buf[3], read_buf[2], read_buf[1], read_buf[0]);
	if (read_buf[3] == 0 && read_buf[2] == 0 && read_buf[1] == 0 && read_buf[0] == 0){
		process_gslX680_data(ts);
	}
	else
	{
		reset_chip(ts->client);
		startup_chip(ts->client);
	}
schedule:
	gls_ts_irq_onoff(1);
}

static u32 gsl_ts_irq(struct gsl_ts *ts)
{
	dprintk(DEBUG_INT_INFO,"==========GSLX680 Interrupt============\n");
        dprintk(DEBUG_INT_INFO,"Enter work\n");
	queue_work(ts->wq, &ts->work);
#ifdef GSL_TIMER
	mod_timer(&ts->gsl_timer, jiffies + msecs_to_jiffies(30));
#endif
	return 0;
}

static int gsl_ts_init_ts(struct i2c_client *client, struct gsl_ts *ts)
{
	struct input_dev *input_device;
	int  rc = 0;

	dprintk(DEBUG_INIT,"[GSLX680] Enter %s\n", __func__);
	ts->dd = &devices[ts->device_id];

	if (ts->device_id == 0) {
		ts->dd->data_size = MAX_FINGERS * ts->dd->touch_bytes + ts->dd->touch_meta_data;
		ts->dd->touch_index = 0;
	}

	ts->touch_data = kzalloc(ts->dd->data_size, GFP_KERNEL);
	if (!ts->touch_data) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return -ENOMEM;
	}

	ts->prev_touches = 0;

	input_device = input_allocate_device();
	if (!input_device) {
		rc = -ENOMEM;
		goto error_alloc_dev;
	}

	ts->input = input_device;
	input_device->name = GSLX680_I2C_NAME;
	input_device->id.bustype = BUS_I2C;
	input_device->dev.parent = &client->dev;
	input_set_drvdata(input_device, ts);

#ifdef REPORT_DATA_ANDROID_4_0
	__set_bit(EV_ABS, input_device->evbit);
	__set_bit(EV_KEY, input_device->evbit);
	__set_bit(EV_REP, input_device->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_device->propbit);
	input_mt_init_slots(input_device, (MAX_CONTACTS+1));
#else
	input_set_abs_params(input_device,ABS_MT_TRACKING_ID, 0, (MAX_CONTACTS+1), 0, 0);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
#endif
#ifdef FOR_TSLIB_TEST
	set_bit(BTN_TOUCH, input_device->keybit);
#endif
#ifdef HAVE_TOUCH_KEY
	input_device->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 1; i <= MAX_KEY_NUM; i++)
		set_bit(i, input_device->keybit);
#endif

	set_bit(ABS_MT_POSITION_X, input_device->absbit);
	set_bit(ABS_MT_POSITION_Y, input_device->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_device->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_device->absbit);

	input_set_abs_params(input_device,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_device,ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_device,ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_device,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	ts->wq = create_singlethread_workqueue("kworkqueue_ts");
	if (!ts->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
		goto error_wq_create;
	}
	flush_workqueue(ts->wq);

	INIT_WORK(&ts->work, gsl_ts_xy_worker);

	rc = input_register_device(input_device);
	if (rc)
		goto error_unreg_device;

	return 0;

error_unreg_device:
	destroy_workqueue(ts->wq);
error_wq_create:
	input_free_device(input_device);
error_alloc_dev:
	kfree(ts->touch_data);
	return rc;
}
static void glsX680_resume_events (struct work_struct *work)
{
	
	gslX680_shutdown_high();
	msleep(10);
        reset_chip(glsX680_i2c);
        startup_chip(glsX680_i2c);
        check_mem_data(glsX680_i2c);
	gls_ts_irq_onoff(1);
}

static int gsl_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
        struct gsl_ts *ts = i2c_get_clientdata(client);
        dprintk(DEBUG_SUSPEND,"%s,start\n",__func__);

        cancel_work_sync(&glsX680_resume_work);
  	flush_workqueue(gslX680_resume_wq);

#ifndef CONFIG_HAS_EARLYSUSPEND
        ts->is_suspended = true;
#endif

#ifdef GSL_TIMER
	dprintk(DEBUG_SUSPEND,"gsl_ts_suspend () : delete gsl_timer\n");
	del_timer(&ts->gsl_timer);
#endif
        if(ts->is_suspended == true ){
		gls_ts_irq_onoff(0);
        	flush_workqueue(gslX680_resume_wq);
        	cancel_work_sync(&ts->work);
        	flush_workqueue(ts->wq);
		gslX680_shutdown_low();
        }
        return 0;

}

static int gsl_ts_resume(struct i2c_client *client)
{
	struct gsl_ts *ts = i2c_get_clientdata(client);

  	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND:%s,start\n",__func__);
	queue_work(gslX680_resume_wq, &glsX680_resume_work);

#ifdef GSL_TIMER
	dprintk(DEBUG_SUSPEND, "gsl_ts_resume () : add gsl_timer\n");
	init_timer(&ts->gsl_timer);
	ts->gsl_timer.expires = jiffies + msecs_to_jiffies(100);
	add_timer(&ts->gsl_timer);
#endif
        ts->is_suspended = true;

	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void gsl_ts_early_suspend(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND:Enter %s\n", __func__);

	cancel_work_sync(&glsX680_resume_work);
	flush_workqueue(gslX680_resume_wq);

#ifdef GSL_TIMER
	dprintk(DEBUG_SUSPEND,"gsl_ts_suspend () : delete gsl_timer\n");
	del_timer(&ts->gsl_timer);
#endif
	ts->is_suspended = false;
	gls_ts_irq_onoff(0);

	cancel_work_sync(&ts->work);
	flush_workqueue(ts->wq);
	gslX680_shutdown_low();
}

static void gsl_ts_late_resume(struct early_suspend *h)
{
	struct gsl_ts *ts = container_of(h, struct gsl_ts, early_suspend);
	dprintk(DEBUG_SUSPEND,"CONFIG_HAS_EARLYSUSPEND: Enter %s\n", __func__);
#ifndef CONFIG_PM
        gsl_ts_resume(ts->client);
#else
        if(ts->is_suspended == false){
                queue_work(gslX680_resume_wq, &glsX680_resume_work);
        }
#endif

	printk("ts->is_suspended:%d\n",ts->is_suspended);
}
#endif

static void glsX680_init_events (struct work_struct *work)
{
	gslX680_chip_init();
	init_chip(glsX680_i2c);
	check_mem_data(glsX680_i2c);

#ifndef GSL_TIMER
	int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)gsl_ts_irq,ts_init);
	if (!int_handle) {
		printk( "gsl_probe: request irq failed\n");
	}
	ctp_set_int_port_rate(config_info.int_number, 1);
	ctp_set_int_port_deb(config_info.int_number, 0x07);
#else
	printk( "add gsl_timer\n");
	init_timer(&ts_init->gsl_timer);
	ts_init->gsl_timer.expires = jiffies + msecs_to_jiffies(500);
	ts_init->gsl_timer.function = &gsl_ts_irq;
	ts_init->gsl_timer.data = (unsigned long)ts_init;
	add_timer(&ts_init->gsl_timer);
#endif
	return;
}

static int __devinit gsl_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct gsl_ts *ts;
	int rc;

	dprintk(DEBUG_INIT,"GSLX680 Enter %s\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (!ts){
	        printk("allocate data fail!\n");
		return -ENOMEM;
	}

	gslX680_wq = create_singlethread_workqueue("gslX680_init");
	if (gslX680_wq == NULL) {
		printk("create gslX680_wq fail!\n");
		return -ENOMEM;
	}

	gslX680_resume_wq = create_singlethread_workqueue("gslX680_resume");
	if (gslX680_resume_wq == NULL) {
		printk("create gslX680_resume_wq fail!\n");
		return -ENOMEM;
	}

        glsX680_i2c = client;
	ts->client = client;
	ts->device_id = id->driver_data;

	ts->is_suspended = false;
	ts->int_pending = false;
	mutex_init(&ts->sus_lock);

	rc = gsl_ts_init_ts(client, ts);
	if (rc < 0) {
		dev_err(&client->dev, "GSLX680 init failed\n");
		goto error_mutex_destroy;
	}

	ts_init = ts;
	queue_work(gslX680_wq, &glsX680_init_work);

	i2c_set_clientdata(ts->client,ts);

	/* create debug attribute */
	rc = device_create_file(&ts->input->dev, &dev_attr_debug_enable);

	gslX680_debug_mask = 0;
	device_enable_async_suspend(&client->dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	ts->early_suspend.suspend = gsl_ts_early_suspend;
	ts->early_suspend.resume = gsl_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	dprintk(DEBUG_INIT,"[GSLX680] End %s\n", __func__);

	return 0;

error_mutex_destroy:
	mutex_destroy(&ts->sus_lock);
	input_free_device(ts->input);
	kfree(ts);
	return rc;
}

static int __devexit gsl_ts_remove(struct i2c_client *client)
{
	struct gsl_ts *ts = i2c_get_clientdata(client);
	printk("==gsl_ts_remove=\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	device_remove_file(&ts->input->dev, &dev_attr_debug_enable);
	device_init_wakeup(&client->dev, 0);
	cancel_work_sync(&ts->work);
	cancel_work_sync(&glsX680_init_work);
	cancel_work_sync(&glsX680_resume_work);
#ifndef GSL_TIMER
	sw_gpio_irq_free(int_handle);
#else
	del_timer(&ts->gsl_timer);
#endif
	destroy_workqueue(ts->wq);
	destroy_workqueue(gslX680_wq);
	destroy_workqueue(gslX680_resume_wq);
	input_unregister_device(ts->input);
	mutex_destroy(&ts->sus_lock);
	kfree(ts->touch_data);
	kfree(ts);

	return 0;
}

static const struct i2c_device_id gsl_ts_id[] = {
	{GSLX680_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gsl_ts_id);

static struct i2c_driver gsl_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = GSLX680_I2C_NAME,
		.owner = THIS_MODULE,
	},
	.probe		= gsl_ts_probe,
	.remove		= __devexit_p(gsl_ts_remove),
	.id_table		= gsl_ts_id,
	.address_list	= normal_i2c,
	.suspend  =  gsl_ts_suspend,
	.resume   =  gsl_ts_resume,
};
static int ctp_get_system_config(void)
{
        char *name[] = {"ctp_para", "ctp_name"};
        
        get_str_para(name, &fwname, 1);
	dprintk(DEBUG_INIT,"%s:fwname:%s\n",__func__,fwname);
        fw_index = gsl_find_fw_idx(fwname);
        if (fw_index == -1) {
        	printk("gslx680: no matched TP firmware(%s)!\n", fwname);
        	return 0;
        }

        twi_id = config_info.twi_id;
        screen_max_x = config_info.screen_max_x;
        screen_max_y = config_info.screen_max_y;
        revert_x_flag = config_info.revert_x_flag;
        revert_y_flag = config_info.revert_y_flag;
        exchange_x_y_flag = config_info.exchange_x_y_flag;
        if((twi_id == 0) || (screen_max_x == 0) || (screen_max_y == 0)){
                printk("%s:read config error!\n",__func__);
                return 0;
        }
        return 1;
}
static int __init gsl_ts_init(void)
{
	int ret = -1;
	dprintk(DEBUG_INIT,"****************************************************************\n");
	
	if (input_fetch_sysconfig_para(&(config_info.input_type))) {
		printk("%s: ctp_fetch_sysconfig_para err.\n", __func__);
		return 0;
	} else {
		ret = input_init_platform_resource(&(config_info.input_type));
		if (0 != ret) {
			printk("%s:ctp_ops.init_platform_resource err. \n", __func__);    
		}
	}
	
        if(config_info.ctp_used == 0){
	        printk("*** ctp_used set to 0 !\n");
	        printk("*** if use ctp,please put the sys_config.fex ctp_used set to 1. \n");
	        return 0;
	}
	if(!ctp_get_system_config()){
                printk("%s:read config fail!\n",__func__);
                return ret;
        }
        
	ctp_wakeup(config_info.wakeup_number, 0, 10);
	gsl_ts_driver.detect = ctp_detect;
	ret = i2c_add_driver(&gsl_ts_driver);
	dprintk(DEBUG_INIT,"****************************************************************\n");
	return ret;
}
static void __exit gsl_ts_exit(void)
{
	printk("==gsl_ts_exit==\n");
	i2c_del_driver(&gsl_ts_driver);
	input_free_platform_resource(&(config_info.input_type));
	return;
}

late_initcall(gsl_ts_init);
module_exit(gsl_ts_exit);
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GSLX680 touchscreen controller driver");
MODULE_AUTHOR("Guan Yuwei, guanyuwei@basewin.com");
MODULE_ALIAS("platform:gsl_ts");
