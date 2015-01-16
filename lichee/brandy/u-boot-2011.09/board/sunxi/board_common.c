/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <spare_head.h>
#include <axp_power.h>
#include <android_misc.h>
#include <sunxi_mbr.h>
#include <boot_type.h>
#include <sys_partition.h>
#include <sys_config.h>
#include <fastboot.h>
#include <pmu.h>
#include <asm/arch/timer.h>
#include <asm/arch/key.h>
#include <asm/arch/dma.h>
#include <sunxi_board.h>
#if defined(CONFIG_SUNXI_I2C)
	#include <i2c.h>
#elif defined(CONFIG_SUNXI_P2WI)
	#include <p2wi.h>
#elif defined(CONFIG_SUNXI_RSB)
	#include <rsb.h>
#else
#endif

#if defined(CONFIG_SUNXI_RTC)
	#include <rtc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

extern int update_user_data(void);

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void sunxi_flush_allcaches(void)
{
	icache_disable();

	flush_dcache_all();
	dcache_disable();
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_board_close_source(void)
{
	axp_set_vbus_limit_dc();

	timer_exit();

	sunxi_key_exit();
#ifdef CONFIG_SUN6I
	p2wi_exit();
#endif
	sunxi_dma_exit();
	sunxi_flash_exit(1);	//强制关闭FLASH
	disable_interrupts();
	interrupt_exit();

#if defined(CONFIG_ARCH_SUN9IW1P1)
	*( volatile unsigned int *)(0x008000e0) = 0x16aa0000;
#endif

	return ;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_board_restart(void)
{
	printf("set next system normal\n");
	axp_set_next_poweron_status(PMU_PRE_SYS_MODE);
	board_display_set_exit_mode(0);
	drv_disp_exit();
	sunxi_board_close_source();
	reset_cpu(0);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_board_shutdown(void)
{
#if defined(CONFIG_SUNXI_RTC)
	printf("rtc disable\n");
    rtc_disable();
#endif
	printf("set next system normal\n");
	axp_set_next_poweron_status(0x0);

	board_display_set_exit_mode(0);

	drv_disp_exit();

	sunxi_flash_exit(1);	//强制关闭FLASH
	disable_interrupts();
	interrupt_exit();

	tick_printf("power off\n");
	axp_set_hardware_poweroff_vol();
	axp_set_power_off();

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_board_run_fel(void)
{
#if defined(CONFIG_SUN6I) || defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN7IW1P1)
	*((volatile unsigned int *)(SUNXI_RUN_EFEX_ADDR)) = SUNXI_RUN_EFEX_FLAG;
#endif
	printf("set next system status\n");

	axp_set_next_poweron_status(PMU_PRE_SYS_MODE);

	board_display_set_exit_mode(0);

	drv_disp_exit();
	printf("sunxi_board_close_source\n");
	sunxi_board_close_source();

	sunxi_flush_allcaches();

	printf("reset cpu\n");

	reset_cpu(0);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_board_run_fel_eraly(void)
{
#if defined(CONFIG_SUN6I) || defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN7IW1P1)
	*((volatile unsigned int *)(SUNXI_RUN_EFEX_ADDR)) = SUNXI_RUN_EFEX_FLAG;
#endif
	printf("set next system status\n");

    axp_set_next_poweron_status(PMU_PRE_SYS_MODE);

	timer_exit();
	sunxi_key_exit();
#ifdef CONFIG_SUN6I
	p2wi_exit();
#endif
	sunxi_dma_exit();

	printf("reset cpu\n");

	reset_cpu(0);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_update_subsequent_processing(int next_work)
{
	printf("next work %d\n", next_work);
	switch(next_work)
	{
		case SUNXI_UPDATE_NEXT_ACTION_REBOOT:	//重启
			printf("SUNXI_UPDATE_NEXT_ACTION_REBOOT\n");
			//do_reset(NULL, 0, 0, NULL);
			sunxi_board_restart();

			break;
		case SUNXI_UPDATE_NEXT_ACTION_SHUTDOWN:	//关机
			printf("SUNXI_UPDATE_NEXT_ACTION_SHUTDOWN\n");
			//do_shutdown(NULL, 0, 0, NULL);
			sunxi_board_shutdown();

			break;
		case SUNXI_UPDATE_NEXT_ACTION_REUPDATE:
			printf("SUNXI_UPDATE_NEXT_ACTION_REUPDATE\n");
			sunxi_board_run_fel();			//进行量产

			break;
		case SUNXI_UPDATE_NEXT_ACTION_BOOT:
		case SUNXI_UPDATE_NEXT_ACTION_NORMAL:
		default:
			printf("SUNXI_UPDATE_NEXT_ACTION_NULL\n");
			break;
	}

	return ;
}

static void set_boot_type_cmd(char *boot_commond);

void fastboot_partition_init(void)
{
	fastboot_ptentry fb_part;
	int index, part_total;
	char partition_sets[256];
	char part_name[16];
	char *partition_index = partition_sets;
	int offset = 0;
	int storage_type = uboot_spare_head.boot_data.storage_type;

	printf("--------fastboot partitions--------\n");
	part_total = sunxi_partition_get_total_num();
	if((part_total <= 0) || (part_total > SUNXI_MBR_MAX_PART_COUNT))
	{
		printf("mbr not exist\n");

		return ;
	}
	printf("-total partitions:%d-\n", part_total);
	printf("%-12s  %-12s  %-12s\n", "-name-", "-start-", "-size-");

	memset(partition_sets, 0, 256);
	partition_sets[255] = '\0';

	for(index = 0; index < part_total && index < SUNXI_MBR_MAX_PART_COUNT; index++)
	{
		sunxi_partition_get_name(index, &fb_part.name[0]);
		fb_part.start = sunxi_partition_get_offset(index) * 512;
		fb_part.length = sunxi_partition_get_size(index) * 512;
		fb_part.flags = 0;
		printf("%-12s: %-12x  %-12x\n", fb_part.name, fb_part.start, fb_part.length);
		fastboot_flash_add_ptn(&fb_part);

		memset(part_name, 0, 16);
		if(!storage_type)
		{
			sprintf(part_name, "nand%c", 'a' + index);
		}
		else
		{
			if(index == 0)
			{
				strcpy(part_name, "mmcblk0p2");
			}
			else if( (index+1)==part_total)
			{
				strcpy(part_name, "mmcblk0p1");
			}
			else
			{
				sprintf(part_name, "mmcblk0p%d", index + 4);
			}
		}
		sprintf(partition_index, "%s@%s:", fb_part.name, part_name);
		offset += strlen(fb_part.name) + strlen(part_name) + 2;
		partition_index = partition_sets + offset;
	}

	partition_sets[offset-1] = '\0';
	printf("-----------------------------------\n");

	setenv("partitions", partition_sets);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :  把目标字符串dest_buf(格式：空格隔开，比如"AAA BBB C DD EEE"，但是不包括tab
*                     中的goal子字符串，替换成replace
*                     如
*					  sunxi_str_replace("abc def gh", "def", "replace")
*					  执行的结果是  "abc def gh"变成了 "abc replace gh"
*					  如果找不到不够，则不替换
*					  必须保证空间足够
************************************************************************************************************
*/
static int sunxi_str_replace(char *dest_buf, char *goal, char *replace)
{
	char tmp[128];
	char tmp_str[16];
	int  goal_len, rep_len, dest_len;
	int  i, j, k;

	if( (goal == NULL) || (dest_buf == NULL))
	{
		return -1;
	}

	memset(tmp, 0, 128);
	strcpy(tmp, dest_buf);

	goal_len = strlen(goal);
	dest_len = strlen(dest_buf);

	if(replace != NULL)
	{
		rep_len = strlen(replace);
	}
	else
	{
		rep_len = 0;
	}
	j = 0;
	for(i=0;tmp[i];)
	{
		//找出空格字符
		k = 0;
		while(((tmp[i] != ' ') && (tmp[i] != 0) )|| (tmp[i+1] == ' '))
		{
			tmp_str[k++] = tmp[i];
			i ++;
			if(i >= dest_len)
				break;
		}
		i ++;
		//开始找出一个完整的字符串
		tmp_str[k] = 0;
		if(!strcmp(tmp_str, goal))
		{
			if(rep_len)
			{
				strcpy(dest_buf + j, replace);
				if(tmp[j + goal_len])
				{
					memcpy(dest_buf + j + rep_len, tmp + j + goal_len, dest_len - j - goal_len);
					dest_buf[dest_len - goal_len] = 0;
				}
			}
			else
			{
				if(tmp[j + goal_len])
				{
					memcpy(dest_buf + j, tmp + j + goal_len, dest_len - j - goal_len);
					dest_buf[dest_len - goal_len] = 0;
				}
			}

			return 0;
		}
		j = i;
	}

	return 0;

}
#define    ANDROID_NULL_MODE            0
#define    ANDROID_FASTBOOT_MODE		1
#define    ANDROID_RECOVERY_MODE		2
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int detect_other_boot_mode(void)
{
    int ret1, ret2;
	int key_high, key_low;
	int keyvalue;

	keyvalue = gd->key_pressd_value;
	printf("key %d\n", keyvalue);

    ret1 = script_parser_fetch("recovery_key", "key_max", &key_high, 1);
	ret2 = script_parser_fetch("recovery_key", "key_min", &key_low,  1);
	if((ret1) || (ret2))
	{
		printf("cant find rcvy value\n");
	}
	else
	{
		printf("recovery key high %d, low %d\n", key_high, key_low);
		if((keyvalue >= key_low) && (keyvalue <= key_high))
		{
			printf("key found, android recovery\n");

			return ANDROID_RECOVERY_MODE;
		}
	}
    ret1 = script_parser_fetch("fastboot_key", "key_max", &key_high, 1);
	ret2 = script_parser_fetch("fastboot_key", "key_min", &key_low, 1);
	if((ret1) || (ret2))
	{
		printf("cant find fstbt value\n");
	}
	else
	{
		printf("fastboot key high %d, low %d\n", key_high, key_low);
		if((keyvalue >= key_low) && (keyvalue <= key_high))
		{
			printf("key found, android fastboot\n");
			return ANDROID_FASTBOOT_MODE;
		}
	}

	return ANDROID_NULL_MODE;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int check_android_misc(void)
{
	int   mode;
	u32   misc_offset = 0;
	char  misc_args[2048];
	char  misc_fill[2048];
	char  boot_commond[128];
	static struct bootloader_message *misc_message;

	if(uboot_spare_head.boot_data.work_mode != WORK_MODE_BOOT)
	{
		return 0;
	}
	if(gd->force_shell)
	{
		char delaytime[8];

		sprintf(delaytime, "%d", 3);
		setenv("bootdelay", delaytime);
	}

    memset(boot_commond, 0x0, 128);
	set_boot_type_cmd(boot_commond);
	printf("base bootcmd=%s\n", boot_commond);
	//判断存储介质
	if((uboot_spare_head.boot_data.storage_type == 1) || (uboot_spare_head.boot_data.storage_type == 2))
	{
		sunxi_str_replace(boot_commond, "setargs_nand", "setargs_mmc");
		printf("bootcmd set setargs_mmc\n");
	}
	else
	{
		printf("bootcmd set setargs_nand\n");
	}
	//判断是否存在按键进入其它模式
	memset(misc_args, 0x0, 2048);
	mode = detect_other_boot_mode();
	misc_message = (struct bootloader_message *)misc_args;
	if(mode == ANDROID_NULL_MODE)
	{
		misc_offset = sunxi_partition_get_offset_byname("misc");
		if(!misc_offset)
		{
			puts("no misc partition is found\n");
			printf("to be run cmd=%s\n", boot_commond);
			setenv("bootcmd", boot_commond);

			return 0;
		}
		memset(misc_fill, 0xff, 2048);
#ifdef DEBUG
		tick_printf("misc_offset  : %d\n", (int )misc_offset);
#endif
		sunxi_flash_read(misc_offset, 2048/512, misc_args);
	}
	else if(mode == ANDROID_RECOVERY_MODE)
	{
		strcpy(misc_message->command, "boot-recovery");
	}
	else if(mode == ANDROID_FASTBOOT_MODE)
	{
		strcpy(misc_message->command, "bootloader");
	}
#ifdef DEBUG
	{
		uint *dump_value;

		dump_value = *(uint *)misc_message->command;
		if(dump_value != 0xffffffff)
			printf("misc.command  : %s\n", misc_message->command);
		else
			printf("misc.command  : NULL\n");

		dump_value = *(uint *)misc_message->status;
		if(dump_value != 0xffffffff)
			printf("misc.status  : %s\n", misc_message->status);
		else
			printf("misc.status  : NULL\n");

		dump_value = *(uint *)misc_message->recovery;
		if(dump_value != 0xffffffff)
			printf("misc.recovery  : %s\n", misc_message->recovery);
		else
			printf("misc.recovery  : NULL\n");
	}
#endif
	//判断命令
	if(!strcmp(misc_message->command, "efex"))
	{
		/* there is a recovery command */
		puts("find efex cmd\n");
		sunxi_flash_write(misc_offset, 2048/512, misc_fill);
		sunxi_board_run_fel();

		return 0;
	}

	if(!strcmp(misc_message->command, "boot-recovery"))
	{
		puts("Recovery detected, will boot recovery\n");
		sunxi_str_replace(boot_commond, "boot_normal", "boot_recovery");
		/* android recovery will clean the misc */
	}
	else if(!strcmp(misc_message->command, "bootloader"))
	{
		puts("Fastboot detected, will boot fastboot\n");
		sunxi_str_replace(boot_commond, "boot_normal", "boot_fastboot");
		sunxi_flash_write(misc_offset, 2048/512, misc_fill);
	}

	setenv("bootcmd", boot_commond);

	printf("to be run cmd=%s\n", boot_commond);

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void set_boot_type_cmd(char *boot_commond)
{
	if(uboot_spare_head.boot_data.storage_type)
	{
		strcpy(boot_commond, "run setargs_mmc boot_normal");
	}
	else
	{
		strcpy(boot_commond, "run setargs_nand boot_normal");
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int board_late_init(void)
{
	fastboot_partition_init();
	check_android_misc();

#ifdef  CONFIG_ARCH_HOMELET
	update_user_data();
#endif
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int check_uart_input(void)
{
	int c = 0;
	if(uboot_spare_head.boot_data.work_mode != WORK_MODE_BOOT)
	{
	    return 0;
	}
	if(tstc())
	{
		c = getc();
		printf("0x%x\n", c);
	}
	else
	{
		puts("no key input\n");
	}

	if(c == '2')
	{
		return -1;
	}
	else if(c == '3')
	{
		sunxi_key_init();
		do_key_test(NULL, 0, 1, NULL);
	}
	else if(c == 's')		//shell mode
	{
		gd->force_shell = 1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
#define   KEY_DELAY_MAX          (8)
#define   KEY_DELAY_EACH_TIME    (40)
#define   KEY_MAX_COUNT_GO_ON    ((KEY_DELAY_MAX * 1000)/(KEY_DELAY_EACH_TIME))


int check_update_key(void)
{
	int ret;
	int fel_key_max;
	int fel_key_mode = 0;

    gd->key_pressd_value = 0;
	if(uboot_spare_head.boot_data.work_mode != WORK_MODE_BOOT)
	{
	    return 0;
	}

	sunxi_key_init();
	ret = script_parser_fetch("fel_key", "fel_key_max", &fel_key_max, 1);
    if(ret)
    {
    	printf("fel key old mode\n");
		fel_key_mode = 1;
	}
	else
	{
		printf("fel key new mode\n");
	}

	printf("run key detect\n");

	sunxi_key_read();
	__msdelay(10);

	if(!fel_key_mode)
	{
		int key_value;
		int fel_key_max, fel_key_min;


	    key_value = sunxi_key_read();  		//读取按键信息
	    if(key_value < 0)             				//没有按键按下
	    {
	        printf("no key found\n");
	        return -1;
	    }
	   gd->key_pressd_value = key_value;
		ret = script_parser_fetch("fel_key", "fel_key_max", &fel_key_max, 1);
	    if(ret)
	    {
	    	printf("fel key max not found\n");

	    	return 0;
	    }

		ret = script_parser_fetch("fel_key", "fel_key_min", &fel_key_min, 1);
	    if(ret)
	    {
	    	printf("fel key min not found\n");

	    	return 0;
	    }

		if((key_value <= fel_key_max) && (key_value >= fel_key_min))
		{
			printf("fel key detected\n");

			return 1;
		}

		printf("fel key value %d is not in the range from %d to %d\n", key_value, fel_key_min, fel_key_max);

		return 0;
	}
	else
	{
	    int count, time_tick;
	    int value_old, value_new, value_cnt;
	    int new_key, new_key_flag;

	    time_tick = 0;
	    count = 0;
	    value_cnt = 0;
	    new_key = 0;
	    new_key_flag = 0;
	    ret = sunxi_key_read();  				//读取按键信息
	    if(ret < 0)             				//没有按键按下
	    {
	        printf("no key found\n");
	        return 0;
	    }
	    else
	    {
	    	value_old = ret;
	    }
			gd->key_pressd_value = ret;
	    while(1)
	    {
	        time_tick ++;
	        ret = axp_probe_key();  			//获取power按键信息
	        if(ret > 0)              	  		//检测到POWER按键按下
	        {
	            count ++;
	        }

	        __msdelay(KEY_DELAY_EACH_TIME);
	        ret = sunxi_key_read();  			//读取按键信息
	        if(ret < 0)             			//没有按键按下
	        {
	            printf("key not pressed anymore\n");
	            if(count == 1)
	            {
	            	if(new_key >= 2)
	            	{
	            		printf("1\n");
	            		printf("force to debug mode\n");

	            		return -2;
	            	}
	            }

				return 0;
	        }
	        else
	        {
	        	value_new = ret;
	        	if(value_old == value_new)
	        	{
	        		value_cnt ++;
	        		if(new_key_flag == 1)
	        		{
	        			new_key ++;
	        			new_key_flag ++;
	        		}
	        		else if(!new_key_flag)
	        		{
	        			new_key_flag ++;
	        		}
	        	}
	        	else
	        	{
	        		new_key_flag = 0;
	        		value_old = value_new;
	        	}
	        }

	        if(count == 3)
	        {
	        	printf("you can unclench the key to update now\n");
	            return -1;
	        }

	        if((!count) && (time_tick >= KEY_MAX_COUNT_GO_ON))
	        {
	            printf("timeout, but no power key found\n");

	            return 0;
	        }
	    }
	}
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :    作用：初始化用户配置的gpio   主键:[boot_init_gpio]
*
*
************************************************************************************************************
*/
int gpio_control(void)
{
	int ret;
	int use_data;

	ret = script_parser_fetch("boot_init_gpio", "use", &use_data, sizeof(int) / 4);
    if (!ret) {
		if (use_data) {
			puts("user_gpio config\n");
			gpio_request_ex("boot_init_gpio", NULL);
			puts("user_gpio ok\n");
			return 0;
		}
	}
	printf("donn't initialize ther user_gpio (main_key:boot_init_gpio)\n");
	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :	一键恢复的按键检测
*
*    parmeters     :
*
*    return        :
*
*    note          :	yanjianbo@allwinnertech.com
*
*
************************************************************************************************************
*/
int check_boot_recovery_key(void)
{
	user_gpio_set_t     gpio_recovery;
	__u32			    gpio_hd;
	int					gpio_value;
	int ret;
	
	ret = script_parser_fetch("system", "recovery_key", (int *)&gpio_recovery, sizeof(user_gpio_set_t)/4);
	if (!ret)
	{
		gpio_recovery.mul_sel = 0;		//强制设置成输入
		gpio_hd = gpio_request(&gpio_recovery, 1);
		
		if (gpio_hd)
		{
			int k;
			gpio_value = 0;
			for(k=0;k<4;k++)
			{
				gpio_value += gpio_read_one_pin_value(gpio_hd, 0);
				__msdelay(5);
			}
			if (!gpio_value)
			{
				printf("set to recovery\n");
				return 0;
			}
		}
	}
	return 1;
}