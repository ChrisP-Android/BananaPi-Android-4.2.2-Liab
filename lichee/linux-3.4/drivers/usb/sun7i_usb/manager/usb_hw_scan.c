/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: usb_hw_scan.c
*
* Author 		: javen
*
* Description 	: USB 检测
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

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/includes.h>

#include  "../include/sw_usb_config.h"
#include  "usb_manager.h"
#include  "usb_hw_scan.h"
#include  "usb_msg_center.h"

static struct usb_scan_info g_usb_scan_info;
extern int axp_usb_det(void);
void (*__usb_hw_scan) (struct usb_scan_info *);

static void ac_enable(struct usb_scan_info *info)
{
    if(info->ac_enable)
        return;
    printk("ac_enable\n");
    info->ac_enable = 1;
    if(info->cfg->port[0].ac_enable.valid){
        printk("gpio ac_enable pull up\n");
		__gpio_set_value(info->cfg->port[0].ac_enable.gpio_set.gpio.gpio, 1);
	}
}

static void ac_disable(struct usb_scan_info *info)
{
    if(!info->ac_enable)
        return;
    printk("ac_disable\n");
    info->ac_enable = 0;
    if(info->cfg->port[0].ac_enable.valid){
        printk("gpio ac_enable pull down\n");
		__gpio_set_value(info->cfg->port[0].ac_enable.gpio_set.gpio.gpio, 0);
    }
}

/*
*******************************************************************************
*                     __get_pin_data
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
static __u32 get_pin_data(struct usb_gpio *usb_gpio)
{
    return __gpio_get_value(usb_gpio->gpio_set.gpio.gpio);
}

/*
*********************************************************************
*                     PIODataIn_debounce
*
* Description:
*   过滤PIO的毛刺
*   取10次，如果10次相同，则认为无抖动，取任意一次的值返回
*   如果10次有一次不相同，则本次读取无效
*
* Arguments:
*    phdle  :  input.
*    value  :  output.  读回来的PIO的值
*
* Returns:
*    返回是否有变化
*
* note:
*    无
*
*********************************************************************
*/
static __u32 PIODataIn_debounce(struct usb_gpio *usb_gpio, __u32 *value)
{
    __u32 retry  = 0;
    __u32 time   = 10;
    __u32 temp1  = 0;
    __u32 cnt    = 0;
    __u32 change = 0;   /* 是否有抖动? */

    /* 取 10 次PIO的状态，如果10次的值都一样，说明本次读操作有效，
       否则，认为本次读操作失败。
    */
    if(usb_gpio->valid){
        retry = time;
        while(retry--){
            temp1 = get_pin_data(usb_gpio);
            if(temp1){
                cnt++;
            }
        }

        /* 10 次都为0，或者都为1 */
        if((cnt == time)||(cnt == 0)){
            change = 0;
        }
        else{
            change = 1;
        }
    }else{
        change = 1;
    }

    if(!change){
        *value = temp1;
    }

    DMSG_DBG_MANAGER("usb_gpio->valid = %x, cnt = %x, change= %d, temp1 = %x\n",
                    usb_gpio->valid, cnt, change, temp1);

    return change;
}

/*
*******************************************************************************
*                     get_id_state
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
static u32 get_id_state(struct usb_scan_info *info)
{
    enum usb_id_state id_state = USB_DEVICE_MODE;
    __u32 pin_data = 0;

    if(info->cfg->port[0].id.valid){
        if(!PIODataIn_debounce(&info->cfg->port[0].id, &pin_data)){
            if(pin_data){
                id_state = USB_DEVICE_MODE;
            }else{
                id_state = USB_HOST_MODE;
            }

            info->id_old_state = id_state;
        }else{
            id_state = info->id_old_state;
        }
    }

    return id_state;
}

/*
*******************************************************************************
*                     get_detect_vbus_state
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
static u32 get_detect_vbus_state(struct usb_scan_info *info)
{
    enum usb_det_vbus_state det_vbus_state = USB_DET_VBUS_INVALID;
    __u32 pin_data = 0;

    if(info->cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_GIPO){
        if(info->cfg->port[0].det_vbus.valid){
            if(!PIODataIn_debounce(&info->cfg->port[0].det_vbus, &pin_data)){
                if(pin_data){
                    det_vbus_state = USB_DET_VBUS_VALID;
                }else{
                    det_vbus_state = USB_DET_VBUS_INVALID;
                }

                info->det_vbus_old_state = det_vbus_state;
            }else{
                det_vbus_state = info->det_vbus_old_state;
            }
        }
    }else if(info->cfg->port[0].det_vbus_type == USB_DET_VBUS_TYPE_AXP){
        if(axp_usb_det()){        
            det_vbus_state = USB_DET_VBUS_VALID;
        }else{
            det_vbus_state = USB_DET_VBUS_INVALID;
        }
    }else{
        det_vbus_state = info->det_vbus_old_state;
    }
    return det_vbus_state;
}

static u32 get_dp_dm_status_normal(struct usb_scan_info *info)
{
    __u32 reg_val = 0;
    __u32 dp = 0;
    __u32 dm = 0;

    /* USBC_EnableDpDmPullUp */
    reg_val = USBC_Readl(USBC_REG_ISCR(SW_VA_USB0_IO_BASE));
    reg_val |= (1 << USBC_BP_ISCR_DPDM_PULLUP_EN);
    USBC_Writel(reg_val, USBC_REG_ISCR(SW_VA_USB0_IO_BASE));

    /* USBC_EnableIdPullUp */
    reg_val = USBC_Readl(USBC_REG_ISCR(SW_VA_USB0_IO_BASE));
    reg_val |= (1 << USBC_BP_ISCR_ID_PULLUP_EN);
    USBC_Writel(reg_val, USBC_REG_ISCR(SW_VA_USB0_IO_BASE));

    msleep(10);

    reg_val = USBC_Readl(USBC_REG_ISCR(SW_VA_USB0_IO_BASE));
    dp = (reg_val >> USBC_BP_ISCR_EXT_DP_STATUS) & 0x01;
    dm = (reg_val >> USBC_BP_ISCR_EXT_DM_STATUS) & 0x01;

    return ((dp << 1) | dm);
}

static u32 get_dp_dm_status(struct usb_scan_info *info)
{
    u32 ret  = 0;
    u32 ret0 = 0;
    u32 ret1 = 0;
    u32 ret2 = 0;

    ret0 = get_dp_dm_status_normal(info);
    ret1 = get_dp_dm_status_normal(info);
    ret2 = get_dp_dm_status_normal(info);

    //连续读3次是为了避开电平的瞬间变化
    if((ret0 == ret1) && (ret0 == ret2)){
        ret = ret0;
    }else if(ret2 == 0x11){
        if(get_usb_role() == USB_ROLE_DEVICE){
            ret = 0x11;
            DMSG_PANIC("ERR: dp/dm status is continuous(0x11)\n");
        }
    }else{
        ret = ret2;
    }

    return ret;
}



/*
*******************************************************************************
*                     do_vbus0_id0
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
static void do_vbus0_id0(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	info->device_insmod_delay = 0;

	switch(role){
		case USB_ROLE_NULL:
			/* delay for vbus is stably */
			if(info->host_insmod_delay < USB_SCAN_INSMOD_HOST_DRIVER_DELAY){
				info->host_insmod_delay++;
				break;
			}
			info->host_insmod_delay = 0;

			/* insmod usb host */
			hw_insmod_usb_host();
		break;

		case USB_ROLE_HOST:
			/* nothing to do */
		break;

		case USB_ROLE_DEVICE:
			/* rmmod usb device */
			hw_rmmod_usb_device();
		break;

		default:
			DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}

	return;
}

/*
*******************************************************************************
*                     do_vbus0_id1
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
static void do_vbus0_id1(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	info->device_insmod_delay = 0;
	info->host_insmod_delay   = 0;

	switch(role){
		case USB_ROLE_NULL:
			/* nothing to do */
		break;

		case USB_ROLE_HOST:
			hw_rmmod_usb_host();
		break;

		case USB_ROLE_DEVICE:
			hw_rmmod_usb_device();
		break;

		default:
			DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}

	return;
}

/*
*******************************************************************************
*                     do_vbus1_id0
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
static void do_vbus1_id0(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	info->device_insmod_delay = 0;

	switch(role){
		case USB_ROLE_NULL:
			/* delay for vbus is stably */
			if(info->host_insmod_delay < USB_SCAN_INSMOD_HOST_DRIVER_DELAY){
				info->host_insmod_delay++;
				break;
			}
			info->host_insmod_delay = 0;

			hw_insmod_usb_host();
		break;

		case USB_ROLE_HOST:
			/* nothing to do */
		break;

		case USB_ROLE_DEVICE:
			hw_rmmod_usb_device();
		break;

		default:
			DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}

	return;
}

/*
*******************************************************************************
*                     do_vbus1_id1
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
static void do_vbus1_id1(struct usb_scan_info *info)
{
	enum usb_role role = USB_ROLE_NULL;

	role = get_usb_role();
	info->host_insmod_delay = 0;

	switch(role){
		case USB_ROLE_NULL:
			if(get_dp_dm_status(info) == 0x00){
    			/* delay for vbus is stably */
    			if(info->device_insmod_delay < USB_SCAN_INSMOD_DEVICE_DRIVER_DELAY){
    				info->device_insmod_delay++;
    				break;
    			}

    			info->device_insmod_delay = 0;
			    hw_insmod_usb_device();
			}
			else{
			    ac_enable(info);
			    return;
			}
		break;

		case USB_ROLE_HOST:
			hw_rmmod_usb_host();
		break;

		case USB_ROLE_DEVICE:
			/* nothing to do */
		break;

		default:
			DMSG_PANIC("ERR: unkown usb role(%d)\n", role);
	}
    ac_disable(info);
	return;
}

/*
*******************************************************************************
*                     get_vbus_id_state
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
static __u32 get_vbus_id_state(struct usb_scan_info *info)
{
    u32 state = 0;

	if(get_id_state(info) == USB_DEVICE_MODE){
		x_set_bit(state, 0);
	}

	if(get_detect_vbus_state(info) == USB_DET_VBUS_VALID){
		x_set_bit(state, 1);
	}

	return state;
}

/*
*******************************************************************************
*                     vbus_id_hw_scan
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
static void vbus_id_hw_scan(struct usb_scan_info *info)
{
	__u32 vbus_id_state = 0;

	vbus_id_state = get_vbus_id_state(info);

	DMSG_DBG_MANAGER("vbus_id=%d, role=%d\n", vbus_id_state, get_usb_role());

	switch(vbus_id_state){
		case  0x00:
			do_vbus0_id0(info);
		break;

		case  0x01:
			do_vbus0_id1(info);
		break;

		case  0x02:
			do_vbus1_id0(info);
		break;

		case  0x03:
			do_vbus1_id1(info);
		return;

		default:
			DMSG_PANIC("ERR: vbus_id_hw_scan: unkown vbus_id_state(0x%x)\n", vbus_id_state);
	}
    ac_disable(info);
	return ;
}

/*
*******************************************************************************
*                     usb_hw_scan
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
static void null_hw_scan(struct usb_scan_info *info)
{
	DMSG_DBG_MANAGER("null_hw_scan\n");

	return;
}

/*
*******************************************************************************
*                     usb_hw_scan
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
void usb_hw_scan(struct usb_cfg *cfg)
{
    __usb_hw_scan(&g_usb_scan_info);
}

/*
*******************************************************************************
*                     usb_hw_scan_init
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
__s32 usb_hw_scan_init(struct usb_cfg *cfg)
{
	struct usb_scan_info *scan_info = &g_usb_scan_info;
	struct usb_port_info *port_info = NULL;
	__s32 ret = 0;

	memset(scan_info, 0, sizeof(struct usb_scan_info));
	scan_info->cfg 					= cfg;
	scan_info->id_old_state 		= USB_DEVICE_MODE;
	scan_info->det_vbus_old_state 	= USB_DET_VBUS_INVALID;

	port_info =&(cfg->port[0]);
	switch(port_info->port_type){
		case USB_PORT_TYPE_DEVICE:
			__usb_hw_scan = null_hw_scan;
		break;

		case USB_PORT_TYPE_HOST:
			__usb_hw_scan = null_hw_scan;
		break;

		case USB_PORT_TYPE_OTG:
		{
			switch(port_info->detect_type){
				case USB_DETECT_TYPE_DP_DM:
					__usb_hw_scan = null_hw_scan;
				break;

				case USB_DETECT_TYPE_VBUS_ID:
				{
					__u32 need_pull_pio = 1;

					if(port_info->det_vbus_type == USB_DET_VBUS_TYPE_GIPO){
						if((port_info->id.valid == 0) || (port_info->det_vbus.valid == 0)){
							DMSG_PANIC("ERR: usb detect tpye is vbus/id, but id(%d)/vbus(%d) is invalid\n",
								       port_info->id.valid, port_info->det_vbus.valid);
							ret = -1;
							goto failed;
						}

	                    /* 如果id和vbus的pin相同, 就不需要拉pio了 */
						if(port_info->id.gpio_set.gpio.gpio == port_info->det_vbus.gpio_set.gpio.gpio){
							need_pull_pio = 0;
						}
					}

						/* request id gpio */
					if(port_info->id.valid){
						ret = gpio_request(port_info->id.gpio_set.gpio.gpio, "otg_id");
						if(ret != 0){
							DMSG_PANIC("ERR: id gpio_request failed\n");
							ret = -1;
							port_info->id.valid = 0;
							goto failed;
						}

						/* set config, input */
						sw_gpio_setcfg(port_info->id.gpio_set.gpio.gpio, 0);

						/* reserved is pull up */
						if(need_pull_pio){
							sw_gpio_setpull(port_info->id.gpio_set.gpio.gpio, 1);
						}
					}

					/* request det_vbus gpio */
					if(port_info->det_vbus.valid){
						ret = gpio_request(port_info->det_vbus.gpio_set.gpio.gpio, "otg_det");
						if(ret != 0){
							DMSG_PANIC("ERR: det_vbus gpio_request failed\n");
							ret = -1;
							port_info->det_vbus.valid = 0;
							goto failed;
						}

						/* set config, input */
						sw_gpio_setcfg(port_info->det_vbus.gpio_set.gpio.gpio, 0);

						/* reserved is disable */
						if(need_pull_pio){
							sw_gpio_setpull(port_info->det_vbus.gpio_set.gpio.gpio, 0);
						}
					}

					__usb_hw_scan = vbus_id_hw_scan;
				}
				break;

				default:
					DMSG_PANIC("ERR: unkown detect_type(%d)\n", port_info->detect_type);
					ret = -1;
					goto failed;
			}
		}
		break;

		default:
			DMSG_PANIC("ERR: unkown port_type(%d)\n", cfg->port[0].port_type);
			ret = -1;
			goto failed;
	}
    if(port_info->ac_enable.valid){
        ret = gpio_request(port_info->ac_enable.gpio_set.gpio.gpio, "ac_enable");
        if(ret != 0){
            DMSG_PANIC("ERR: ac_enable gpio_request failed\n");
            port_info->ac_enable.valid = 0;
        }else{
            /* set config, ouput */
            sw_gpio_setcfg(port_info->ac_enable.gpio_set.gpio.gpio, 1);

            /* reserved is pull down */
            sw_gpio_setpull(port_info->ac_enable.gpio_set.gpio.gpio, 2);
        }
    }

	return 0;

failed:
	if(port_info->id.valid){
		gpio_free(port_info->id.gpio_set.gpio.gpio);
	}

	if(port_info->det_vbus.valid){
		gpio_free(port_info->det_vbus.gpio_set.gpio.gpio);
	}

	__usb_hw_scan = null_hw_scan;

	return ret;
}

/*
*******************************************************************************
*                     usb_hw_scan_exit
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
__s32 usb_hw_scan_exit(struct usb_cfg *cfg)
{
	if(cfg->port[0].id.valid){
		gpio_free(cfg->port[0].id.gpio_set.gpio.gpio);
	}

	if(cfg->port[0].det_vbus.valid){
		gpio_free(cfg->port[0].det_vbus.gpio_set.gpio.gpio);
	}

	return 0;
}


