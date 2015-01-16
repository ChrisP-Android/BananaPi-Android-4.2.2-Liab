/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_udc_dma.c
*
* Author 		: javen
*
* Description 	: DMA 操作集
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-3-3            1.0          create this file
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
#include <asm/cacheflush.h>
#include <mach/dma.h>

#include  "sw_udc_config.h"
#include  "sw_udc_board.h"
#include  "sw_udc_dma.h"


#ifdef  SW_UDC_DMA

#define  is_tx_ep(ep)		((ep->bEndpointAddress) & USB_DIR_IN)

extern void sw_udc_dma_completion(struct sw_udc *dev, struct sw_udc_ep *ep, struct sw_udc_request *req);

/*
*******************************************************************************
*                     sw_udc_switch_bus_to_dma
*
* Description:
*    切换 USB 总线给 DMA
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
void sw_udc_switch_bus_to_ddma(struct sw_udc_ep *ep, u32 is_tx)
{
	if(!is_tx){ /* ep in, rx */
		USBC_SelectBus(ep->dev->sw_udc_io->usb_bsp_hdle,
			           USBC_IO_TYPE_DMA,
			           USBC_EP_TYPE_RX,
			           ep->num);
	}else{  /* ep out, tx */
		USBC_SelectBus(ep->dev->sw_udc_io->usb_bsp_hdle,
					   USBC_IO_TYPE_DMA,
					   USBC_EP_TYPE_TX,
					   ep->num);
	}

    return;
}

/*
*******************************************************************************
*                     sw_udc_switch_bus_to_pio
*
* Description:
*    切换 USB 总线给 PIO
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
void sw_udc_switch_bus_to_pio(struct sw_udc_ep *ep, __u32 is_tx)
{
	USBC_SelectBus(ep->dev->sw_udc_io->usb_bsp_hdle, USBC_IO_TYPE_PIO, 0, 0);

    return;
}

/*
*******************************************************************************
*                     sw_udc_enable_dma_channel_irq
*
* Description:
*    使能 DMA channel 中断
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
void sw_udc_enable_dma_channel_irq(struct sw_udc_ep *ep)
{
	DMSG_DBG_DMA("sw_udc_enable_dma_channel_irq\n");

    return;
}

/*
*******************************************************************************
*                     sw_udc_disable_dma_channel_irq
*
* Description:
*    禁止 DMA channel 中断
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
void sw_udc_disable_dma_channel_irq(struct sw_udc_ep *ep)
{
	DMSG_DBG_DMA("sw_udc_disable_dma_channel_irq\n");

    return;
}

/*
*******************************************************************************
*                     sw_udc_dma_set_config
*
* Description:
*    配置 DMA
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
void sw_udc_dma_set_config(struct sw_udc_ep *ep, struct sw_udc_request *req, __u32 buff_addr, __u32 len)
{	
	int ret 		= 0;
	int ep_drq_type = 0;
    
 	dma_config_t dmaconfig;
    u32 state = 2;
	memset(&dmaconfig, 0, sizeof(dmaconfig));    
	
	if(is_tx_ep(ep)){ /* ep in, tx*/
        switch(ep->num){
            case 1:
                ep_drq_type = N_DST_USB_EP1;
            break;    
            case 2:
                ep_drq_type = N_DST_USB_EP2;
            break;    
            case 3:
                ep_drq_type = N_DST_USB_EP3;
            break;    
            case 4:
                ep_drq_type = N_DST_USB_EP4;
            break;    
            case 5:
                ep_drq_type = N_DST_USB_EP5;
            break;    
            default:
                ep_drq_type = 0;
        }


		dmaconfig.src_drq_type                  = N_DST_SDRAM;
		dmaconfig.dst_drq_type                  = ep_drq_type;
		dmaconfig.xfer_type.src_data_width      = DATA_WIDTH_32BIT; 
        dmaconfig.xfer_type.src_bst_len         = DATA_BRST_8;  
        dmaconfig.xfer_type.dst_data_width      = DATA_WIDTH_32BIT;
        dmaconfig.xfer_type.dst_bst_len         = DATA_BRST_8;
		dmaconfig.address_type.src_addr_mode    = NDMA_ADDR_INCREMENT ;	
		dmaconfig.address_type.dst_addr_mode    = NDMA_ADDR_NOCHANGE;
		dmaconfig.irq_spt       = CHAN_IRQ_FD;		
		dmaconfig.bconti_mode   = false;		
	}
	else{ /* ep out, rx*/
        switch(ep->num){
    		case 1:
    			ep_drq_type = N_SRC_USB_EP1;
    		break;
    		case 2:
    			ep_drq_type = N_SRC_USB_EP2;
    		break;
    		case 3:
    			ep_drq_type = N_SRC_USB_EP3;
    		break;
    		case 4:
    			ep_drq_type = N_SRC_USB_EP4;
    		break;
    		case 5:
    			ep_drq_type = N_SRC_USB_EP5;
    		break;
    		default:
    			ep_drq_type = 0;
    	}
		dmaconfig.src_drq_type                  = ep_drq_type;
		dmaconfig.dst_drq_type                  = N_DST_SDRAM;
        dmaconfig.xfer_type.src_data_width      = DATA_WIDTH_32BIT; 
        dmaconfig.xfer_type.src_bst_len         = DATA_BRST_8;  
        dmaconfig.xfer_type.dst_data_width      = DATA_WIDTH_32BIT;
        dmaconfig.xfer_type.dst_bst_len         = DATA_BRST_8;
		dmaconfig.address_type.src_addr_mode    = NDMA_ADDR_NOCHANGE;	
		dmaconfig.address_type.dst_addr_mode    = NDMA_ADDR_INCREMENT;
		dmaconfig.irq_spt       = CHAN_IRQ_FD;			
		dmaconfig.bconti_mode   = false;
	}

	ret = sw_dma_config(ep->dev->sw_udc_dma[ep->num].dma_hdle, &dmaconfig);
	if(ret != 0) {
		DMSG_PANIC("ERR: sw_dma_config failed\n");
		return;
	}

    ret = sw_dma_ctl(ep->dev->sw_udc_dma[ep->num].dma_hdle, DMA_OP_SET_WAIT_STATE, &state);
    if(ret != 0) {
		DMSG_PANIC("ERR: set wait state failed\n");
		return;
	}
	
	return;
}

/*
*******************************************************************************
*                     sw_udc_dma_start
*
* Description:
*    开始 DMA 传输
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
void sw_udc_dma_start(struct sw_udc_ep *ep, __u32 fifo, __u32 buffer, __u32 len)
{
    __u32 fifo_addr = 0;
	DMSG_DBG_DMA("sw_udc_dma_start: ep(%d), fifo = 0x%x, buffer = (0x%x, 0x%x), len = 0x%x\n",
		      ep->num, fifo, buffer, (u32)phys_to_virt(buffer), len);
    
    fifo_addr = USBC_REG_EPFIFOx((u32)ep->dev->sw_udc_io->usb_vbase, ep->num);
    fifo_addr &= 0xfffffff;
	
	if(is_tx_ep(ep))
	    sw_dma_enqueue(ep->dev->sw_udc_dma[ep->num].dma_hdle, buffer, fifo_addr, len);
	else   
	    sw_dma_enqueue(ep->dev->sw_udc_dma[ep->num].dma_hdle, fifo_addr, buffer, len);
    if(!ep->dev->sw_udc_dma[ep->num].is_start){
		ep->dev->sw_udc_dma[ep->num].is_start = 1;            	
    	sw_dma_ctl(ep->dev->sw_udc_dma[ep->num].dma_hdle, DMA_OP_START, NULL);
    }
    return;
}

/*
*******************************************************************************
*                     sw_udc_dma_stop
*
* Description:
*    终止 DMA 传输
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
void sw_udc_dma_stop(struct sw_udc_ep *ep)
{
    int ret = 0;
	DMSG_DBG_DMA("sw_udc_dma_stop\n");

	ret = sw_dma_ctl((dma_hdl_t)ep->dev->sw_udc_dma[ep->num].dma_hdle, DMA_OP_STOP, NULL);	
    ep->dev->sw_udc_dma[ep->num].is_start = 0;	

    return;
}

/*
*******************************************************************************
*                     sw_udc_dma_transmit_length
*
* Description:
*    查询 DMA 已经传输的长度
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
#if 0
__u32 sw_udc_dma_left_length(struct sw_udc_ep *ep, __u32 is_in, __u32 buffer_addr)
{
    dma_addr_t src = 0;
    dma_addr_t dst = 0;
	__u32 dma_buffer = 0;
	__u32 left_len = 0;

	DMSG_DBG_DMA("sw_udc_dma_transmit_length\n");

	sw_dma_getcurposition(ep->dev->sw_udc_dma.dma_hdle, &src, &dst);
	if(is_in){	/* tx */
		dma_buffer = (__u32)src;
	}else{	/* rx */
		dma_buffer = (__u32)dst;
	}

	left_len = buffer_addr - (u32)phys_to_virt(dma_buffer);

    DMSG_DBG_DMA("dma transfer lenght, buffer_addr(0x%x), dma_buffer(0x%x), left_len(%d), want(%d)\n",
		      buffer_addr, dma_buffer, left_len, ep->dma_transfer_len);

    return left_len;
}

__u32 sw_udc_dma_transmit_length(struct sw_udc_ep *ep, __u32 is_in, __u32 buffer_addr)
{
    if(ep->dma_transfer_len){
		return (ep->dma_transfer_len - sw_udc_dma_left_length(ep, is_in, buffer_addr));
	}

	return ep->dma_transfer_len;
}

#else
__u32 sw_udc_dma_transmit_length(struct sw_udc_ep *ep, __u32 is_in, __u32 buffer_addr)
{
    return ep->dma_transfer_len;
}
#endif

/*
*******************************************************************************
*                     sw_udc_dma_probe
*
* Description:
*    DMA 初始化
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
__u32 sw_udc_dma_is_busy(struct sw_udc_ep *ep)
{
	return 	ep->dma_working;
}

/*
*******************************************************************************
*                     sw_udc_dma_probe
*
* Description:
*    DMA 初始化
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

static void sw_udc_dma_callback(dma_hdl_t dma_hdl, void *parg)
{
    struct sw_udc *dev = NULL;
    struct sw_udc_request *req = NULL;
    struct sw_udc_ep *ep = NULL;
    int i = 1;
    
    dev = (struct sw_udc *)parg;
    if(dev == NULL) {
        DMSG_PANIC("ERR: sw_udc_dma_callback failed\n");
        return;
    }

    /* find ep */
    for(i = 1; i < (USBC_MAX_EP_NUM - 1); i++){
        if(dma_hdl == dev->sw_udc_dma[i].dma_hdle){
            ep = &dev->ep[i];
            break;
        }
    }

    if(ep){
        /* find req */
        if(likely (!list_empty(&ep->queue))){
            req = list_entry(ep->queue.next, struct sw_udc_request, queue);
        }else{
            req = NULL;
        }

        /* call back */
        if(req){            
            sw_udc_dma_completion(dev, ep, req);
        }
    }else{
        DMSG_PANIC("ERR: sw_udc_dma_callback: dma is remove, but dma irq is happened\n");
    }

    return;
}


/*
*******************************************************************************
*                     sw_udc_dma_probe
*
* Description:
*    DMA 初始化
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
__s32 sw_udc_dma_probe(struct sw_udc *dev)
{
    int ret = 0;
	int i = 0;
	dma_cb_t done_cb;

	for(i = 1; i <= (USBC_MAX_EP_NUM - 1); i++){
		char* p = dev->sw_udc_dma[i].name;
		strcpy(dev->sw_udc_dma[i].name, dev->driver_name);
		strcat(dev->sw_udc_dma[i].name, "_dma_");
		p[strlen(p) + 1] = 0;
		p[strlen(p)] = i + '0';

		dev->sw_udc_dma[i].dma_hdle = sw_dma_request(dev->sw_udc_dma[i].name, CHAN_NORAML);
		if(dev->sw_udc_dma[i].dma_hdle == 0) {
			DMSG_PANIC("ERR: sw_dma_request failed\n");
			return -1;
		}
		DMSG_INFO("[%s %d]:name:%s dma_hdle[%d] = 0x%x\n", __func__, __LINE__, dev->sw_udc_dma[i].name, i, (__u32)dev->sw_udc_dma[i].dma_hdle);
	}
	/*set callback */
	memset(&done_cb, 0, sizeof(done_cb));

	done_cb.func = sw_udc_dma_callback;
	done_cb.parg = dev;

	for(i = 1; i <= (USBC_MAX_EP_NUM - 1); i++ ){
		ret = sw_dma_ctl((dma_hdl_t)dev->sw_udc_dma[i].dma_hdle, DMA_OP_SET_FD_CB, (void *)&done_cb);
		if(ret != 0){
			DMSG_PANIC("ERR: set callback failed\n");
			return -1;
		}
	}

    return 0;
}

/*
*******************************************************************************
*                     sw_udc_dma_remove
*
* Description:
*    DMA 移除
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
__s32 sw_udc_dma_remove(struct sw_udc *dev)
{
	int ret = 0;
	int i = 0;
#if 0
	__u32 channel = 0;

	DMSG_INFO_UDC("dma remove\n");

	channel = DMACH_DUSB0;


	if(dev->sw_udc_dma.dma_hdle >= 0){
		sw_dma_free(channel, &(dev->sw_udc_dma.dma_client));
		dev->sw_udc_dma.dma_hdle = -1;
	}else{
		DMSG_PANIC("ERR: sw_udc_dma_remove, dma_hdle is null\n");
	}

	//memset(&sw_udc_dma_para, 0, sizeof(sw_udc_dma_parg_t));

#endif
	for(i = 1; i <= (USBC_MAX_EP_NUM - 1); i++){
		if(dev->sw_udc_dma[i].dma_hdle != 0) {
			ret = sw_dma_ctl((dma_hdl_t)dev->sw_udc_dma[i].dma_hdle, DMA_OP_STOP, NULL);
			if(ret != 0) {
				DMSG_PANIC("ERR: sw_udc_dma_remove: stop failed\n");
			}

			ret = sw_dma_release((dma_hdl_t)dev->sw_udc_dma[i].dma_hdle);

			if(ret != 0) {
				DMSG_PANIC("sw_udc_dma_remove: sw_dma_release failed\n");
			}

			dev->sw_udc_dma[i].dma_hdle = 0;
			dev->sw_udc_dma[i].is_start = 0;
			dev->ep[i].dma_working = 0;
			dev->ep[i].dma_transfer_len = 0;
		}
	}

	return 0;
}

#else

void sw_udc_switch_bus_to_ddma(struct sw_udc_ep *ep, u32 is_tx){}
void sw_udc_switch_bus_to_pio(struct sw_udc_ep *ep, __u32 is_tx){}

void sw_udc_enable_dma_channel_irq(struct sw_udc_ep *ep){}
void sw_udc_disable_dma_channel_irq(struct sw_udc_ep *ep){}

void sw_udc_dma_set_config(struct sw_udc_ep *ep, struct sw_udc_request *req, __u32 buff_addr, __u32 len){}
void sw_udc_dma_start(struct sw_udc_ep *ep, __u32 fifo, __u32 buffer, __u32 len){}
void sw_udc_dma_stop(struct sw_udc_ep *ep){}
__u32 sw_udc_dma_transmit_length(struct sw_udc_ep *ep, __u32 is_in, __u32 buffer_addr){return 0;}
__u32 sw_udc_dma_is_busy(struct sw_udc_ep *ep){return 0;}

__s32 sw_udc_dma_probe(struct sw_udc *dev){return 0;}
__s32 sw_udc_dma_remove(struct sw_udc *dev){return 0;}

#endif

