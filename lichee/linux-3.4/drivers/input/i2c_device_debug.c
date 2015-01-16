#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>


#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>

#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/device.h>


#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#include <mach/gpio.h> 
#include <linux/init-input.h>

struct device_struct {
	u32 	twi_id;	        /* I2C group number */
	u32 	dev_addr;	/* Device addr */
};

struct read_struct {
	u32 	number;         /* Read element count */
	u32	reg_addr;       /* Read address  */
};


struct write_struct {
	u32 	        reg_addr;       /* Write address */       
	uint8_t	        val[512];       /* Value to write*/
	u32             number;         /* write element count */
	uint16_t        reg_len;
};

static struct device_struct device_para;
static struct read_struct read_para;
static struct  write_struct write_para;
static char para_name[32];
static int config_flag;
struct i2c_client *client = NULL;
struct i2c_adapter *adap = NULL ;

static bool __addr_valid(u32 addr)
{
	if(addr >= 0 && addr < 0xffff)
			return true;
	return false;
}

static int __parse_str(char *str, u32 *out)
{
        if ((str == NULL) || (out == NULL)) {
                printk(KERN_ERR "%s err, line %d\n", __func__, __LINE__);
                return -EINVAL;
        }
        
        if(strict_strtoul(str, 16, (long unsigned int *)out)) {
                printk(KERN_ERR "%s err, line %d\n", __func__, __LINE__);
                printk("str:%s\n", str);
                return -EINVAL;
        }
        
        if(!__addr_valid(*out) ) 
                return -EINVAL;
        
        return 0;
}

static char * __first_str_to_u32(char *pstr, char ch, u32 *pout)
{
	char 	*pret = NULL;
	char 	str_tmp[260] = {0};

	pret = strchr(pstr, ch);
	if(NULL != pret) {
		memcpy(str_tmp, pstr, pret - pstr);
		if(strict_strtoul(str_tmp, 10, (long unsigned int *)pout)) {
			printk(KERN_ERR "%s err, line %d\n", __func__, __LINE__);
			return NULL;
		}
	} else
		*pout = 0;

	return pret;
}

static int __parse_device_str(const char *buf, size_t size, u32 *twi_id, u32 *dev_addr)
{
	char 	*ptr = (char *)buf;

	if(NULL == strchr(ptr, ',')) { 
	        return -EINVAL;
	}

	ptr = __first_str_to_u32(ptr, ',', twi_id);
	if(NULL == ptr)
		return -EINVAL;

	ptr += 1;
	if(strict_strtoul(ptr, 16, (long unsigned int *)dev_addr))
		return -EINVAL;

	return 0;
}



static int __parse_write_str(const char *buf)
{
	int     i = 0;
	u32     val = 0;
	char 	*ptr = (char *)buf;
	char    tmp[32];
	
	write_para.number = 0;

	if(NULL == ptr) { 
	        return -EINVAL;
	}
	
	while(*ptr != '\0') {
        	if(*ptr == ','){
        	        tmp[i] = '\0';
        	        __parse_str(tmp, &val);
                        
        		if(write_para.number == 0) {
        		        write_para.reg_addr = val;
        		        
        		        if(val & 0xff00) {
        		                 write_para.reg_len = 2;
        		                 write_para.val[write_para.number++] = val >> 8;
                                         write_para.val[write_para.number] = val & 0x00ff;
        		                
        		        }else {
        		                write_para.reg_len = 1;
        		                write_para.val[write_para.number] = val;
        		        }
        		}else {
        		        write_para.val[write_para.number] = val;
        		}
        		
	                i = 0;
	                memset(&tmp,0,sizeof(tmp));
        		write_para.number++;
        	}else {
        	        tmp[i++] = *ptr ;
        	}
        	ptr += 1;
        }
        
        tmp[i] = '\0';
        __parse_str(tmp, &val);
        write_para.val[write_para.number] = val;
        

	return 0;
}


static ssize_t i2c_device_show(struct class *class, struct class_attribute *attr, char *buf)
{
	ssize_t cnt = 0;
        cnt += sprintf(buf, "twi_id:%d\n",device_para.twi_id);
	cnt += sprintf(buf + cnt, "dev_addr:0x%04x\n",device_para.dev_addr);
	return cnt;

}
static ssize_t i2c_device_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	u32 	twi_id = 0, dev_addr = 0;

	if(0 != __parse_device_str((char *)buf, size, &twi_id, &dev_addr)) {
		printk(KERN_ERR "%s err, invalid para, __parse_device_str failed\n", __func__);
		goto err;
	}
	if(!__addr_valid(twi_id) || !__addr_valid(dev_addr)) {
		printk(KERN_ERR "%s err, invalid para, the addr is not reg\n", __func__);
		goto err;
	}

	device_para.twi_id = twi_id;
	device_para.dev_addr = dev_addr;
	
        client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!client)
                return -ENOMEM;
                
        adap = i2c_get_adapter(twi_id);
        client->adapter = adap;
        client->addr = dev_addr;	
	
	printk(KERN_INFO "%s: get start_reg 0x%08x, end_reg 0x%08x\n", __func__, twi_id, dev_addr);
	return size;
err:
	device_para.twi_id = device_para.dev_addr = -1;
	return -EINVAL;
}



static ssize_t i2c_write_show(struct class *class, struct class_attribute *attr, char *buf)
{
        ssize_t cnt = 0;
        int i = 0;
        cnt += sprintf(buf, "first write reg addr is:0x%04x\n",write_para.reg_addr);
        cnt += sprintf(buf + cnt, "write reg        write val    \n");
        
        for(i = write_para.reg_len; i <= write_para.number; i++){
                cnt += sprintf(buf + cnt, "0x%04x           0x%04x\n",
                        write_para.reg_addr + i - write_para.reg_len, write_para.val[i]);        
        }
	
	return cnt;

}
static ssize_t i2c_write_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
        
        if(0 != __parse_write_str((char *)buf)) {
		printk(KERN_ERR "%s err, invalid para, __parse_write_str failed\n", __func__);
		goto err;
	}
	
	 if(client == NULL) {
	        printk("clinet is NULL,please write device first!\n"); 
	        goto err;       
	}
	
	if(write_para.number == 0) {
                printk( "write data is NULL!\n"); 
	        goto err;       
	}
	
	
	sw_i2c_write_bytes(client, write_para.val, write_para.number + 1);

        return size;
        
err:
	memset(&write_para,0,sizeof(write_para));
	return -EINVAL;
}

static ssize_t i2c_read_show(struct class *class, struct class_attribute *attr, char *buf)
{
        ssize_t cnt = 0;
        u8 data[512] = {0};
        int i = 0;
        int reg_len = 0;
        cnt += sprintf(buf, "number:%d\n",read_para.number);
	cnt += sprintf(buf + cnt, "reg_addr:0x%04x\n",read_para.reg_addr);
	
	if(client == NULL) {
	        cnt += sprintf(buf + cnt, "clinet is NULL,please write device first!\n"); 
	        return cnt;       
	}
	
	if(read_para.reg_addr & 0xff00){
	        data[0] = read_para.reg_addr >> 8;
                data[1] = read_para.reg_addr & 0x00ff;
                reg_len = 2;
                i2c_read_bytes_addr16(client, data, read_para.number + reg_len);
        }else{
                reg_len = 1;
                data[0] = read_para.reg_addr;
                i2c_read_bytes_addr8(client, data, read_para.number + reg_len);
        }
        
        cnt += sprintf(buf + cnt, "reg_addr           val        \n");
        
        for(i = reg_len; i < (read_para.number + reg_len); i++) {
                cnt += sprintf(buf + cnt, "0x%04x             0x%04x\n",(read_para.reg_addr + i - reg_len), data[i]);
        }
        	
	return cnt;

}
static ssize_t i2c_read_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
        u32 	number = 0, reg_addr = 0;

	if(0 != __parse_device_str((char *)buf, size, &number, &reg_addr)) {
		printk(KERN_ERR "%s err, invalid para, __parse_device_str failed\n", __func__);
		goto err;
	}
	if(!__addr_valid(number) || !__addr_valid(reg_addr)) {
		printk(KERN_ERR "%s err, invalid para, the addr is not reg\n", __func__);
		goto err;
	}

	read_para.number = number;
	read_para.reg_addr = reg_addr;
	printk(KERN_INFO "%s: get start_reg 0x%08x, end_reg 0x%08x\n", __func__, number, reg_addr);
	return size;
err:
	read_para.number = read_para.reg_addr = -1;
	return -EINVAL;       
}
static ssize_t get_sysconfig_para_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
       memset(&para_name, 0 , sizeof(para_name));
       strcpy(para_name, buf);
       para_name[strlen(para_name) - 1] = '\0';
       printk("%s: para key name:%s,strlen:%d\n", __func__, para_name, strlen(para_name));
	return size;
       
}

static ssize_t get_sysconfig_para_show(struct class *class, struct class_attribute *attr, char *buf)
{        
        ssize_t cnt = 0;
        int ret = -1;
        ret = script_dump_mainkey(para_name);
        cnt += sprintf(buf + cnt, "key name:%s\n", para_name); 
        if(!ret)
                cnt += sprintf(buf + cnt, "print the sysconfig para success!\n"); 
        else
                cnt += sprintf(buf + cnt, "print the sysconfig para fail!\n");             
        	
	return cnt;

}


static ssize_t get_config_para_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
        unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	
	if (error)
		return error;
        config_flag = data;
        printk("*****%s:config_para:%d\n", __func__, config_flag);
	return size;
       
}

static ssize_t get_config_para_show(struct class *class, struct class_attribute *attr, char *buf)
{        
        ssize_t cnt = 0;
        struct ctp_config_info data = {
                .input_type = CTP_TYPE,
                };
        struct sensor_config_info gsensor_data = {
                .input_type = GSENSOR_TYPE,
                };
        struct sensor_config_info gyr_data = {
                .input_type = GYR_TYPE,
                };
        struct sensor_config_info ls_data = {
                .input_type = LS_TYPE,
                };
        struct sensor_config_info compass_data = {
                .input_type = COMPASS_TYPE,
                };
       struct ir_config_info ir_data = {
                .input_type = IR_TYPE,
                };
                
        switch(config_flag) {
        case 0:
                input_fetch_sysconfig_para(&(data.input_type));
	        cnt += sprintf(buf + cnt, "ctp_used : %d \n", data.ctp_used); 	
	        cnt += sprintf(buf + cnt, "ctp_twi_id : %d \n", data.twi_id);
	        cnt += sprintf(buf + cnt, "ctp_screen_max_x: %d \n", data.screen_max_x); 
	        cnt += sprintf(buf + cnt, "ctp_screen_max_y: %d \n", data.screen_max_y); 
	        cnt += sprintf(buf + cnt, "ctp_exchange_x_y_flag: %d \n", data.exchange_x_y_flag);
	        cnt += sprintf(buf + cnt, "int number: %d \n", data.int_number);  
	        cnt += sprintf(buf + cnt, "wakeup number: %d \n", data.wakeup_number);
                break;
        case 1:
                input_fetch_sysconfig_para(&(gsensor_data.input_type));
                cnt += sprintf(buf + cnt, "gsensor_used : %d \n", gsensor_data.sensor_used); 	
	        cnt += sprintf(buf + cnt, "gsensor_twi_id : %d \n", gsensor_data.twi_id);
                break;
        case 2: 
                input_fetch_sysconfig_para(&(gyr_data.input_type));
                cnt += sprintf(buf + cnt, "gyr_used : %d \n", gyr_data.sensor_used); 	
	        cnt += sprintf(buf + cnt, "gyr_twi_id : %d \n", gyr_data.twi_id);
                break;
        case 4: 
                input_fetch_sysconfig_para(&(ls_data.input_type));
                cnt += sprintf(buf + cnt, "ls_used : %d \n", ls_data.sensor_used); 	
	        cnt += sprintf(buf + cnt, "ls_twi_id : %d \n", ls_data.twi_id);
                break;
        case 5: 
                input_fetch_sysconfig_para(&(compass_data.input_type));
                cnt += sprintf(buf + cnt, "compass_used : %d \n", compass_data.sensor_used); 	
	        cnt += sprintf(buf + cnt, "compass_twi_id : %d \n", compass_data.twi_id);
                break;               
        case 3:
                input_fetch_sysconfig_para(&(ir_data.input_type));
                cnt += sprintf(buf + cnt, "ir_used : %d \n", ir_data.ir_used); 	
	        cnt += sprintf(buf + cnt, "ir_rx number : %d \n", ir_data.ir_gpio.gpio);
                break;
                 
        }  
 
               	
	return cnt;

}

static ssize_t get_irq_para_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
        int clk = 0, clk_pre_scl = 0;	
	struct ctp_config_info data = {
                .input_type = CTP_TYPE,
        };
        
        input_fetch_sysconfig_para(&(data.input_type));
        
	if(0 != __parse_device_str((char *)buf, size, &clk, &clk_pre_scl)) {
		printk(KERN_ERR "%s err, invalid para, __parse_device_str failed\n", __func__);
		goto err;
	}
	if((clk != 0) && (clk != 1) && (clk_pre_scl < 0) && (clk_pre_scl > 7)) {
		printk(KERN_ERR "%s err, invalid para, the addr is not reg\n", __func__);
		goto err;
	}
	
	ctp_set_int_port_rate(data.int_number, clk);
	ctp_set_int_port_deb(data.int_number, clk_pre_scl);


        printk("*****%s:config_para:%d\n", __func__, config_flag);
	return size;
err:
	return -EINVAL;
       
}

static ssize_t get_irq_para_show(struct class *class, struct class_attribute *attr, char *buf)
{        
        ssize_t cnt = 0;
        int clk, clk_pre_scl;
        struct ctp_config_info data = {
                .input_type = CTP_TYPE,
                };
        
        input_fetch_sysconfig_para(&(data.input_type));
        ctp_get_int_port_deb(data.int_number, &clk_pre_scl);
        ctp_get_int_port_rate(data.int_number, &clk);
        
        cnt += sprintf(buf + cnt, "int number : %d \n", data.int_number);
        cnt += sprintf(buf + cnt, "clk : %d (0 is 32Khz, 1 is 24Mhz) \n", clk);
        cnt += sprintf(buf + cnt, "clk_pre_scl : %d (Frequency division coefficient)\n", clk_pre_scl);
               	
	return cnt;

}
static struct class_attribute i2c_class_attrs[] = {
	__ATTR(device, 	0664, i2c_device_show, i2c_device_store),
	__ATTR(write,	0664, i2c_write_show, i2c_write_store),
	__ATTR(read,	0664, i2c_read_show, i2c_read_store),
	__ATTR(para,	0664, get_sysconfig_para_show, get_sysconfig_para_store),
	__ATTR(config,	0664, get_config_para_show, get_config_para_store),
	__ATTR(irq,	0664, get_irq_para_show, get_irq_para_store),
	__ATTR_NULL,
};

static struct class device_of_i2c_class = {
	.name		= "device_of_i2c",
	.owner		= THIS_MODULE,
	.class_attrs	= i2c_class_attrs,
};

static int __init sw_device_i2c_init(void)
{
	int	status;

	status = class_register(&device_of_i2c_class);
	if(status < 0)
		printk(KERN_ERR "%s err, status %d\n", __func__, status);
	else
		printk(KERN_DEBUG "%s success\n", __func__);

	return status;
}

MODULE_AUTHOR("Ida yin");
MODULE_DESCRIPTION("device debug of i2c device driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(sw_device_i2c_init);
