/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: usb_manager.c
*
* Author 		: javen
*
* Description 	: USB 管理程序
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-4-14            1.0          create this file
*
*************************************************************************************
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/kthread.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/suspend.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/includes.h>

#include  "../include/sw_usb_config.h"
#include  "usb_manager.h"
#include  "usbc_platform.h"
#include  "usb_hw_scan.h"
#include  "usb_msg_center.h"


static struct usb_cfg g_usb_cfg;

#ifdef CONFIG_USB_SW_SUN7I_USB0_OTG
#define THREAD_SUSPEND_START_MASK 0x1
#define THREAD_SUSPEND_USB_DETECT_MASK 0x2

__u32 thread_suspend_flag = 0;
static __u32 thread_run_flag = 1;
static __u32 thread_stopped_flag = 1;
#endif

/*
*******************************************************************************
*                     usb_hardware_scan_thread
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
#ifdef CONFIG_USB_SW_SUN7I_USB0_OTG

static int usb_hardware_scan_thread(void * pArg)
{
	struct usb_cfg *cfg = pArg;

    /* delay for udc & hcd ready */
	msleep(3000);

	while(thread_run_flag){
		DMSG_DBG_MANAGER("\n\n");
		msleep(1000);  /* 1s */
		if(thread_suspend_flag & THREAD_SUSPEND_START_MASK){
			thread_suspend_flag = THREAD_SUSPEND_START_MASK;
			continue;
		}
		thread_suspend_flag |= THREAD_SUSPEND_USB_DETECT_MASK;
		usb_hw_scan(cfg);
		usb_msg_center(cfg);
		thread_suspend_flag &= ~THREAD_SUSPEND_USB_DETECT_MASK;

		DMSG_DBG_MANAGER("\n\n");		
	}

	thread_stopped_flag = 1;

	return 0;
}

#endif

/*
*******************************************************************************
*                     usb_manager_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static __s32 usb_script_parse(struct usb_cfg *cfg)
{
    u32 i = 0;
    char *set_usbc = NULL;
    script_item_value_type_e type = 0;
    script_item_u item_temp;

    for(i = 0; i < cfg->usbc_num; i++){
        if(i == 0){
            set_usbc = SET_USB0;
        }else if(i == 1){
            set_usbc = SET_USB1;
        }else{
            set_usbc = SET_USB2;
        }

        /* usbc enable */
        type = script_get_item(set_usbc, KEY_USB_ENABLE, &item_temp);
        if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
            cfg->port[i].enable = item_temp.val;
        }else{
            DMSG_PANIC("ERR: get usbc(%d) enable failed\n", i);
            cfg->port[i].enable = 0;
        }

        /* usbc port type */
        type = script_get_item(set_usbc, KEY_USB_PORT_TYPE, &item_temp);
        if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
            cfg->port[i].port_type = item_temp.val;
        }else{
            DMSG_PANIC("ERR: get usbc(%d) port type failed\n", i);
            cfg->port[i].port_type = 0;
        }

        /* usbc usb_restrict_flag  */
        type = script_get_item(set_usbc, KEY_USB_USB_RESTRICT_FLAG, &item_temp);
        if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
            cfg->port[i].usb_restrict_flag = item_temp.val;
        }else{
            DMSG_PANIC("ERR: get usbc(%d) usb_restrict_flag failed\n", i);
            cfg->port[i].usb_restrict_flag = 0;
        }

        /* usbc voltage type */
        type = script_get_item(set_usbc, KEY_USB_USB_RESTRICT_VOLTAGE, &item_temp);
        if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
            cfg->port[i].voltage = item_temp.val;
        }else{
            DMSG_PANIC("ERR: get usbc(%d) voltage  failed\n", i);
            cfg->port[i].voltage = 0;
        }

        /* usbc capacity type */
        type = script_get_item(set_usbc, KEY_USB_USB_RESTRICT_CAPACITY, &item_temp);
        if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
            cfg->port[i].capacity = item_temp.val;
        }else{
            DMSG_PANIC("ERR: get usbc(%d)voltage failed\n", i);
            cfg->port[i].capacity = 0;
        }

        /* usbc detect type */
        type = script_get_item(set_usbc, KEY_USB_DETECT_TYPE, &item_temp);
        if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
            cfg->port[i].detect_type = item_temp.val;
        }else{
            DMSG_PANIC("ERR: get usbc(%d) detect_type  failed\n", i);
            cfg->port[i].detect_type = 0;
        }

        /* usbc id */
        type = script_get_item(set_usbc, KEY_USB_ID_GPIO, &(cfg->port[i].id.gpio_set));
        if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
            cfg->port[i].id.valid = 1;
        }else{
            cfg->port[i].id.valid = 0;
            DMSG_PANIC("ERR: get usbc(%d) id failed\n", i);
        }

        /* usbc det_vbus */
        type = script_get_item(set_usbc, KEY_USB_DETVBUS_GPIO, &(cfg->port[i].det_vbus.gpio_set));
        if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
            cfg->port[i].det_vbus.valid = 1;
            cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_GIPO;
        }else{
            cfg->port[i].det_vbus.valid = 0;
            cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_NULL;
            DMSG_PANIC("ERR: get usbc(%d) det_vbus gpio failed\n", i);
        }

        if(cfg->port[i].det_vbus.valid == 0){
            type = script_get_item(set_usbc, KEY_USB_DETVBUS_GPIO, &item_temp);
            if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
                if(strncmp(item_temp.str, "axp_ctrl", 8) == 0){
                    cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_AXP;
                }else{
                    cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_NULL;
                }
            }else{
                    DMSG_PANIC("ERR: get usbc(%d) det_vbus axp failed\n", i);
                    cfg->port[i].det_vbus_type = USB_DET_VBUS_TYPE_NULL;
            }
        }

        /* usbc drv_vbus */
        type = script_get_item(set_usbc, KEY_USB_DRVVBUS_GPIO, &(cfg->port[i].drv_vbus.gpio_set));
        if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
            cfg->port[i].drv_vbus.valid = 1;
        }else{
            cfg->port[i].drv_vbus.valid = 0;
            DMSG_PANIC("ERR: get usbc(%d) det_vbus failed\n", i);
        }

        /* enable ac */
        type = script_get_item(set_usbc, "usb_ac_enable_gpio", &(cfg->port[i].ac_enable.gpio_set));
        if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
            cfg->port[i].ac_enable.valid = 1;
        }else{
            cfg->port[i].ac_enable.valid = 0;
            DMSG_PANIC("ERR: get usbc(%d) ac_enable failed\n", i);
        }
        
        /* usbc usb_restrict */
        type = script_get_item(set_usbc, KEY_USB_RESTRICT_GPIO, &(cfg->port[i].restrict_gpio_set.gpio_set));
        if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
            cfg->port[i].restrict_gpio_set.valid = 1;
        }else{
            DMSG_PANIC("ERR: get usbc0(usb_restrict) failed\n");
            cfg->port[i].restrict_gpio_set.valid = 0;
        }
    }

    return 0;
}

/*
*******************************************************************************
*                     check_usb_board_info
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static __s32 check_usb_board_info(struct usb_cfg *cfg)
{
    //-------------------------------------
    // USB0
    //-------------------------------------
    if(cfg->port[0].enable){
        /* 检查port的使用类型是否合法 */
        if(cfg->port[0].port_type != USB_PORT_TYPE_DEVICE
           && cfg->port[0].port_type != USB_PORT_TYPE_HOST
           && cfg->port[0].port_type != USB_PORT_TYPE_OTG){
            DMSG_PANIC("ERR: usbc0 port_type(%d) is unkown\n", cfg->port[0].port_type);
            goto err;
        }

        /* 检查USB的插拔检测方式是否合法 */
        if(cfg->port[0].detect_type != USB_DETECT_TYPE_DP_DM
           && cfg->port[0].detect_type != USB_DETECT_TYPE_VBUS_ID){
            DMSG_PANIC("ERR: usbc0 detect_type(%d) is unkown\n", cfg->port[0].detect_type);
            goto err;
        }

        /* 如果用VBUS/ID检测方式，就必须检查id/vbus pin 的有效性 */
        if(cfg->port[0].detect_type == USB_DETECT_TYPE_VBUS_ID){
            if(cfg->port[0].id.valid == 0){
                DMSG_PANIC("ERR: id pin is invaild\n");
                goto err;
            }

            if(cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_GIPO){
                if(cfg->port[0].det_vbus.valid == 0){
                    DMSG_PANIC("ERR: det_vbus pin is invaild\n");
                    goto err;
                }
            }
        }
    }

    //-------------------------------------
    // USB1
    //-------------------------------------

    //-------------------------------------
    // USB2
    //-------------------------------------

    return 0;

err:
    return -1;
}

/*
*******************************************************************************
*                     print_gpio_set
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void print_gpio_set(struct gpio_config *gpio)
{
    DMSG_MANAGER_DEBUG("gpio_name            = %x\n", gpio->gpio);
    DMSG_MANAGER_DEBUG("mul_sel              = %x\n", gpio->mul_sel);
    DMSG_MANAGER_DEBUG("pull                 = %x\n", gpio->pull);
    DMSG_MANAGER_DEBUG("drv_level            = %x\n", gpio->drv_level);
    DMSG_MANAGER_DEBUG("data                 = %x\n", gpio->data);
}

/*
*******************************************************************************
*                     print_usb_cfg
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void print_usb_cfg(struct usb_cfg *cfg)
{
    u32 i = 0;

    DMSG_MANAGER_DEBUG("\n-----------usb config information--------------\n");

    DMSG_MANAGER_DEBUG("controller number    = %x\n", (u32)USBC_MAX_CTL_NUM);

    DMSG_MANAGER_DEBUG("usb_global_enable    = %x\n", cfg->usb_global_enable);
    DMSG_MANAGER_DEBUG("usbc_num             = %x\n", cfg->usbc_num);

    for(i = 0; i < USBC_MAX_CTL_NUM; i++){
        DMSG_MANAGER_DEBUG("\n");
        DMSG_MANAGER_DEBUG("port[%d]:\n", i);
        DMSG_MANAGER_DEBUG("enable               = %x\n", cfg->port[i].enable);
        DMSG_MANAGER_DEBUG("port_no              = %x\n", cfg->port[i].port_no);
        DMSG_MANAGER_DEBUG("port_type            = %x\n", cfg->port[i].port_type);
        DMSG_MANAGER_DEBUG("detect_type          = %x\n", cfg->port[i].detect_type);

        DMSG_MANAGER_DEBUG("id.valid             = %x\n", cfg->port[i].id.valid);
        print_gpio_set(&cfg->port[i].id.gpio_set.gpio);

        if(cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_GIPO){
            DMSG_MANAGER_DEBUG("vbus.valid           = %x\n", cfg->port[i].det_vbus.valid);
            print_gpio_set(&cfg->port[i].det_vbus.gpio_set.gpio);
        }

        DMSG_MANAGER_DEBUG("drv_vbus.valid       = %x\n", cfg->port[i].drv_vbus.valid);
        print_gpio_set(&cfg->port[i].drv_vbus.gpio_set.gpio);

        DMSG_MANAGER_DEBUG("\n");
    }

    DMSG_MANAGER_DEBUG("-------------------------------------------------\n");
}



/*
*******************************************************************************
*                     get_usb_cfg
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static __s32 get_usb_cfg(struct usb_cfg *cfg)
{
	__s32 ret = 0;

	/* parse script */
	ret = usb_script_parse(cfg);
	if(ret != 0){
		DMSG_PANIC("ERR: usb_script_parse failed\n");
		return -1;
	}

	//modify_usb_borad_info(cfg);

	print_usb_cfg(cfg);

    ret = check_usb_board_info(cfg);
	if(ret != 0){
		DMSG_PANIC("ERR: check_usb_board_info failed\n");
		return -1;
	}

	return 0;
}

#ifdef CONFIG_USB_SW_SUN7I_USB0_OTG
struct task_struct *th = NULL;
static int usb_pm_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{
    if (event == PM_SUSPEND_PREPARE) {
        thread_suspend_flag |= THREAD_SUSPEND_START_MASK;
        while(thread_suspend_flag&THREAD_SUSPEND_USB_DETECT_MASK){
            wake_up_process(th);
            msleep(1);
        }
    } else if (event == PM_POST_SUSPEND) {
        thread_suspend_flag = 0;
    }
    return NOTIFY_OK;
}

static struct notifier_block usb_pm_notifier = {
    .notifier_call = usb_pm_notify,
//    .priority = 0xFF,
};
#endif
/*
*******************************************************************************
*                     usb_manager_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int __init usb_manager_init(void)
{
	__s32 ret = 0;
	bsp_usbc_t usbc;
	u32 i = 0;

    printk("line = %d,%s\n", __LINE__, __func__);
    DMSG_MANAGER_DEBUG("[sw usb]: usb_manager_init\n");

#if defined(CONFIG_USB_SW_SUN7I_USB0_DEVICE_ONLY)
	DMSG_INFO_MANAGER("CONFIG_USB_SW_SUN7I_USB0_DEVICE_ONLY\n");
#elif defined(CONFIG_USB_SW_SUN7I_USB0_HOST_ONLY)
	DMSG_INFO_MANAGER("CONFIG_USB_SW_SUN7I_USB0_HOST_ONLY\n");
#elif defined(CONFIG_USB_SW_SUN7I_USB0_OTG)
	DMSG_INFO_MANAGER("CONFIG_USB_SW_SUN7I_USB0_OTG\n");
#else
	DMSG_INFO_MANAGER("CONFIG_USB_SW_SUN7I_USB0_NULL\n");
	return 0;
#endif

	memset(&g_usb_cfg, 0, sizeof(struct usb_cfg));
	g_usb_cfg.usb_global_enable = 1;
    g_usb_cfg.usbc_num = USBC_MAX_CTL_NUM;

	ret = get_usb_cfg(&g_usb_cfg);
	if(ret != 0){
		DMSG_PANIC("ERR: get_usb_cfg failed\n");
		return -1;
	}

	if(g_usb_cfg.port[0].enable == 0){
		DMSG_PANIC("wrn: usb0 is disable\n");
		return 0;
	}

    memset(&usbc, 0, sizeof(bsp_usbc_t));
   	for(i = 0; i < USBC_MAX_CTL_NUM; i++){
		usbc.usbc_info[i].num = i;

		switch(i){
            case 0:
                usbc.usbc_info[i].base = SW_VA_USB0_IO_BASE;
            break;

            case 1:
                usbc.usbc_info[i].base = SW_VA_USB1_IO_BASE;
            break;

			case 2:
                usbc.usbc_info[i].base = SW_VA_USB2_IO_BASE;
            break;

            default:
                DMSG_PANIC("ERR: unkown cnt(%d)\n", i);
                usbc.usbc_info[i].base = 0;
        }
	}

	usbc.sram_base = SW_VA_SRAM_IO_BASE;
	USBC_init(&usbc);

    usbc0_platform_device_init(&g_usb_cfg.port[0]);

#ifdef CONFIG_USB_SW_SUN7I_USB0_OTG
    if(g_usb_cfg.port[0].port_type == USB_PORT_TYPE_OTG
       && g_usb_cfg.port[0].detect_type == USB_DETECT_TYPE_VBUS_ID){
    	usb_hw_scan_init(&g_usb_cfg);

    	thread_run_flag = 1;
    	thread_stopped_flag = 0;
    	th = kthread_create(usb_hardware_scan_thread, &g_usb_cfg, "usb-hardware-scan");
    	if(IS_ERR(th)){
    		DMSG_PANIC("ERR: kthread_create failed\n");
    		return -1;
    	}

    	wake_up_process(th);
	}
    register_pm_notifier(&usb_pm_notifier);
#endif

    DMSG_MANAGER_DEBUG("[sw usb]: usb_manager_init end\n");

    return 0;
}

/*
*******************************************************************************
*                     usb_manager_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void __exit usb_manager_exit(void)
{
	bsp_usbc_t usbc;

    DMSG_MANAGER_DEBUG("[sw usb]: usb_manager_exit\n");

#if defined(CONFIG_USB_SW_SUN7I_USB0_DEVICE_ONLY)
	DMSG_INFO("CONFIG_USB_SW_SUN7I_USB0_DEVICE_ONLY\n");
#elif defined(CONFIG_USB_SW_SUN7I_USB0_HOST_ONLY)
	DMSG_INFO("CONFIG_USB_SW_SUN7I_USB0_HOST_ONLY\n");
#elif defined(CONFIG_USB_SW_SUN7I_USB0_OTG)
	DMSG_INFO("CONFIG_USB_SW_SUN7I_USB0_OTG\n");
#else
	DMSG_INFO("CONFIG_USB_SW_SUN7I_USB0_NULL\n");
	return;
#endif

	if(g_usb_cfg.port[0].enable == 0){
		DMSG_PANIC("wrn: usb0 is disable\n");
		return ;
	}

    memset(&usbc, 0, sizeof(bsp_usbc_t));
	USBC_exit(&usbc);

#ifdef CONFIG_USB_SW_SUN7I_USB0_OTG
    unregister_pm_notifier(&usb_pm_notifier);
    if(g_usb_cfg.port[0].port_type == USB_PORT_TYPE_OTG
       && g_usb_cfg.port[0].detect_type == USB_DETECT_TYPE_VBUS_ID){
    	thread_run_flag = 0;
    	while(!thread_stopped_flag){
    		DMSG_INFO("waitting for usb_hardware_scan_thread stop\n");
    		msleep(10);
    	}

    	usb_hw_scan_exit(&g_usb_cfg);
    }
#endif

    usbc0_platform_device_exit(&g_usb_cfg.port[0]);

    return;
}

//subsys_initcall(usb_manager_init);
//module_init(usb_manager_init);
fs_initcall(usb_manager_init);
module_exit(usb_manager_exit);

