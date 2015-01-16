/**************************************************************************
*  AW5306_ts.c
* 
*  AW5306 ALLWIN sample code version 1.0
* 
*  Create Date : 2012/06/07
* 
*  Modify Date : 
*
*  Create by   : wuhaijun
* 
**************************************************************************/

#include <linux/i2c.h>
#include <linux/init-input.h>
#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
    #include <linux/pm.h>
    #include <linux/earlysuspend.h>
#endif
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/file.h>
#include <linux/proc_fs.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#include "AW5306_Drv.h"
#include "AW5306_userpara.h"
#define CONFIG_AW5306_MULTITOUCH     (1)
#define FOR_TSLIB_TEST
//#define DEBUG
//#define TOUCH_KEY_SUPPORT
#ifdef TOUCH_KEY_SUPPORT
//#define TOUCH_KEY_LIGHT_SUPPORT

#define TOUCH_KEY_FOR_EVB13
//#define TOUCH_KEY_FOR_ANGDA
#ifdef TOUCH_KEY_FOR_ANGDA
#define TOUCH_KEY_X_LIMIT	(60000)
#define TOUCH_KEY_NUMBER	(4)
#endif
#ifdef TOUCH_KEY_FOR_EVB13
#define TOUCH_KEY_LOWER_X_LIMIT	(848)
#define TOUCH_KEY_HIGHER_X_LIMIT	(852)
#define TOUCH_KEY_NUMBER	(5)
#endif
#endif

#define TOUCH_KEY_NUMBER	(5)
#define AW5306_NAME	"aw5306_ts"//"synaptics_i2c_rmi"//"synaptics-rmi-ts"// 
#define I2C_CTPM_ADDRESS        (0x39)
#define CTP_NAME			AW5306_NAME

struct i2c_dev{
struct list_head list;	
struct i2c_adapter *adap;
struct device *dev;
};
//static  struct i2c_driver AW5306_ts_driver;
//static struct class *i2c_dev_class;
//static LIST_HEAD (i2c_dev_list);
//static DEFINE_SPINLOCK(i2c_dev_list_lock);

static struct i2c_client *this_client;
#ifdef TOUCH_KEY_LIGHT_SUPPORT
static int gpio_light_hdle = 0;
#endif
#ifdef TOUCH_KEY_SUPPORT
static int key_tp  = 0;
static int key_val = 0;
#endif

//specific tp related macro: need be configured for specific tp
//#define CTP_IRQ_NO			(gpio_int_info[0].port_num)
//#define CTP_IRQ_MODE			(NEGATIVE_EDGE)

#define CTP_IRQ_NUMBER                  (config_info.irq_gpio_number)
#define CTP_IRQ_MODE			(TRIG_EDGE_NEGATIVE)

#define TS_RESET_LOW_PERIOD		(1)
#define TS_INITIAL_HIGH_PERIOD		(30)
#define TS_WAKEUP_LOW_PERIOD	(10)
#define TS_WAKEUP_HIGH_PERIOD	(20)
#define TS_POLL_DELAY			(10)	/* ms delay between samples */
#define TS_POLL_PERIOD			(10)	/* ms delay between samples */
//#define SCREEN_MAX_X			(screen_max_x)
//#define SCREEN_MAX_Y			(screen_max_y)
#define SCREEN_MAX_X			(800)
#define SCREEN_MAX_Y			(480)
#define PRESS_MAX			(255)


//static int  store_pin_num;
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;
static u32 int_handle = 0;
static int chip_addr = 0;
static bool is_suspended = false;

static void aw5x06_resume_events(struct work_struct *work);
struct workqueue_struct *aw5x06_resume_wq;
static DECLARE_WORK(aw5x06_resume_work, aw5x06_resume_events);

static void aw5x06_init_events(struct work_struct *work);
struct workqueue_struct *aw5x06_wq;
static DECLARE_WORK(aw5x06_init_work, aw5x06_init_events);

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x39,I2C_CLIENT_END};
static __u32 twi_id = 0;

//extern struct ctp_config_info config_info;
static struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
};

static u32 debug_mask = 0;
#define dprintk(level_mask,fmt,arg...)    if(unlikely(debug_mask & level_mask)) \
        printk("***CTP***"fmt, ## arg)

module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

//long long get_us_time()
//{
//    struct timeval tv;
//    do_gettimeofday(&tv);
//    return (long long)tv.tv_sec * 1000000ll + tv.tv_usec;
//}

/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;

	if(twi_id == adapter->nr) {

		ret = i2c_smbus_read_byte_data(client,0x01);
		dprintk(DEBUG_INIT,"%s: addr = 0x%x,chip_id_value:0x%x\n", __func__, client->addr,ret);
		if(ret != 0xA8) {
			client->addr = 0x38;
			ret = i2c_smbus_read_byte_data(client,0x01);
			dprintk(DEBUG_INIT,"%s: addr = 0x%x,chip_id_value:0x%x\n", __func__, client->addr,ret);
			if(ret != 0xA8) {
				return -ENODEV;
			}
			chip_addr = 0x38;
			strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
    			return 0;
		}
		else {
			chip_addr = 0x39;
			strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
			return 0;
		}
	}else{
		return -ENODEV;
	}
}

void __aeabi_unwind_cpp_pr0(void)
{
};

void __aeabi_unwind_cpp_pr1(void)
{
};

int fts_ctpm_fw_upgrade_with_i_file(void);

#if 0
static struct i2c_dev *i2c_dev_get_by_minor(unsigned index)
{
	struct i2c_dev *i2c_dev;
	spin_lock(&i2c_dev_list_lock);
	
	list_for_each_entry(i2c_dev,&i2c_dev_list,list){
		pr_info("--line = %d ,i2c_dev->adapt->nr = %d,index = %d.\n",__LINE__,i2c_dev->adap->nr,index);
		if(i2c_dev->adap->nr == index){
		     goto found;
		}
	}
	i2c_dev = NULL;
	
found: 
	spin_unlock(&i2c_dev_list_lock);
	
	return i2c_dev ;
}
#endif

struct ts_event {
	int	x[5];
	int	y[5];
	int	pressure;
	int touch_ID[5];
	int touch_point;
	int pre_point;
};

struct AW5306_ts_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif
	struct timer_list touch_timer;
};


/* ---------------------------------------------------------------------
*
*   Focal Touch panel upgrade related driver
*
*
----------------------------------------------------------------------*/

typedef enum
{
	ERR_OK,
	ERR_MODE,
	ERR_READID,
	ERR_ERASE,
	ERR_STATUS,
	ERR_ECC,
	ERR_DL_ERASE_FAIL,
	ERR_DL_PROGRAM_FAIL,
	ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

typedef unsigned char         FTS_BYTE;     //8 bit
typedef unsigned short        FTS_WORD;    //16 bit
typedef unsigned int          FTS_DWRD;    //16 bit
typedef unsigned char         FTS_BOOL;    //8 bit 

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE               0x0


//extern char AW5306_CLB();
//extern void AW5306_CLB_GetCfg();
extern STRUCTCALI       AW_Cali;
extern AW5306_UCF   AWTPCfg;
extern STRUCTBASE		AW_Base;
extern short	Diff[NUM_TX][NUM_RX];
extern short	adbDiff[NUM_TX][NUM_RX];
extern short	AWDeltaData[32];

static unsigned char suspend_flag=0; //0: sleep out; 1: sleep in
static short tp_idlecnt = 0;
static char tp_SlowMode = 1;


int AW_nvram_read(char *filename, char *buf, ssize_t len, int offset)
{	
    struct file *fd;
    //ssize_t ret;
    int retLen = -1;
    
    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);
    
    fd = filp_open(filename, O_RDONLY, 0);
    
    if(IS_ERR(fd)) {
        printk("[AW5306][nvram_read] : failed to open!!\n");
        return -1;
    }
    do{
        if ((fd->f_op == NULL) || (fd->f_op->read == NULL))
    		{
            printk("[AW5306][nvram_read] : file can not be read!!\n");
            break;
    		} 
    		
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
        		    if(fd->f_op->llseek(fd, offset, 0) != offset) {
						printk("[AW5306][nvram_read] : failed to seek!!\n");
					    break;
        		    }
        	  } else {
        		    fd->f_pos = offset;
        	  }
        }    		
        
    		retLen = fd->f_op->read(fd,
    									  buf,
    									  len,
    									  &fd->f_pos);			
    		
    }while(false);
    
    filp_close(fd, NULL);
    set_fs(old_fs);
    
    return retLen;
}

int AW_nvram_write(char *filename, char *buf, ssize_t len, int offset)
{	
    struct file *fd;
    //ssize_t ret;
    int retLen = -1;
        
    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);
    
    fd = filp_open(filename, O_WRONLY|O_CREAT, 0666);
    
    if(IS_ERR(fd)) {
        printk("[AW5306][nvram_write] : failed to open!!\n");
        return -1;
    }
    do{
        if ((fd->f_op == NULL) || (fd->f_op->write == NULL))
    		{
            printk("[AW5306][nvram_write] : file can not be write!!\n");
            break;
    		} /* End of if */
    		
        if (fd->f_pos != offset) {
            if (fd->f_op->llseek) {
        	    if(fd->f_op->llseek(fd, offset, 0) != offset) {
				    printk("[AW5306][nvram_write] : failed to seek!!\n");
                    break;
                }
            } else {
                fd->f_pos = offset;
            }
        }       		
        
        retLen = fd->f_op->write(fd,
                                 buf,
                                 len,
                                 &fd->f_pos);			
    		
    }while(false);
    
    filp_close(fd, NULL);    
    set_fs(old_fs);
   
    return retLen;
}

int AW_I2C_WriteByte(u8 addr, u8 para)
{
	int ret;
	u8 buf[3];
	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 2,
			.buf	= buf,
		},
	};

	buf[0] = addr;
	buf[1] = para;

	ret = i2c_transfer(this_client->adapter, msg, 1);

	return ret;
}

unsigned char AW_I2C_ReadByte(u8 addr)
{
	int ret;
	u8 buf[2] = {0};
	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= buf,
		},
	};

	buf[0] = addr;

	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 2);

	return buf[0];
  
}

unsigned char AW_I2C_ReadXByte( unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret,i;
	u8 rdbuf[512] = {0};
	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rdbuf,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= rdbuf,
		},
	};
	rdbuf[0] = addr;

	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	for(i = 0; i < len; i++) {
		buf[i] = rdbuf[i];
	}

    return ret;
}

unsigned char AW_I2C_WriteXByte( unsigned char *buf, unsigned char addr, unsigned short len)
{
	int ret,i;
	u8 wdbuf[512] = {0};

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= len+1,
			.buf	= wdbuf,
		}
	};
	wdbuf[0] = addr;
	for(i = 0; i < len; i++) {
		wdbuf[i+1] = buf[i];
	}
	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 1);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

    return ret;
}

void AW_Sleep(unsigned int msec)
{
	msleep(msec);
}

static ssize_t AW5306_get_Cali(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_set_Cali(struct device* cd,struct device_attribute *attr, const char *buf, size_t count);
static ssize_t AW5306_get_reg(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_write_reg(struct device* cd,struct device_attribute *attr, const char *buf, size_t count);
static ssize_t AW5306_get_Base(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_Diff(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_adbDiff(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_get_FreqScan(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW5306_Set_FreqScan(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);



static DEVICE_ATTR(cali,S_IRUGO | S_IWUSR, AW5306_get_Cali,AW5306_set_Cali);
static DEVICE_ATTR(readreg,  S_IRUGO | S_IWUSR, AW5306_get_reg,AW5306_write_reg);
static DEVICE_ATTR(base,S_IRUGO | S_IWUSR, AW5306_get_Base,NULL);
static DEVICE_ATTR(diff, S_IRUGO | S_IWUSR, AW5306_get_Diff,NULL);
static DEVICE_ATTR(adbbase,S_IRUGO | S_IWUSR, AW5306_get_adbBase,NULL);
static DEVICE_ATTR(adbdiff,S_IRUGO | S_IWUSR, AW5306_get_adbDiff,NULL);
static DEVICE_ATTR(freqscan,S_IRUGO | S_IWUSR, AW5306_get_FreqScan,AW5306_Set_FreqScan);


static ssize_t AW5306_get_Cali(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;
	//del_timer(&AW5306_ts->touch_timer);  
	//AW5306_CLB();	
	//AW5306_CLB_GetCfg();

	len += snprintf(buf+len, PAGE_SIZE-len,"*****AW5306 Calibrate data*****\n");
	len += snprintf(buf+len, PAGE_SIZE-len,"TXOFFSET:");
	
	for(i=0;i<10;i++) {
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.TXOFFSET[i]);
	}
	
	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "RXOFFSET:");

	for(i=0;i<6;i++) {
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.RXOFFSET[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "TXCAC:");

	for(i=0;i<20;i++) {
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.TXCAC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "RXCAC:");

	for(i=0;i<12;i++) {
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.RXCAC[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	len += snprintf(buf+len, PAGE_SIZE-len,  "TXGAIN:");

	for(i=0;i<20;i++) {
		len += snprintf(buf+len, PAGE_SIZE-len, "0x%02X ", AW_Cali.TXGAIN[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");

	for(i=0;i<AWTPCfg.TX_LOCAL;i++) {
		for(j=0;j<AWTPCfg.RX_LOCAL;j++) {
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d ", AW_Cali.SOFTOFFSET[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	}
	return len;
	
}

static ssize_t AW5306_set_Cali(struct device* cd,struct device_attribute *attr, const char *buf, size_t count)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);	
	unsigned long on_off = simple_strtoul(buf, NULL, 10);

	if(on_off == 1) {
		suspend_flag = 1;
		AW_Sleep(50);	
		TP_Force_Calibration();
		AW5306_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	}
	
	return count;
}


static ssize_t AW5306_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "base: \n");
	for(i=0;i< AWTPCfg.TX_LOCAL;i++) {
		for(j=0;j<AWTPCfg.RX_LOCAL;j++) {
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",AW_Base.Base[i][j]+AW_Cali.SOFTOFFSET[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	}
	
	return len;
}

static ssize_t AW5306_get_Base(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	*(buf+len) = AWTPCfg.TX_LOCAL;
	len++;
	*(buf+len) = AWTPCfg.RX_LOCAL;
	len++;
	
	for(i=0;i< AWTPCfg.TX_LOCAL;i++) {
		for(j=0;j<AWTPCfg.RX_LOCAL;j++) {
			*(buf+len) = (char)(((AW_Base.Base[i][j]+AW_Cali.SOFTOFFSET[i][j]) & 0xFF00)>>8);
			len++;
			*(buf+len) = (char)((AW_Base.Base[i][j]+AW_Cali.SOFTOFFSET[i][j]) & 0x00FF);
			len++;
		}
	}
	return len;

}

static ssize_t AW5306_get_adbDiff(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len, "Diff: \n");
	for(i=0;i< AWTPCfg.TX_LOCAL;i++) {
		for(j=0;j<AWTPCfg.RX_LOCAL;j++) {
			len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",adbDiff[i][j]);
		}
		len += snprintf(buf+len, PAGE_SIZE-len, "\n");
	}
	
	return len;
}

static ssize_t AW5306_get_Diff(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i,j;
	ssize_t len = 0;

	*(buf+len) = AWTPCfg.TX_LOCAL;
	len++;
	*(buf+len) = AWTPCfg.RX_LOCAL;
	len++;
	
	for(i=0;i< AWTPCfg.TX_LOCAL;i++) {
		for(j=0;j<AWTPCfg.RX_LOCAL;j++) {
			*(buf+len) = (char)((adbDiff[i][j] & 0xFF00)>>8);
			len++;
			*(buf+len) = (char)(adbDiff[i][j] & 0x00FF);
			len++;
		}
	}
	return len;
}

static ssize_t AW5306_get_FreqScan(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned char i;
	ssize_t len = 0;

	for(i=0;i< 32;i++) {
		//*(buf+len) = (char)((AWDeltaData[i] & 0xFF00)>>8);
		//len++;
		//*(buf+len) = (char)(AWDeltaData[i] & 0x00FF);
		//len++;
		len += snprintf(buf+len, PAGE_SIZE-len, "%4d, ",AWDeltaData[i]);
	}

	len += snprintf(buf+len, PAGE_SIZE-len,  "\n");
	return len;
}

static ssize_t AW5306_Set_FreqScan(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
	unsigned long Basefreq = simple_strtoul(buf, NULL, 10);

	if(Basefreq < 10) {
		suspend_flag = 1;
		AW_Sleep(50);
		FreqScan(Basefreq);
		AW5306_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	}
	return len;
}

static ssize_t AW5306_get_reg(struct device* cd,struct device_attribute *attr, char* buf)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
	u8 reg_val[128];
	ssize_t len = 0;
	u8 i;
	suspend_flag = 1;	
	AW_Sleep(50);		
	AW_I2C_ReadXByte(reg_val,0,127);

	AW5306_TP_Reinit();
	tp_idlecnt = 0;
	tp_SlowMode = 0;
	suspend_flag = 0;
	data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
	add_timer(&data->touch_timer);
	
	for(i=0;i<0x7F;i++){
		reg_val[0] = AW_I2C_ReadByte(i);
		len += snprintf(buf+len, PAGE_SIZE-len, "reg%02X = 0x%02X, ", i,reg_val[0]);
	}

	return len;
}

static ssize_t AW5306_write_reg(struct device* cd,struct device_attribute *attr, const char *buf, size_t count)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
	int databuf[2];
	
	if(2 == sscanf(buf, "%d %d", &databuf[0], &databuf[1])) { 
		suspend_flag = 1;
		AW_Sleep(50);		
		AW_I2C_WriteByte((u8)databuf[0],(u8)databuf[1]);
		AW5306_TP_Reinit();
		tp_idlecnt = 0;
		tp_SlowMode = 0;
		suspend_flag = 0;
		data->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		add_timer(&data->touch_timer);
	}
	else {
		printk("invalid content: '%s', length = %d\n", buf, count);
	}
	return count; 
}

static int AW5306_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);
	
	err = device_create_file(dev, &dev_attr_cali);
	err = device_create_file(dev, &dev_attr_readreg);
	err = device_create_file(dev, &dev_attr_base);
	err = device_create_file(dev, &dev_attr_diff);
	err = device_create_file(dev, &dev_attr_adbbase);
	err = device_create_file(dev, &dev_attr_adbdiff);
	err = device_create_file(dev, &dev_attr_freqscan);
	return err;
}

static void AW5306_ts_release(void)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
#ifdef CONFIG_AW5306_MULTITOUCH	
	#ifdef TOUCH_KEY_SUPPORT
	if(1 == key_tp){
		if(key_val == 1){
			input_report_key(data->input_dev, KEY_MENU, 0);
			input_sync(data->input_dev);  
        }
        else if(key_val == 2){
			input_report_key(data->input_dev, KEY_BACK, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 2   upupupupupu===++=\n");     
        }
        else if(key_val == 3){
			input_report_key(data->input_dev, KEY_SEARCH, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 3   upupupupupu===++=\n");     
        }
        else if(key_val == 4){
			input_report_key(data->input_dev, KEY_HOMEPAGE, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 4   upupupupupu===++=\n");     
        }
        else if(key_val == 5){
			input_report_key(data->input_dev, KEY_VOLUMEDOWN, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 5   upupupupupu===++=\n");     
        }
        else if(key_val == 6){
			input_report_key(data->input_dev, KEY_VOLUMEUP, 0);
			input_sync(data->input_dev);  
		//	printk("===KEY 6   upupupupupu===++=\n");     
        }
//		input_report_key(data->input_dev, key_val, 0);
		//printk("Release Key = %d\n",key_val);		
		//printk("Release Keyi+++++++++++++++++++++++++++++\n");		
	} else{
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	}
	#else
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	#endif

#else
	input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	input_report_key(data->input_dev, BTN_TOUCH, 0);
#endif
	
	input_sync(data->input_dev);
	return;

}

static int AW5306_read_data(void)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	int Pevent;
	int i = 0;
	
	AW5306_TouchProcess();
	
	//memset(event, 0, sizeof(struct ts_event));
	event->touch_point = AW5306_GetPointNum();

	for(i=0;i<event->touch_point;i++)
	{
		AW5306_GetPoint(&event->x[i],&event->y[i],&event->touch_ID[i],&Pevent,i);
		swap(event->x[i], event->y[i]);
		//printk("AW5306key%d = %d,%d,%d \n",i,event->x[i],event->y[i],event->touch_ID[i] );
	}
    	
	if (event->touch_point == 0) 
	{
		if(tp_idlecnt <= AWTPCfg.FAST_FRAME*5)
		{
			tp_idlecnt++;
		}
		if(tp_idlecnt > AWTPCfg.FAST_FRAME*5)
		{
			tp_SlowMode = 1;
		}
		
		if (event->pre_point != 0)
		{
		    AW5306_ts_release();
			event->pre_point = 0;
		}
		return 1; 
	}
	else
	{
		tp_SlowMode = 0;
		tp_idlecnt = 0;
    event->pre_point = event->touch_point;
     
#ifdef CONFIG_AW5306_MULTITOUCH
  //  printk("tmp =%d\n",store_pin_num);
 //  printk("touch_point=%d\n",event->touch_point);
	switch (event->touch_point) {
	case 5:
		if(0 == exchange_x_y_flag){
			swap(event->x[4], event->y[4]);
		}
		if(1 == revert_x_flag){
			event->x[4] = SCREEN_MAX_X - event->x[4];    
		}
		if(1 == revert_y_flag){
			event->y[4] = SCREEN_MAX_Y - event->y[4];   
		}
		
	case 4:
		if(0 == exchange_x_y_flag){
			swap(event->x[3], event->y[3]);
		}
		if(1 == revert_x_flag){
			event->x[3] = SCREEN_MAX_X - event->x[3];
		}
		if(1 == revert_y_flag){
			event->y[3] = SCREEN_MAX_Y - event->y[3];
		}
		
	case 3:
		if(0 == exchange_x_y_flag){
			swap(event->x[2], event->y[2]);
		}
		if(1 == revert_x_flag){
			event->x[2] = SCREEN_MAX_X - event->x[2];
		}
		if(1 == revert_y_flag){
			event->y[2] = SCREEN_MAX_Y - event->y[2];
		}
		
	case 2:
		if(0 == exchange_x_y_flag){
			swap(event->x[1], event->y[1]);
		}
		if(1== revert_x_flag){
			event->x[1] = SCREEN_MAX_X - event->x[1];
		}
		if(1 == revert_y_flag){
			event->y[1] = SCREEN_MAX_Y - event->y[1];
		}
		
	case 1:
#ifdef TOUCH_KEY_FOR_ANGDA
		if(event->x[0] < TOUCH_KEY_X_LIMIT)
		{
			if(1 == revert_x_flag){
				event->x[0] = SCREEN_MAX_X - event->x[0];
			}
			if(1 == revert_y_flag){
				event->y[0] = SCREEN_MAX_Y - event->y[0];
			}
			if(0 == exchange_x_y_flag){
				swap(event->x[0], event->y[0]);
			}
		}
#elif defined(TOUCH_KEY_FOR_EVB13)
		if((event->x[0] > TOUCH_KEY_LOWER_X_LIMIT)&&(event->x[0]<TOUCH_KEY_HIGHER_X_LIMIT))
		{
		    //   printk("event->x=%d\n",event->x[0]);
		   
		if(0 == exchange_x_y_flag){
				swap(event->x[0], event->y[0]);
			}
			  // printk("event->x=%d,y=%d\n",event->x[0],event->y[0]);
		
			if(1 == revert_x_flag){
				event->x[0] = SCREEN_MAX_X - event->x[0];
			}
			if(1 == revert_y_flag){
				event->y[0] = SCREEN_MAX_Y - event->y[0];
			}
			}
			 
#else
            if(0 == exchange_x_y_flag){
			swap(event->x[0], event->y[0]);
		}
		//printk("event->x=%d,y=%d\n",event->x[0],event->y[0]); 
		 
		if(1 == revert_x_flag){
			event->x[0] = SCREEN_MAX_X - event->x[0];
		}
		if(1== revert_y_flag){
			event->y[0] = SCREEN_MAX_Y - event->y[0];
		}
		
		//printk("x=%d\n",event->x[0]);
#endif

		break;
	default:
		return -1;
	}
#else
#endif
	event->pressure = 200;

	dev_dbg(&this_client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
	event->x[0], event->y[0], event->x[1], event->y[1]);
		

    return 0;
		}
}

#ifdef TOUCH_KEY_LIGHT_SUPPORT
static void AW5306_lighting(void)
{   
	ctp_key_light(1,15);
	return;
}
#endif

static void AW5306_report_multitouch(void)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;

#ifdef TOUCH_KEY_SUPPORT
	if(1 == key_tp){
		return;
	}
#endif

	switch(event->touch_point) {
	case 5:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[4]);	
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[4]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[4]);
		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
		dprintk(DEBUG_INIT,"=++==x5 = %d,y5 = %d ====\n",event->x[4],event->y[4]);
	case 4:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[3]);	
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[3]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[3]);
		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
		dprintk(DEBUG_INIT,"===x4 = %d,y4 = %d ====\n",event->x[3],event->y[3]);
	case 3:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[2]);	
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[2]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[2]);
		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
		dprintk(DEBUG_INIT,"===x3 = %d,y3 = %d ====\n",event->x[2],event->y[2]);
	case 2:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[1]);	
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[1]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[1]);
		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
	     	dprintk(DEBUG_INIT,"===x2 = %x,y2 = %x====\n",event->x[1],event->y[1]);
	case 1:
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->touch_ID[0]);	
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
		input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x[0]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y[0]);
		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
		input_mt_sync(data->input_dev);
	     	dprintk(DEBUG_INIT,"===x1 = %d,y1 = %d====\n",event->x[0],event->y[0]);
		break;
	default:
		dprintk(DEBUG_INIT,"==touch_point default =\n");
		break;
	}
	
	input_sync(data->input_dev);
	dev_dbg(&this_client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
		event->x[0], event->y[0], event->x[1], event->y[1]);
	return;
}

#ifndef CONFIG_AW5306_MULTITOUCH
static void AW5306_report_singletouch(void)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}
	input_report_key(data->input_dev, BTN_TOUCH, 1);
	input_sync(data->input_dev);
	dprintk(DEBUG_INIT,"%s: 1:%d %d 2:%d %d \n", __func__,
		event->x1, event->y1, event->x2, event->y2);

	return;
}
#endif

#ifdef TOUCH_KEY_SUPPORT
static void AW5306_report_touchkey(void)
{
	struct AW5306_ts_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	//printk("x=%d===Y=%d\n",event->x[0],event->y[0]);

#ifdef TOUCH_KEY_FOR_ANGDA
	if((1==event->touch_point)&&(event->x1 > TOUCH_KEY_X_LIMIT)){
		key_tp = 1;
		if(event->x1 < 40){
			key_val = 1;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);  
			dprintk(DEBUG_INIT,"===KEY 1====\n");
		}else if(event->y1 < 90) {
			key_val = 2;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
			dprintk(DEBUG_INIT,"===KEY 2 ====\n");
		}else {
			key_val = 3;
			input_report_key(data->input_dev, key_val, 1);
			input_sync(data->input_dev);     
			dprintk(DEBUG_INIT,"===KEY 3====\n");	
		}
	} else{
		key_tp = 0;
	}
#endif
#ifdef TOUCH_KEY_FOR_EVB13
	if((1==event->touch_point)&&((event->y[0] > 510)&&(event->y[0]<530)))
	{
		if(key_tp != 1)
		{
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_sync(data->input_dev);
		}
		else
		{
			//printk("===KEY touch ++++++++++====++=\n");     
		
			if(event->x[0] < 90){
				key_val = 1;
				input_report_key(data->input_dev, KEY_MENU, 1);
				input_sync(data->input_dev);  
			dprintk(DEBUG_INIT,"===KEY 1===++=\n");     
			}else if((event->x[0] < 230)&&(event->x[0]>185)){
				key_val = 2;
				input_report_key(data->input_dev, KEY_BACK, 1);
				input_sync(data->input_dev);     
			dprintk(DEBUG_INIT,"===KEY 2 ====\n");
			}else if((event->x[0] < 355)&&(event->x[0]>305)){
				key_val = 3;
				input_report_key(data->input_dev, KEY_SEARCH, 1);
				input_sync(data->input_dev);     
			dprintk(DEBUG_INIT,"===KEY 3====\n");
			}else if ((event->x[0] < 497)&&(event->x[0]>445))	{
				key_val = 4;
				input_report_key(data->input_dev, KEY_HOMEPAGE, 1);
				input_sync(data->input_dev);     
			dprintk(DEBUG_INIT,"===KEY 4====\n");	
			}else if ((event->x[0] < 615)&&(event->x[0]>570))	{
				key_val = 5;
				input_report_key(data->input_dev, KEY_VOLUMEDOWN, 1);
				input_sync(data->input_dev);     
			dprintk(DEBUG_INIT,"===KEY 5====\n");	
			}else if ((event->x[0] < 750)&&(event->x[0]>705))	{
				key_val = 6;
				input_report_key(data->input_dev, KEY_VOLUMEUP, 1);
				input_sync(data->input_dev);     
			dprintk(DEBUG_INIT,"===KEY 6====\n");	
			}
		}
		key_tp = 1;
	}
	else
	{
		key_tp = 0;
	}
#endif

#ifdef TOUCH_KEY_LIGHT_SUPPORT
	AW5306_lighting();
#endif
	return;
}
#endif

static void AW5306_report_value(void)
{

	dprintk(DEBUG_INIT,"==AW5306_report_value =\n");

#ifdef CONFIG_AW5306_MULTITOUCH
	AW5306_report_multitouch();
#else	
	AW5306_report_singletouch();
#endif	

#ifdef TOUCH_KEY_SUPPORT
	printk("touch_key_support++++++++++++++++\n");
	AW5306_report_touchkey();
#endif
	return;
}

static void AW5306_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	if(suspend_flag != 1) {
		ret = AW5306_read_data();
         	if (ret == 0) {
			AW5306_report_value();
		}
	}
	else {
		AW5306_Sleep(); 
	}
}
#ifdef INTMODE
static u32 AW5306_ts_interrupt(struct ft5x_ts_data *ft5x_ts)
{
	dprintk(DEBUG_INT_INFO,"==========AW5306_ts TS Interrupt============\n"); 
	if (!work_pending(&AW5306_ts->pen_event_work)) {
		dprintk(DEBUG_INT_INFO,"Enter work\n");
		queue_work(AW5306_ts->ts_workqueue, &AW5306_ts->pen_event_work);
	}
	return 0;
}
#else
void AW5306_tpd_polling(unsigned long s)
 {
	struct AW5306_ts_data *AW5306_ts = i2c_get_clientdata(this_client);
 	
 	if (!work_pending(&AW5306_ts->pen_event_work)) {
		queue_work(AW5306_ts->ts_workqueue, &AW5306_ts->pen_event_work);
	}
    
	if(suspend_flag != 1)
	{
	#ifdef AUTO_RUDUCEFRAME
		if(tp_SlowMode)
		{  	
			AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.SLOW_FRAME;
		}
		else
		{
			AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
		}
	#else
		AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
	#endif
		add_timer(&AW5306_ts->touch_timer);
	}
 }
#endif

static void aw5x06_resume_events (struct work_struct *work)
{
	struct AW5306_ts_data *AW5306_ts = i2c_get_clientdata(this_client);
	printk("Enter aw5x06_resume_events!!!!\n");

	if(STANDBY_WITH_POWER_OFF == standby_level){
		msleep(10);
                AW5306_User_Cfg1();
		AW5306_TP_Reinit();
		tp_SlowMode = 0;
	        AW5306_read_data();
                AW5306_report_value();
                init_timer(&AW5306_ts->touch_timer);
		dprintk(DEBUG_INIT,"AW5306 POWER OFF, WAKE UP!!!");	
	}
	else {
		AW5306_TP_Reinit();	
		tp_SlowMode = 1;
		dprintk(DEBUG_INIT,"AW5306 POWER ON, WAKE UP!!!\n");
	}
	tp_idlecnt = 0;	
	AW5306_ts->touch_timer.expires = jiffies + HZ/AWTPCfg.FAST_FRAME;
	add_timer(&AW5306_ts->touch_timer);
}

static int AW5306_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct AW5306_ts_data *AW5306_ts = i2c_get_clientdata(this_client);

#ifndef CONFIG_HAS_EARLYSUSPEND
        is_suspended = true;
#endif
	if(is_suspended == true) {
		cancel_work_sync(&aw5x06_resume_work);
		flush_workqueue(aw5x06_resume_wq);   
		printk("AW5306 power off!!!\n");
		del_timer(&AW5306_ts->touch_timer); 
	}
	suspend_flag = 1;
	return 0;
}
static int AW5306_resume(struct i2c_client *client)
{ 
	queue_work(aw5x06_resume_wq, &aw5x06_resume_work); 
	is_suspended = true;  
	suspend_flag = 0;    
	return 0;		
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void AW5306_ts_suspend(struct early_suspend *handler)
{
	struct AW5306_ts_data *AW5306_ts = i2c_get_clientdata(this_client);
	dprintk(DEBUG_INIT,"AW5306_ts_suspend: write AW53060X_REG_PMODE .\n");	
	//AW5306_Sleep();
	cancel_work_sync(&aw5x06_resume_work);
	flush_workqueue(aw5x06_resume_wq);   
	printk("AW5306 SLEEP!!!\n");
	del_timer(&AW5306_ts->touch_timer);  
	is_suspended = false;
	suspend_flag = 1;
}

static void AW5306_ts_resume(struct early_suspend *handler)
{
	dprintk(DEBUG_INIT,"==AW5306_ts_resume== \n");
	if(is_suspended == false) {
		queue_work(aw5x06_resume_wq, &aw5x06_resume_work);
	}
	suspend_flag = 0;
}
#endif  //CONFIG_HAS_EARLYSUSPEND

static void aw5x06_init_events (struct work_struct *work)
{ 
	dprintk(DEBUG_INIT,"====%s begin=====.  \n", __func__);
	AW5306_TP_Init();
}

static int AW5306_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct AW5306_ts_data *AW5306_ts;
	struct input_dev *input_dev;
	//struct device *dev;
	//struct i2c_dev *i2c_dev;
	int err = 0;
//	long long time1 = 0;
//      long long time2 = 0;
//	time1 = get_us_time();

#ifdef TOUCH_KEY_SUPPORT
	int i = 0;
#endif

	dprintk(DEBUG_INIT,"====%s begin=====.  \n", __func__);    
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	AW5306_ts = kzalloc(sizeof(*AW5306_ts), GFP_KERNEL);
	if (!AW5306_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	this_client = client;
	
	client->addr =chip_addr;
	this_client->addr = client->addr;
	
	i2c_set_clientdata(client, AW5306_ts);

#ifdef TOUCH_KEY_LIGHT_SUPPORT
	//gpio_light_hdle = gpio_request_ex("ctp_para", "ctp_light");
#endif

	INIT_WORK(&AW5306_ts->pen_event_work, AW5306_ts_pen_irq_work);
	AW5306_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));

	if (!AW5306_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	AW5306_ts->input_dev = input_dev;

#ifdef CONFIG_AW5306_MULTITOUCH
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);	
#ifdef FOR_TSLIB_TEST
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_MT_TRACKING_ID, 0, 4, 0, 0);
#ifdef TOUCH_KEY_SUPPORT
	input_set_capability(input_dev,EV_KEY,KEY_BACK);
	input_set_capability(input_dev,EV_KEY,KEY_MENU);
	input_set_capability(input_dev,EV_KEY,KEY_HOMEPAGE);
	input_set_capability(input_dev,EV_KEY,KEY_SEARCH);
	input_set_capability(input_dev,EV_KEY,KEY_VOLUMEDOWN);
	input_set_capability(input_dev,EV_KEY,KEY_VOLUMEUP);
	set_bit(KEY_MENU, input_dev->keybit);
	key_tp = 0;
	input_dev->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 1; i < TOUCH_KEY_NUMBER; i++)
		set_bit(i, input_dev->keybit);
#endif
#else
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			     ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	input_dev->name		= CTP_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"AW5306_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	printk("==register_early_suspend =\n");
	AW5306_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	AW5306_ts->early_suspend.suspend = AW5306_ts_suspend;
	AW5306_ts->early_suspend.resume	= AW5306_ts_resume;
	register_early_suspend(&AW5306_ts->early_suspend);
#endif

#ifdef CONFIG_AW5306X_MULTITOUCH
	dprintk(DEBUG_INIT,"CONFIG_AW5306_MULTITOUCH is defined. \n");
#endif

#ifdef INTMODE
	int_handle = sw_gpio_irq_request(CTP_IRQ_NUMBER,CTP_IRQ_MODE,(peint_handle)AW5306_ts_interrupt,AW5306_ts);
	if (!int_handle) {
		printk("AW5306_ts_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
#else
	AW5306_create_sysfs(client);

	aw5x06_wq = create_singlethread_workqueue("aw5x06_init");
	if (aw5x06_wq == NULL) {
		printk("create aw5x06_wq fail!\n");
		return -ENOMEM;
	}
	queue_work(aw5x06_wq, &aw5x06_init_work);

	AW5306_ts->touch_timer.function = AW5306_tpd_polling;
	AW5306_ts->touch_timer.data = 0;
	init_timer(&AW5306_ts->touch_timer);
	AW5306_ts->touch_timer.expires = jiffies + HZ*5;
	add_timer(&AW5306_ts->touch_timer);
#endif
	aw5x06_resume_wq = create_singlethread_workqueue("aw5x06_resume");
	if (aw5x06_resume_wq == NULL) {
		printk("create aw5x06_resume_wq fail!\n");
		return -ENOMEM;
	}
	
	//time2 = get_us_time();
	//dprintk(DEBUG_INIT,"**ctp***func:%s,line:%d,(time2-time1):%llu\n", __func__, __LINE__, time2-time1);
	dprintk(DEBUG_INIT,"==%s over =\n", __func__);	
	return 0;

//exit_irq_request_failed:
	sw_gpio_irq_free(int_handle);
	cancel_work_sync(&AW5306_ts->pen_event_work);
	destroy_workqueue(AW5306_ts->ts_workqueue);
exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
exit_create_singlethread:
	i2c_set_clientdata(client, NULL);
	kfree(AW5306_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int __devexit AW5306_ts_remove(struct i2c_client *client)
{

	struct AW5306_ts_data *AW5306_ts = i2c_get_clientdata(client);
	
	printk("==AW5306_ts_remove=\n");
#ifdef INTMODE
	sw_gpio_irq_free(int_handle);
#else
	del_timer(&AW5306_ts->touch_timer);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&AW5306_ts->early_suspend);
#endif
	input_unregister_device(AW5306_ts->input_dev);
	input_free_device(AW5306_ts->input_dev);
	cancel_work_sync(&AW5306_ts->pen_event_work);
	destroy_workqueue(AW5306_ts->ts_workqueue);
	kfree(AW5306_ts);
    
	i2c_set_clientdata(client, NULL);
	//input_free_platform_resource(&(config_info.input_type));

	return 0;

}

static const struct i2c_device_id AW5306_ts_id[] = {
	{ CTP_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, AW5306_ts_id);

static struct i2c_driver AW5306_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.probe		= AW5306_ts_probe,
	.remove		= __devexit_p(AW5306_ts_remove),
	.id_table	= AW5306_ts_id,
	.suspend        = AW5306_suspend,
	.resume         = AW5306_resume,
	.driver	= {
		.name	= CTP_NAME,
		.owner	= THIS_MODULE,
	},
	.address_list	= normal_i2c,
};

static int ctp_get_system_config(void)
{   
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

static int __init AW5306_ts_init(void)
{ 
	int ret = -1;
//	long long time1 = 0;
//        long long time2 = 0;
//	time1 = get_us_time();

	dprintk(DEBUG_INIT,"*********************%s begin**********************\n",__func__);

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

	ctp_wakeup(config_info.wakeup_number,1,TS_WAKEUP_LOW_PERIOD);  
	//time2 = get_us_time();
	//dprintk(DEBUG_INIT,"**ctp***func:%s,line:%d,(time2-time1):%llu\n", __func__, __LINE__, time2-time1);

	AW5306_ts_driver.detect = ctp_detect;
	ret = i2c_add_driver(&AW5306_ts_driver);

	//time2 = get_us_time();
	//dprintk(DEBUG_INIT,"**ctp***func:%s,line:%d,(time2-time1):%llu\n", __func__, __LINE__, time2-time1);

	dprintk(DEBUG_INIT,"*********************%s end**********************\n",__func__);
	return ret;
}

static void __exit AW5306_ts_exit(void)
{
	printk("==AW5306_ts_exit==\n");
	i2c_del_driver(&AW5306_ts_driver);
	input_free_platform_resource(&(config_info.input_type));
}

late_initcall(AW5306_ts_init);
module_exit(AW5306_ts_exit);

MODULE_AUTHOR("<whj@AWINIC.com>");
MODULE_DESCRIPTION("AWINIC AW5306 TouchScreen driver");
MODULE_LICENSE("GPL");

